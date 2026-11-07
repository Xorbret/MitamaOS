#pragma once
// Update fetcher: asks the MitamaOS Pages site which build is newest and
// downloads that build's merged image to the SD card, where Launcher can
// install it (Launcher > SD). It also fetches the lesson pack (Shugyo
// Trials and Harae drills) and unpacks it into /MitamaOS/study/. MitamaOS can't overwrite itself while it
// runs, and Launcher keeps its own flash layout, so the last step stays
// in Launcher.
//
// https://xorbret.github.io/MitamaOS/firmware/version.txt holds the build
// id (short git hash) of the newest build; the image sits next to it.
// TLS is checked against the Mozilla roots in ca_bundle.h.
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include "ca_bundle.h"
#include "../study/lesson.h"   // study::kEngine

#ifndef MITAMA_BUILD
#define MITAMA_BUILD "dev"
#endif

namespace OsUpdate {

static constexpr const char* HOST = "xorbret.github.io";
static constexpr const char* BASE = "/MitamaOS/firmware/";
static constexpr const char* IMAGE = "mitamaos-cardputer-adv-merged.bin";
static constexpr const char* DIR = "/MitamaOS/update";

inline const char* current() { return MITAMA_BUILD; }

// Sends a GET and reads the headers. Returns the body length (-1 if the
// server didn't say), or -2 on failure with `msg` set. A silent or
// dropped connection is tried again up to three times (GitHub's servers
// can be slow to start sending a file right after a new build lands).
inline long _get1(WiFiClientSecure& c, const String& path, String& msg, bool& retry) {
    retry = false;
    c.setCACertBundle(CA_BUNDLE);
    c.setTimeout(20);
    if (!c.connect(HOST, 443)) {
        char err[48] = ""; c.lastError(err, sizeof(err));
        msg = String("Can't reach the update site") + (err[0] ? String(": ") + err : "");
        retry = true; return -2;
    }
    c.print(String("GET ") + path + " HTTP/1.1\r\nHost: " + HOST +
            "\r\nUser-Agent: MitamaOS\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n");
    String status = c.readStringUntil('\n');
    status.trim();
    if (!status.length()) {
        char err[48] = ""; c.lastError(err, sizeof(err));
        msg = String("No answer from the update site") + (err[0] ? String(": ") + err : "")
            + " (" + String(ESP.getMaxAllocHeap() / 1024) + "K free)";
        c.stop(); retry = true; return -2;
    }
    if (status.indexOf(" 200") < 0) {
        msg = "Update site said: " + status.substring(status.indexOf(' ') + 1);
        c.stop(); return -2;
    }
    long len = -1;
    for (;;) {
        String h = c.readStringUntil('\n');
        h.trim();
        if (!h.length()) break;
        if (h.substring(0, 15).equalsIgnoreCase("Content-Length:")) len = h.substring(15).toInt();
    }
    return len;
}

inline long _get(WiFiClientSecure& c, const String& path, String& msg) {
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt) delay(1500 * attempt);
        bool retry;
        long len = _get1(c, path, msg, retry);
        if (len != -2 || !retry) return len;
        c.stop();
    }
    return -2;
}

// Newest build id, or "" with `msg` set.
inline String latest(String& msg) {
    WiFiClientSecure c;
    long len = _get(c, String(BASE) + "version.txt?t=" + millis(), msg);
    if (len == -2) return "";
    String v = c.readString();
    c.stop();
    v.trim();
    int nl = v.indexOf('\n');
    if (nl > 0) v = v.substring(0, nl);
    if (!v.length()) msg = "No version on the update site";
    return v;
}

// Streams `file` from the site to `path` on the SD card (via a .part file,
// renamed only when complete). Returns false with `msg` set.
inline bool _fetch(const String& file, const String& path, void (*progress)(size_t, size_t), String& msg) {
    if (!SD.exists("/MitamaOS")) SD.mkdir("/MitamaOS");
    if (!SD.exists(DIR)) SD.mkdir(DIR);
    WiFiClientSecure c;
    long len = _get(c, String(BASE) + file + "?t=" + millis(), msg);
    if (len == -2) return false;
    if (len <= 0) { c.stop(); msg = "Update size unknown"; return false; }
    if (SD.totalBytes() - SD.usedBytes() < (uint64_t)len + 65536) { c.stop(); msg = "SD card is full"; return false; }
    String tmp = path + ".part";
    SD.remove(tmp.c_str());
    File f = SD.open(tmp.c_str(), FILE_WRITE);
    if (!f) { c.stop(); msg = "Can't write to the SD card"; return false; }
    uint8_t* buf = (uint8_t*)malloc(4096);
    if (!buf) { f.close(); c.stop(); msg = "Not enough memory"; return false; }
    size_t done = 0;
    uint32_t last = millis();
    while (done < (size_t)len) {
        int n = c.read(buf, min((size_t)4096, (size_t)len - done));
        if (n > 0) {
            if (f.write(buf, n) != (size_t)n) { msg = "SD write failed"; break; }
            done += n;
            last = millis();
            if (progress) progress(done, len);
        } else if (!c.connected() || millis() - last > 20000) {
            msg = "Download interrupted"; break;
        } else {
            delay(5);
        }
    }
    free(buf);
    f.close();
    c.stop();
    if (done != (size_t)len) { SD.remove(tmp.c_str()); return false; }
    SD.remove(path.c_str());
    SD.rename(tmp.c_str(), path.c_str());
    return true;
}

// Downloads the newest image to /MitamaOS/update/mitamaos-<build>.bin.
// progress(done, total) is called as it goes. Returns the path, or ""
// with `msg` set; a part-finished download is deleted.
inline String download(const String& build, void (*progress)(size_t, size_t), String& msg) {
    String path = String(DIR) + "/mitamaos-" + build + ".bin";
    return _fetch(IMAGE, path, progress, msg) ? path : String();
}

// ---- lesson pack ---------------------------------------------------------
// study.txt on the site: "<version> <engine> <bytes>". study.pack: a header
// line "MITAMA-STUDY 1 <version> <engine>", then per file
// "F <lessons|scenarios>/<name>.lua <bytes>\n" + the bytes + "\n", then "E".
// It unpacks into /MitamaOS/study/; pack.txt there lists the files the
// last pack wrote, so files a newer pack drops are removed and files the
// player added by hand are never touched.
static constexpr const char* STUDY_DIR  = "/MitamaOS/study";
static constexpr const char* STUDY_PACK = "/MitamaOS/update/study.pack";

// The installed pack's version, or "".
inline String studyInstalled() {
    File f = SD.open((String(STUDY_DIR) + "/version.txt").c_str(), FILE_READ);
    if (!f) return "";
    String v = f.readStringUntil('\n');
    f.close();
    v.trim();
    int sp = v.indexOf(' ');
    return sp > 0 ? v.substring(0, sp) : v;
}

// The site's pack version, or "" with `msg` set. `engine` gets the lesson
// engine it needs.
inline String studyLatest(int& engine, String& msg) {
    WiFiClientSecure c;
    long len = _get(c, String(BASE) + "study.txt?t=" + millis(), msg);
    if (len == -2) return "";
    String v = c.readString();
    c.stop();
    v.trim();
    int sp = v.indexOf(' ');
    if (sp <= 0) { msg = "No lesson pack on the update site"; return ""; }
    engine = v.substring(sp + 1).toInt();
    return v.substring(0, sp);
}

inline bool _safeName(const String& n) {
    if (n.length() < 5 || n.length() > 64 || !n.endsWith(".lua")) return false;
    for (unsigned i = 0; i < n.length(); i++) {
        char ch = n[i];
        if (!(isalnum((unsigned char)ch) || ch == '-' || ch == '_' || ch == '.')) return false;
    }
    return n.indexOf("..") < 0;
}

// Unpacks a downloaded pack. Returns the number of files written, or -1
// with `msg` set (a bad pack writes nothing it hasn't fully checked).
inline int studyInstall(const char* packPath, String& msg) {
    File in = SD.open(packPath, FILE_READ);
    if (!in) { msg = "Lesson pack missing"; return -1; }
    String head = in.readStringUntil('\n');
    head.trim();
    // "MITAMA-STUDY 1 <version> <engine>"
    if (!head.startsWith("MITAMA-STUDY 1 ")) { in.close(); msg = "Lesson pack is damaged"; return -1; }
    String rest = head.substring(15);
    int sp = rest.indexOf(' ');
    String version = sp > 0 ? rest.substring(0, sp) : rest;
    int engine = sp > 0 ? rest.substring(sp + 1).toInt() : 1;
    if (engine > study::kEngine) { in.close(); msg = "These lessons need a newer MitamaOS"; return -1; }

    if (!SD.exists("/MitamaOS")) SD.mkdir("/MitamaOS");
    if (!SD.exists(STUDY_DIR)) SD.mkdir(STUDY_DIR);
    const String dirs[2] = { String(STUDY_DIR) + "/lessons", String(STUDY_DIR) + "/scenarios" };
    for (auto& d : dirs) if (!SD.exists(d.c_str())) SD.mkdir(d.c_str());

    uint8_t* buf = (uint8_t*)malloc(2048);
    if (!buf) { in.close(); msg = "Not enough memory"; return -1; }
    String written;          // one path per line, for pack.txt
    int count = 0;
    bool ok = false;
    for (;;) {
        String line = in.readStringUntil('\n');
        if (line == "E") { ok = true; break; }
        // "F <kind>/<name> <bytes>"
        int s1 = line.indexOf(' '), s2 = line.lastIndexOf(' ');
        if (!line.startsWith("F ") || s2 <= s1) { msg = "Lesson pack is damaged"; break; }
        String rel = line.substring(s1 + 1, s2);
        long len = line.substring(s2 + 1).toInt();
        int slash = rel.indexOf('/');
        String kind = slash > 0 ? rel.substring(0, slash) : "";
        String name = slash > 0 ? rel.substring(slash + 1) : "";
        if ((kind != "lessons" && kind != "scenarios") || !_safeName(name) || len <= 0 || len > 32768) {
            msg = "Lesson pack has a bad entry"; break;
        }
        String path = String(STUDY_DIR) + "/" + kind + "/" + name, tmp = path + ".part";
        SD.remove(tmp.c_str());
        File out = SD.open(tmp.c_str(), FILE_WRITE);
        if (!out) { msg = "Can't write to the SD card"; break; }
        long left = len;
        while (left > 0) {
            int n = in.read(buf, left < 2048 ? left : 2048);
            if (n <= 0 || out.write(buf, n) != (size_t)n) break;
            left -= n;
        }
        out.close();
        if (left > 0 || in.read() != '\n') { SD.remove(tmp.c_str()); msg = "Lesson pack is cut short"; break; }
        SD.remove(path.c_str());   // FAT can't rename over a file
        SD.rename(tmp.c_str(), path.c_str());
        written += kind + "/" + name + "\n";
        count++;
    }
    free(buf);
    in.close();
    if (!ok) return -1;

    // Remove what the previous pack wrote and this one doesn't have.
    String listPath = String(STUDY_DIR) + "/pack.txt";
    File old = SD.open(listPath.c_str(), FILE_READ);
    if (old) {
        while (old.available()) {
            String rel = old.readStringUntil('\n');
            rel.trim();
            if (!rel.length() || written.indexOf(rel + "\n") >= 0) continue;
            int slash = rel.indexOf('/');
            String kind = slash > 0 ? rel.substring(0, slash) : "", name = slash > 0 ? rel.substring(slash + 1) : "";
            if ((kind == "lessons" || kind == "scenarios") && _safeName(name))
                SD.remove((String(STUDY_DIR) + "/" + rel).c_str());
        }
        old.close();
    }
    SD.remove(listPath.c_str());
    File list = SD.open(listPath.c_str(), FILE_WRITE);
    if (list) { list.print(written); list.close(); }
    String verPath = String(STUDY_DIR) + "/version.txt";
    SD.remove(verPath.c_str());
    File ver = SD.open(verPath.c_str(), FILE_WRITE);
    if (ver) { ver.printf("%s %d\n", version.c_str(), engine); ver.close(); }
    SD.remove(packPath);
    return count;
}

// Downloads the site's lesson pack and unpacks it. Returns the number of
// files, or -1 with `msg` set.
inline int studyUpdate(void (*progress)(size_t, size_t), String& msg) {
    if (!_fetch("study.pack", STUDY_PACK, progress, msg)) return -1;
    return studyInstall(STUDY_PACK, msg);
}

} // namespace OsUpdate
