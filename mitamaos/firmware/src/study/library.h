#pragma once
// What Shugyo and Harae share: the list of lessons and drills, the player's
// progress, and the hardware lessons may use.
//
// Content: the Foundations lessons and one practice drill are built into
// the firmware, so Shugyo always works. Everything else (the faction arcs
// and their drills) is a lesson pack on the SD card in /MitamaOS/study/,
// which Molt downloads with each build. A pack that needs a newer lesson
// engine than this firmware (kEngine) is left alone until MitamaOS updates.
// Reading every file at start-up would be slow, so what each file holds is
// cached in /MitamaOS/study/index.txt; only new or changed files are read.
//
// Progress lives in NVS "study" (so it works without a card), split across
// keys c0..c3 so it can't outgrow one NVS string, and is mirrored to
// settings.conf by SdSettings, so updates keep it. A passed lesson task is
// the Ward that lesson teaches.
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "builtin_content.h"
#include "game.h"
#include "lesson.h"
#include "mitama_hal.h"
#include "progress.h"
#include "../system/sd_settings.h"

#ifdef MITAMA_BUILD
#define STUDY_BUILD MITAMA_BUILD
#else
#define STUDY_BUILD "dev"
#endif

namespace study {

struct LessonInfo {
    std::string id, title, file;
    int track = 0;
    bool hasTask = false;
    const char* builtin = nullptr;   // source in flash, or null for an SD file
    std::vector<std::string> prereqs;  // lessons to pass first
    std::string card, tag;             // Harae card earned, cert-domain label
    bool hasField = false;             // has an optional field task
};

struct DrillInfo {
    std::string id, title, faction, demon, file;
    int rounds = 0;
    const char* builtin = nullptr;
    std::vector<std::string> wards;  // every lesson a response in it can need
};

class Library {
public:
    static Library& get() { static Library l; return l; }
    static constexpr const char* SD_LESSONS = "/MitamaOS/study/lessons";
    static constexpr const char* SD_DRILLS  = "/MitamaOS/study/scenarios";
    static constexpr const char* SD_VERSION = "/MitamaOS/study/version.txt";
    static constexpr const char* SD_INDEX   = "/MitamaOS/study/index.txt";
    static constexpr int SD_MAX_FILES = 128;    // per folder
    static constexpr int PROGRESS_KEYS = 4;     // c0..c3, 3800 bytes each

    // What the SD card's lesson pack looks like.
    enum SdState : uint8_t { SD_NO_CARD, SD_NO_PACK, SD_TOO_NEW, SD_OK };
    SdState sdState = SD_NO_PACK;
    std::string packVersion;                    // "" if none installed

    // One line for the player when the faction Trials are missing, or null.
    const char* sdNote() const {
        switch (sdState) {
            case SD_NO_CARD: return "Faction Trials and drills need the SD card.";
            case SD_NO_PACK: return "Get the faction Trials and drills with Molt.";
            case SD_TOO_NEW: return "These Trials need a newer build: run Molt.";
            default:         return nullptr;
        }
    }

    MitamaHal hal;
    Progress progress;

    // Builds the lists once (and again after rescan()). Content that fails to
    // load or is unfair is skipped and counted in `skipped`.
    void ensure() {
        if (_built) return;
        _built = true;
        _loadProgress();
        lessons.clear(); drills.clear(); skipped = 0;
        for (int i = 0; i < kBuiltin_lessons_count; i++)
            _addLesson(kBuiltin_lessons[i].name, kBuiltin_lessons[i].source, std::string());
        for (int i = 0; i < kBuiltin_scenarios_count; i++)
            _addDrill(kBuiltin_scenarios[i].name, kBuiltin_scenarios[i].source, std::string());
        _scanPack();
        std::stable_sort(lessons.begin(), lessons.end(), [](const LessonInfo& a, const LessonInfo& b) {
            return a.track != b.track ? a.track < b.track : a.file < b.file;
        });
        std::stable_sort(drills.begin(), drills.end(), [](const DrillInfo& a, const DrillInfo& b) { return a.file < b.file; });
    }
    void rescan() { _built = false; ensure(); }
    // Molt calls this after unpacking a new pack: the lists are rebuilt the
    // next time Shugyo or Harae opens.
    void invalidate() { _built = false; }

    std::vector<LessonInfo> lessons;
    std::vector<DrillInfo> drills;
    int skipped = 0;

    const LessonInfo* lessonById(const std::string& id) const {
        for (auto& l : lessons) if (l.id == id) return &l;
        return nullptr;
    }
    // The first prerequisite not yet passed, or null if the lesson is open.
    const std::string* lockedBy(const LessonInfo& l) const {
        for (auto& p : l.prereqs) if (!progress.hasWard(p)) return &p;
        return nullptr;
    }
    // Card name for a Ward (falls back to the lesson title).
    std::string wardName(const std::string& id) const {
        const LessonInfo* l = lessonById(id);
        return !l ? id : l->card.empty() ? l->title : l->card;
    }
    std::string lessonTitle(const std::string& id) const {
        const LessonInfo* l = lessonById(id);
        return l ? l->title : id;
    }

    bool source(const LessonInfo& l, std::string& out) const { return _source(l.builtin, l.file, SD_LESSONS, out); }
    bool source(const DrillInfo& d, std::string& out) const { return _source(d.builtin, d.file, SD_DRILLS, out); }

    bool loadDrill(const DrillInfo& d, Scenario& out, std::string& err) {
        std::string src;
        if (!source(d, src)) { err = "can't read " + d.file; return false; }
        return loadScenario(&hal, src, out, err);
    }

    // Wards this drill still needs; empty = fair to play now.
    std::vector<std::string> sealedBy(const Scenario& s) const {
        return missingWards(s, [this](const std::string& w) { return progress.hasWard(w); });
    }

    int wardsHeld() const {
        int n = 0;
        for (auto& l : lessons) n += progress.hasWard(l.id) ? 1 : 0;
        return n;
    }

    void save() {
        // One record per ';' (settings.conf is one key per line), cut into
        // pieces under the NVS string limit.
        std::vector<std::string> parts = progress.chunks(3800);
        if ((int)parts.size() > PROGRESS_KEYS) {
            Serial.printf("[study] progress too big: %u pieces, keeping %d\n", (unsigned)parts.size(), PROGRESS_KEYS);
            parts.resize(PROGRESS_KEYS);
        }
        Preferences p; p.begin("study", false);
        for (int i = 0; i < PROGRESS_KEYS; i++) {
            char k[4] = { 'c', char('0' + i), 0 };
            if (i < (int)parts.size()) p.putString(k, parts[i].c_str());
            else if (p.isKey(k)) p.remove(k);
        }
        if (p.isKey("p")) p.remove("p");   // the old single-key format
        p.end();
        SdSettings::changed();
        _writeFieldLog();
    }

    // A plain Markdown record of what the learner has done, for a
    // portfolio: /MitamaOS/user/Study/field-log.md.
    std::string fieldLog() const {
        std::string md = "# Shugyo field log\n\nTrials passed on MitamaOS. Every attack was simulated; nothing was transmitted.\n\n";
        md += "| Trial | Card earned | Quiz | Field task | Domain |\n| --- | --- | --- | --- | --- |\n";
        int n = 0;
        for (auto& l : lessons) {
            const LessonRecord* r = progress.lesson(l.id);
            if (!r || !r->taskPassed) continue;
            n++;
            std::string quiz = r->quizTotal ? std::to_string(r->quizScore) + "/" + std::to_string(r->quizTotal) : "-";
            md += "| " + l.title + " | " + (l.card.empty() ? "-" : l.card) + " | " + quiz + " | " +
                  (r->fieldDone ? "done" : l.hasField ? "not yet" : "-") + " | " + (l.tag.empty() ? "-" : l.tag) + " |\n";
        }
        if (!n) md += "| (none yet) | | | | |\n";
        return md;
    }

private:
    bool _built = false;

    void _writeFieldLog() {
        if (SD.cardType() == CARD_NONE) return;
        hal.fileWrite("field-log.md", fieldLog());
    }

    void _loadProgress() {
        Preferences p; p.begin("study", true);
        std::vector<std::string> parts;
        for (int i = 0; i < PROGRESS_KEYS; i++) {
            char k[4] = { 'c', char('0' + i), 0 };
            if (p.isKey(k)) parts.push_back(p.getString(k, "").c_str());
        }
        if (parts.empty() && p.isKey("p")) parts.push_back(p.getString("p", "").c_str());
        p.end();
        progress.parseChunks(parts);
    }

    static bool _source(const char* builtin, const std::string& file, const char* dir, std::string& out) {
        if (builtin) { out = builtin; return true; }
        File f = SD.open((std::string(dir) + "/" + file).c_str(), FILE_READ);
        if (!f) return false;
        out.clear();
        size_t n = f.size();
        if (n > 32768) { f.close(); return false; }
        out.reserve(n);
        while (f.available()) out += (char)f.read();
        f.close();
        return true;
    }

    // An SD file with the same id replaces the built-in one.
    void _putLesson(const LessonInfo& info) {
        for (auto& e : lessons) if (e.id == info.id) { e = info; return; }
        lessons.push_back(info);
    }
    void _putDrill(const DrillInfo& info) {
        for (auto& e : drills) if (e.id == info.id) { e = info; return; }
        drills.push_back(info);
    }

    bool _parseLesson(const std::string& file, const char* builtin, const std::string& sdSrc, LessonInfo& info) {
        Lesson l(&hal);
        std::string err;
        if (!l.load(builtin ? std::string(builtin) : sdSrc, err)) {
            Serial.printf("[study] %s: %s\n", file.c_str(), err.c_str());
            return false;
        }
        info = LessonInfo{ l.id(), l.title(), file, l.track(), l.hasTask(), builtin,
                           l.prereqs(), l.card(), l.tag(), !l.field().empty() };
        return true;
    }

    bool _parseDrill(const std::string& file, const char* builtin, const std::string& sdSrc, DrillInfo& info) {
        Scenario s;
        std::string err;
        if (!loadScenario(&hal, builtin ? std::string(builtin) : sdSrc, s, err)) {
            Serial.printf("[study] %s: %s\n", file.c_str(), err.c_str());
            return false;
        }
        info = DrillInfo{ s.id, s.title, s.faction, s.demon, file, (int)s.rounds.size(), builtin, {} };
        for (auto& r : s.rounds)
            for (auto& d : r.defenses)
                if (!d.ward.empty() && std::find(info.wards.begin(), info.wards.end(), d.ward) == info.wards.end())
                    info.wards.push_back(d.ward);
        return true;
    }

    void _addLesson(const std::string& file, const char* builtin, const std::string& sdSrc) {
        LessonInfo info;
        if (_parseLesson(file, builtin, sdSrc, info)) _putLesson(info); else skipped++;
    }
    void _addDrill(const std::string& file, const char* builtin, const std::string& sdSrc) {
        DrillInfo info;
        if (_parseDrill(file, builtin, sdSrc, info)) _putDrill(info); else skipped++;
    }

    // ---- the SD pack and its index -------------------------------------
    // index.txt: a header line, then one tab-separated line per file:
    //   L <file> <size> <time> id title track task prereqs card tag field
    //   D <file> <size> <time> id title faction demon rounds wards
    //   X <kind> <file> <size> <time>         (a file that failed to load)
    // Lists inside a field are comma-separated. A line is used only while
    // the file's size and time still match.
    struct SdFile { std::string name; size_t size; long time; };
    struct Cached { std::vector<std::string> f; bool used = false; };

    static std::string _clean(const std::string& s) {
        std::string o = s;
        for (char& c : o) if (c == '\t' || c == '\n' || c == '\r') c = ' ';
        return o;
    }
    static std::string _joinList(const std::vector<std::string>& v) {
        std::string o;
        for (auto& x : v) { if (!o.empty()) o += ','; o += _clean(x); }
        return o;
    }
    static std::vector<std::string> _split(const std::string& s, char sep) {
        std::vector<std::string> out;
        size_t from = 0;
        for (;;) {
            size_t at = s.find(sep, from);
            out.push_back(s.substr(from, at == std::string::npos ? std::string::npos : at - from));
            if (at == std::string::npos) return out;
            from = at + 1;
        }
    }
    static std::vector<std::string> _list(const std::string& s) {
        return s.empty() ? std::vector<std::string>() : _split(s, ',');
    }
    static std::string _key(char kind, const std::string& file, size_t size, long time) {
        return std::string(1, kind) + '/' + file + '/' + std::to_string(size) + '/' + std::to_string(time);
    }
    static long _num(const std::string& s) { return std::strtol(s.c_str(), nullptr, 10); }
    // A new build reads every file once more (it may load what an older one couldn't).
    // A new pack is read in full too (the card's clock may not be set, so a
    // changed file can keep its old time).
    std::string _indexHeader() const {
        return std::string("MITAMA-INDEX ") + std::to_string(kEngine) + " " + STUDY_BUILD + " " + packVersion;
    }

    std::vector<SdFile> _listDir(const char* dir) {
        std::vector<SdFile> out;
        File d = SD.open(dir);
        if (!d || !d.isDirectory()) return out;
        for (File f = d.openNextFile(); f; f = d.openNextFile()) {
            std::string n = f.name();
            n = n.substr(n.find_last_of('/') + 1);
            if (!f.isDirectory() && n.size() > 4 && n.compare(n.size() - 4, 4, ".lua") == 0)
                out.push_back({ n, (size_t)f.size(), (long)f.getLastWrite() });
            f.close();
            if ((int)out.size() >= SD_MAX_FILES) break;
        }
        d.close();
        std::sort(out.begin(), out.end(), [](const SdFile& a, const SdFile& b) { return a.name < b.name; });
        return out;
    }

    void _readPackVersion() {
        packVersion.clear();
        File f = SD.open(SD_VERSION, FILE_READ);
        if (!f) return;
        String line = f.readStringUntil('\n');
        f.close();
        line.trim();
        int sp = line.indexOf(' ');
        packVersion = (sp > 0 ? line.substring(0, sp) : line).c_str();
        int engine = sp > 0 ? line.substring(sp + 1).toInt() : 1;
        if (engine > kEngine) sdState = SD_TOO_NEW;
    }

    void _scanPack() {
        if (SD.cardType() == CARD_NONE) { sdState = SD_NO_CARD; packVersion.clear(); return; }
        sdState = SD_OK;
        _readPackVersion();
        if (sdState == SD_TOO_NEW) return;
        std::vector<SdFile> ls = _listDir(SD_LESSONS), ds = _listDir(SD_DRILLS);
        if (ls.empty() && ds.empty()) { sdState = SD_NO_PACK; return; }

        std::map<std::string, Cached> cache;
        {
            File f = SD.open(SD_INDEX, FILE_READ);
            if (f) {
                String head = f.readStringUntil('\n');
                head.trim();
                if (head == _indexHeader().c_str()) {
                    while (f.available()) {
                        String line = f.readStringUntil('\n');
                        if (!line.length()) continue;
                        std::vector<std::string> p = _split(line.c_str(), '\t');
                        if (p[0] == "L" && p.size() == 12) cache[_key('L', p[1], _num(p[2]), _num(p[3]))].f = p;
                        else if (p[0] == "D" && p.size() == 10) cache[_key('D', p[1], _num(p[2]), _num(p[3]))].f = p;
                        else if (p[0] == "X" && p.size() == 5 && p[1].size() == 1) cache[_key(p[1][0], p[2], _num(p[3]), _num(p[4]))].f = p;
                    }
                }
                f.close();
            }
        }
        bool dirty = false;
        std::string out = _indexHeader() + "\n";
        auto meta = [](const SdFile& f) { return f.name + '\t' + std::to_string(f.size) + '\t' + std::to_string(f.time); };

        for (auto& f : ls) {
            auto it = cache.find(_key('L', f.name, f.size, f.time));
            if (it != cache.end() && it->second.f[0] == "X") { it->second.used = true; skipped++; out += "X\tL\t" + meta(f) + "\n"; continue; }
            if (it != cache.end()) {
                auto& p = it->second.f;
                it->second.used = true;
                LessonInfo info{ p[4], p[5], f.name, std::atoi(p[6].c_str()), p[7] == "1", nullptr,
                                 _list(p[8]), p[9], p[10], p[11] == "1" };
                _putLesson(info);
                out += "L\t" + meta(f) + '\t' + p[4] + '\t' + p[5] + '\t' + p[6] + '\t' + p[7] + '\t' + p[8] + '\t' + p[9] + '\t' + p[10] + '\t' + p[11] + "\n";
                continue;
            }
            dirty = true;
            std::string src;
            LessonInfo info;
            if (!_source(nullptr, f.name, SD_LESSONS, src) || !_parseLesson(f.name, nullptr, src, info)) {
                skipped++; out += "X\tL\t" + meta(f) + "\n"; continue;
            }
            _putLesson(info);
            out += "L\t" + meta(f) + '\t' + _clean(info.id) + '\t' + _clean(info.title) + '\t' + std::to_string(info.track) +
                   '\t' + (info.hasTask ? "1" : "0") + '\t' + _joinList(info.prereqs) + '\t' + _clean(info.card) + '\t' +
                   _clean(info.tag) + '\t' + (info.hasField ? "1" : "0") + "\n";
        }
        for (auto& f : ds) {
            auto it = cache.find(_key('D', f.name, f.size, f.time));
            if (it != cache.end() && it->second.f[0] == "X") { it->second.used = true; skipped++; out += "X\tD\t" + meta(f) + "\n"; continue; }
            if (it != cache.end()) {
                auto& p = it->second.f;
                it->second.used = true;
                DrillInfo info{ p[4], p[5], p[6], p[7], f.name, std::atoi(p[8].c_str()), nullptr, _list(p[9]) };
                _putDrill(info);
                out += "D\t" + meta(f) + '\t' + p[4] + '\t' + p[5] + '\t' + p[6] + '\t' + p[7] + '\t' + p[8] + '\t' + p[9] + "\n";
                continue;
            }
            dirty = true;
            std::string src;
            DrillInfo info;
            if (!_source(nullptr, f.name, SD_DRILLS, src) || !_parseDrill(f.name, nullptr, src, info)) {
                skipped++; out += "X\tD\t" + meta(f) + "\n"; continue;
            }
            _putDrill(info);
            out += "D\t" + meta(f) + '\t' + _clean(info.id) + '\t' + _clean(info.title) + '\t' + _clean(info.faction) + '\t' +
                   _clean(info.demon) + '\t' + std::to_string(info.rounds) + '\t' + _joinList(info.wards) + "\n";
        }
        for (auto& kv : cache) if (!kv.second.used) dirty = true;   // a file went away
        if (dirty) {
            File f = SD.open(SD_INDEX, FILE_WRITE);
            if (f) { f.write((const uint8_t*)out.data(), out.size()); f.close(); }
        }
    }
};

}  // namespace study
