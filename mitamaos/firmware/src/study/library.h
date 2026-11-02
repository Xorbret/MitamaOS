#pragma once
// What Shugyo and Harae share: the list of lessons and drills (built into the
// firmware, plus any on the SD card in /MitamaOS/study/), the player's
// progress, and the hardware lessons may use.
//
// Progress lives in NVS "study" (so it works without a card) and is mirrored
// to settings.conf by SdSettings, so updates keep it. A passed lesson task is
// the Ward that lesson teaches.
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <algorithm>
#include <string>
#include <vector>
#include "builtin_content.h"
#include "game.h"
#include "lesson.h"
#include "mitama_hal.h"
#include "progress.h"
#include "../system/sd_settings.h"

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
        _scanSd(SD_LESSONS, true);
        _scanSd(SD_DRILLS, false);
        std::stable_sort(lessons.begin(), lessons.end(), [](const LessonInfo& a, const LessonInfo& b) {
            return a.track != b.track ? a.track < b.track : a.file < b.file;
        });
        std::stable_sort(drills.begin(), drills.end(), [](const DrillInfo& a, const DrillInfo& b) { return a.file < b.file; });
    }
    void rescan() { _built = false; ensure(); }

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
        // One record per ';' (settings.conf is one key per line).
        std::string s = progress.serialize();
        std::replace(s.begin(), s.end(), '\n', ';');
        if (s.size() > 3800) s.resize(s.rfind(';', 3800) + 1);   // NVS string limit
        Preferences p; p.begin("study", false); p.putString("p", s.c_str()); p.end();
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
        String s = p.getString("p", "");
        p.end();
        std::string t = s.c_str();
        std::replace(t.begin(), t.end(), ';', '\n');
        progress.parse(t);
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
    void _addLesson(const std::string& file, const char* builtin, const std::string& sdSrc) {
        Lesson l(&hal);
        std::string err;
        if (!l.load(builtin ? std::string(builtin) : sdSrc, err)) { skipped++; return; }
        LessonInfo info{ l.id(), l.title(), file, l.track(), l.hasTask(), builtin,
                         l.prereqs(), l.card(), l.tag(), !l.field().empty() };
        for (auto& e : lessons) if (e.id == info.id) { e = info; return; }
        lessons.push_back(info);
    }

    void _addDrill(const std::string& file, const char* builtin, const std::string& sdSrc) {
        Scenario s;
        std::string err;
        if (!loadScenario(&hal, builtin ? std::string(builtin) : sdSrc, s, err)) { skipped++; return; }
        DrillInfo info{ s.id, s.title, s.faction, s.demon, file, (int)s.rounds.size(), builtin, {} };
        for (auto& r : s.rounds)
            for (auto& d : r.defenses)
                if (!d.ward.empty() && std::find(info.wards.begin(), info.wards.end(), d.ward) == info.wards.end())
                    info.wards.push_back(d.ward);
        for (auto& e : drills) if (e.id == info.id) { e = info; return; }
        drills.push_back(info);
    }

    void _scanSd(const char* dir, bool lessonsDir) {
        if (SD.cardType() == CARD_NONE) return;
        File d = SD.open(dir);
        if (!d || !d.isDirectory()) return;
        std::vector<std::string> names;
        for (File f = d.openNextFile(); f; f = d.openNextFile()) {
            std::string n = f.name();
            n = n.substr(n.find_last_of('/') + 1);
            if (!f.isDirectory() && n.size() > 4 && n.compare(n.size() - 4, 4, ".lua") == 0) names.push_back(n);
            f.close();
            if (names.size() >= 64) break;
        }
        d.close();
        std::sort(names.begin(), names.end());
        for (auto& n : names) {
            std::string src;
            if (!_source(nullptr, n, dir, src)) { skipped++; continue; }
            if (lessonsDir) _addLesson(n, nullptr, src); else _addDrill(n, nullptr, src);
        }
    }
};

}  // namespace study
