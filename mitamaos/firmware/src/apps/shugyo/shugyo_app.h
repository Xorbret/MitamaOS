#pragma once
// Shugyo (修行, "practice"): lessons on the device. Each lesson is a few pages
// to read, a Lua task the device grades, a short quiz and a reflection.
// Lessons run in CardLab's sandbox (study/sandbox.h): only the hardware the
// lesson asks for, a memory cap, a time limit, no files outside
// /MitamaOS/user/Study and nothing that transmits.
//
// A passed task is also a Ward in Harae, the game: the matching defence
// becomes playable there. Passing pays Shugyo ki (daily limit in ki.h).
//
// Screens follow the approved mockups: home, lessons, reader, code task,
// check result, quiz, records. Harae has its own tile; Shugyo only shows the
// Wards a lesson earned for it.
#include <M5Cardputer.h>
#include <memory>
#include "../../os/app.h"
#include "../../os/app_manager.h"
#include "../../os/os_ui.h"
#include "../../config.h"
#include "../../ui/mitama_ui.h"
#include "../../ui/mitama_forms.h"
#include "../../mitama/ki.h"
#include "../../study/library.h"
#include "study_ui.h"

class ShugyoApp : public App {
public:
    ShugyoApp(LGFX_Sprite& canvas) : _c(canvas) {}

    const char* getName() const override { return "Shugyo"; }
    const char* getIcon() const override { return "S"; }
    const char* getDesc() const override { return "Lessons and drills"; }

    void onStart() override {
        OsUI::waitRelease();
        study::Library::get().ensure();
        _scr = S_HOME;
        _homeSel = 0;
        _quip = (uint8_t)(esp_random() % QUIPS);
        _draw();
    }
    void onStop() override { _lesson.reset(); _lines.clear(); _lines.shrink_to_fit(); }

    void update() override {
        M5Cardputer.update();
        auto& kb = M5Cardputer.Keyboard;
        if (_scr == S_TASK) { _updateTask(); return; }
        if (!(kb.isChange() && kb.isPressed())) {
            if (_scr == S_HOME && millis() - _spinMs > 60) { _spinMs = millis(); _spin += 2.5f; _draw(); }
            return;
        }
        auto st = kb.keysState();
        char k = st.word.empty() ? 0 : st.word.back();
        bool back = k == '`', up = k == ';', down = k == '.', left = k == ',', right = k == '/';
        switch (_scr) {
            case S_HOME:    _keysHome(back, up, down, st.enter); break;
            case S_LIST:    _keysList(back, up, down, st.enter); break;
            case S_READ:    _keysRead(back, up, down, left, right, k, st.enter); break;
            case S_RESULT:  _keysResult(back, k, st.enter); break;
            case S_QUIZ:    _keysQuiz(back, up, down, k, st.enter); break;
            case S_REFLECT: if (back || st.enter) _toList(); break;
            case S_RECORDS: if (back || st.enter) { _scr = S_HOME; } break;
            default: break;
        }
        if (_exit) { _exit = false; OsUI::waitRelease(); AppManager::get().goHome(); return; }
        if (_scr != S_TASK) _draw();
    }

private:
    enum Screen : uint8_t { S_HOME, S_LIST, S_READ, S_TASK, S_RESULT, S_QUIZ, S_REFLECT, S_RECORDS };
    static constexpr int QUIPS = 6;
    static constexpr int KI_PASS = 30, KI_REPASS = 10, KI_QUIZ = 5;

    LGFX_Sprite& _c;
    Screen  _scr = S_HOME;
    int     _homeSel = 0, _listSel = 0, _listTop = 0;
    uint8_t _quip = 0;
    bool    _exit = false;
    float   _spin = 0;
    uint32_t _spinMs = 0;

    // the open lesson
    std::unique_ptr<study::Lesson> _lesson;
    int _page = 0, _pageScroll = 0;

    // code task
    std::vector<String> _lines;
    int _row = 0, _col = 0, _top = 0, _left = 0;
    String _con1, _con2;
    uint16_t _con1Col = MUI::DIM, _con2Col = MUI::DIM;
    OsUI::Typing _typing;
    bool _passed = false, _firstPass = false;
    String _feedback;
    uint32_t _kiPaid = 0;

    // quiz
    int _q = 0, _qSel = 0, _qRight = 0;
    bool _qAnswered = false, _qWasRight = false;

    study::Library& _lib() { return study::Library::get(); }

    // ---- home --------------------------------------------------------
    void _keysHome(bool back, bool up, bool down, bool enter) {
        if (back) { _exit = true; return; }
        if (up) _homeSel = max(0, _homeSel - 1);
        if (down) _homeSel = min(1, _homeSel + 1);
        if (enter) {
            if (_homeSel == 0) { _scr = S_LIST; _pickNext(); }
            else _scr = S_RECORDS;
        }
    }

    static const char* _quipText(uint8_t i) {
        static const char* const Q[QUIPS] = {
            "\"Practice now. Complain later. That's the deal.\"",
            "\"Every master was a beginner who refused to quit. Annoyingly.\"",
            "\"Read the page. Yes, all of it. I'll know.\"",
            "\"Errors are just the machine being honest with you.\"",
            "\"One lesson a day keeps the demons at bay. Roughly.\"",
            "\"The sandbox can't hurt you. Your pride, maybe.\"",
        };
        return Q[i % QUIPS];
    }

    void _drawHome() {
        auto& ki = Ki::get();
        char right[32];
        snprintf(right, sizeof(right), "Lv %d  %s", ki.level(), Ki::rankName(ki.rank()));
        StudyUI::head(_c, "SHUGYO", right);

        _c.setClipRect(176, HEADER_H, SCREEN_W - 176, 70);
        MUI::soulForm(_c, 206, 44, 40, MUI::YELLOW, MUI::GREEN, _spin, Forms::get().current(), ki.level());
        _c.clearClipRect();
        char t[24];
        snprintf(t, sizeof(t), "+%u ki today", (unsigned)ki.today(Ki::S_SHUGYO));
        MUI::textCentered(_c, MUI::small(), t, 206, 70, MUI::GREEN);
        snprintf(t, sizeof(t), "limit %u", (unsigned)ki.cap(Ki::S_SHUGYO));
        MUI::textCentered(_c, MUI::small(), t, 206, 82, MUI::DIM);

        auto& lib = _lib();
        char a[32], b[40];
        snprintf(a, sizeof(a), "%d of %d passed", (int)lib.progress.lessonsPassed(), (int)lib.lessons.size());
        snprintf(b, sizeof(b), "%d Harae Ward%s, scores, streak", lib.wardsHeld(), lib.wardsHeld() == 1 ? "" : "s");
        const char* names[2] = { "Lessons", "Records" };
        const char* subs[2] = { a, b };
        for (int i = 0; i < 2; i++) {
            int y = StudyUI::TOP + 6 + i * 32;
            bool sel = i == _homeSel;
            if (sel) StudyUI::bar(_c, 3, y - 3, 170, 27);
            MUI::text(_c, MUI::big(), names[i], 9, y - 1, sel ? MUI::WHITE : MUI::LABEL);
            MUI::text(_c, MUI::small(), subs[i], 9, y + 12, sel ? MUI::YELLOW : MUI::DIM);
        }
        StudyUI::rule(_c, 102);
        StudyUI::wrap(_c, MUI::small(), _quipText(_quip), 6, 105, 228, 11, 1, MUI::CREAM);
        StudyUI::foot(_c, "; . move   ENT open   ` back");
    }

    // ---- lesson list -------------------------------------------------
    // Rows are track headers and lessons; only lessons can be selected.
    struct Row { int lesson; int track; };
    std::vector<Row> _rows() {
        std::vector<Row> r;
        int track = -1;
        auto& L = _lib().lessons;
        for (int i = 0; i < (int)L.size(); i++) {
            if (L[i].track != track) { track = L[i].track; r.push_back({ -1, track }); }
            r.push_back({ i, track });
        }
        return r;
    }
    static const char* _trackName(int t) {
        static const char* const N[] = { "FOUNDATIONS", "HARDWARE", "CONNECT", "TESTING", "RECON" };
        return t >= 0 && t < 5 ? N[t] : "MORE";
    }
    int _nextLesson() {
        auto& L = _lib().lessons;
        for (int i = 0; i < (int)L.size(); i++) if (!_lib().progress.hasWard(L[i].id)) return i;
        return -1;
    }
    void _pickNext() {
        int n = _nextLesson();
        auto rows = _rows();
        _listSel = 1;
        for (int i = 0; i < (int)rows.size(); i++) if (rows[i].lesson == (n < 0 ? 0 : n)) { _listSel = i; break; }
        _scrollList(rows.size());
    }
    void _scrollList(size_t n) {
        const int vis = 8;
        if (_listSel < _listTop + 1) _listTop = max(0, _listSel - 1);
        if (_listSel >= _listTop + vis) _listTop = _listSel - vis + 1;
        _listTop = constrain(_listTop, 0, max(0, (int)n - vis));
    }
    void _keysList(bool back, bool up, bool down, bool enter) {
        auto rows = _rows();
        if (back) { _scr = S_HOME; return; }
        if (rows.empty()) return;
        if (up || down) {
            int d = up ? -1 : 1, i = _listSel + d;
            while (i >= 0 && i < (int)rows.size() && rows[i].lesson < 0) i += d;
            if (i >= 0 && i < (int)rows.size()) _listSel = i;
            _scrollList(rows.size());
        }
        if (enter && _listSel < (int)rows.size() && rows[_listSel].lesson >= 0) _open(rows[_listSel].lesson);
    }
    void _toList() { _lesson.reset(); _scr = S_LIST; _pickNext(); }

    void _drawList() {
        auto& lib = _lib();
        char r[16];
        snprintf(r, sizeof(r), "%d/%d", (int)lib.progress.lessonsPassed(), (int)lib.lessons.size());
        StudyUI::head(_c, "SHUGYO  -  LESSONS", r);
        auto rows = _rows();
        int next = _nextLesson();
        if (rows.empty()) {
            MUI::textCentered(_c, MUI::small(), "No lessons found", SCREEN_W / 2, 60, MUI::DIM);
        }
        for (int i = _listTop; i < (int)rows.size() && i < _listTop + 8; i++) {
            int y = StudyUI::TOP + (i - _listTop) * 12 + 1;
            const Row& row = rows[i];
            if (row.lesson < 0) {
                char tn[4]; snprintf(tn, sizeof(tn), "T%d", row.track);
                MUI::text(_c, MUI::small(), tn, 5, y, MUI::CYAN);
                MUI::text(_c, MUI::small(), _trackName(row.track), 24, y, MUI::CYAN);
                continue;
            }
            auto& L = lib.lessons[row.lesson];
            bool sel = i == _listSel, done = lib.progress.hasWard(L.id), isNext = row.lesson == next;
            if (sel) StudyUI::bar(_c, 2, y - 1, 236, 12);
            uint16_t col = sel ? MUI::WHITE : done ? MUI::GREEN : MUI::LABEL;
            _c.setClipRect(24, y - 1, 160, 12);
            MUI::text(_c, MUI::small(), L.title.c_str(), 24, y, col);
            _c.clearClipRect();
            const char* tag = done ? "PASSED" : isNext ? "NEXT" : "";
            if (*tag) StudyUI::textRight(_c, MUI::small(), tag, 234, y, done ? MUI::GREEN : MUI::YELLOW);
        }
        if (lib.skipped) {
            char s[40]; snprintf(s, sizeof(s), "%d file%s skipped (errors)", lib.skipped, lib.skipped == 1 ? "" : "s");
            StudyUI::foot(_c, "; . move   ENT open   ` back", s);
        } else {
            StudyUI::foot(_c, "; . move   ENT open   ` back");
        }
    }

    // ---- reader ------------------------------------------------------
    void _open(int idx) {
        auto& lib = _lib();
        auto& info = lib.lessons[idx];
        std::string src, err;
        _lesson.reset(new study::Lesson(&lib.hal));
        if (!lib.source(info, src) || !_lesson->load(src, err)) {
            _lesson.reset();
            OsUI::showToast(_c, "Lesson won't load");
            return;
        }
        _page = 0; _pageScroll = 0;
        _lines.clear();
        _scr = S_READ;
    }
    int _readerPages() const { return (int)_lesson->pages().size(); }

    void _keysRead(bool back, bool up, bool down, bool left, bool right, char k, bool enter) {
        if (back) { _toList(); return; }
        if (left && _page > 0) { _page--; _pageScroll = 0; }
        if ((right || enter) && _page < _readerPages() - 1) { _page++; _pageScroll = 0; }
        else if (enter) _startTaskOrQuiz();
        if (up && _pageScroll > 0) _pageScroll--;
        if (down) _pageScroll++;
        if (k == 't' || k == 'T') _startTaskOrQuiz();
        if ((k == 'q' || k == 'Q') && !_lesson->quiz().empty()) _startQuiz();
    }
    void _startTaskOrQuiz() {
        if (_lesson->hasTask()) _startTask();
        else if (!_lesson->quiz().empty()) _startQuiz();
        else _scr = S_REFLECT;
    }

    void _drawRead() {
        char r[12];
        snprintf(r, sizeof(r), "%d/%d", _page + 1, _readerPages());
        String title = _lesson->title().c_str(); title.toUpperCase();
        StudyUI::head(_c, title.c_str(), r);
        const int lh = 12, vis = 8;
        const char* text = _readerPages() ? _lesson->pages()[_page].c_str() : "";
        int total = StudyUI::wrap(_c, MUI::small(), text, 5, 0, 228, lh, 0, 0);
        _pageScroll = constrain(_pageScroll, 0, max(0, total - vis));
        StudyUI::wrap(_c, MUI::small(), text, 5, StudyUI::TOP + 1, 228, lh, vis, MUI::LABEL, _pageScroll);
        if (total > vis) {   // scroll mark
            int h = max(6, 96 * vis / total), y = StudyUI::TOP + (96 - h) * _pageScroll / max(1, total - vis);
            _c.fillRect(SCREEN_W - 3, y, 2, h, MUI::BONES_DIM);
        }
        bool last = _page >= _readerPages() - 1;
        StudyUI::foot(_c, last ? (_lesson->hasTask() ? ", page   ENT try it   ` back" : ", page   ENT quiz   ` back")
                               : ", / page   T try it   ` back");
    }

    // ---- code task ---------------------------------------------------
    void _startTask() {
        _lines.clear();
        String s = _lesson->starter().c_str();
        int from = 0;
        while (from <= (int)s.length()) {
            int nl = s.indexOf('\n', from);
            if (nl < 0) nl = s.length();
            _lines.push_back(s.substring(from, nl));
            from = nl + 1;
        }
        if (_lines.empty()) _lines.push_back("");
        _row = (int)_lines.size() - 1; _col = _lines[_row].length(); _top = 0; _left = 0;
        _con1 = _lesson->task().c_str(); _con1Col = MUI::CREAM;
        _con2 = "fn+R run   fn+C check"; _con2Col = MUI::DIM;
        _typing = OsUI::Typing();
        _scr = S_TASK;
        OsUI::waitRelease();
        _drawTask();
    }

    String _code() const {
        String s;
        for (size_t i = 0; i < _lines.size(); i++) { if (i) s += '\n'; s += _lines[i]; }
        return s;
    }

    void _updateTask() {
        auto& kb = M5Cardputer.Keyboard;
        auto st = kb.keysState();
        String added; bool del = false, ent = false;
        _typing.poll(st, added, del, ent);
        if (!added.length() && !del && !ent) return;

        if (st.fn) {
            for (size_t i = 0; i < added.length(); i++) {
                char ch = added[i];
                if (ch == ';' && _row > 0) { _row--; _col = min(_col, (int)_lines[_row].length()); }
                else if (ch == '.' && _row < (int)_lines.size() - 1) { _row++; _col = min(_col, (int)_lines[_row].length()); }
                else if (ch == ',') { if (_col > 0) _col--; else if (_row > 0) { _row--; _col = _lines[_row].length(); } }
                else if (ch == '/') { if (_col < (int)_lines[_row].length()) _col++; else if (_row < (int)_lines.size() - 1) { _row++; _col = 0; } }
                else if (ch == 'r' || ch == 'R') { _run(false); return; }
                else if (ch == 'c' || ch == 'C') { _run(true); return; }
                else if (ch == 'h' || ch == 'H') { _hint(); }
                else if (ch == 'k' || ch == 'K') { _lines[_row] = ""; _col = 0; }
            }
            _drawTask();
            return;
        }
        for (size_t i = 0; i < added.length(); i++) {
            char ch = added[i];
            if (ch == '`') { OsUI::waitRelease(); _scr = S_READ; _draw(); return; }
            if (_lines[_row].length() < 120) {
                _lines[_row] = _lines[_row].substring(0, _col) + ch + _lines[_row].substring(_col);
                _col++;
            }
        }
        if (st.tab && _lines[_row].length() < 118) { _lines[_row] = _lines[_row].substring(0, _col) + "  " + _lines[_row].substring(_col); _col += 2; }
        if (del) {
            if (_col > 0) { _lines[_row].remove(_col - 1, 1); _col--; }
            else if (_row > 0) {
                _col = _lines[_row - 1].length();
                _lines[_row - 1] += _lines[_row];
                _lines.erase(_lines.begin() + _row);
                _row--;
            }
        }
        if (ent && _lines.size() < 80) {   // new line, keeping the indent
            String rest = _lines[_row].substring(_col);
            _lines[_row] = _lines[_row].substring(0, _col);
            int ind = 0;
            while (ind < (int)_lines[_row].length() && _lines[_row][ind] == ' ') ind++;
            String pad; for (int i = 0; i < ind; i++) pad += ' ';
            _lines.insert(_lines.begin() + _row + 1, pad + rest);
            _row++; _col = ind;
        }
        _drawTask();
    }

    void _hint() {
        const std::string& h = _lesson->hint();
        _con1 = h.empty() ? "No hint for this one. Re-read the last page." : h.c_str();
        _con1Col = MUI::CREAM; _con2 = "";
    }

    static void _splitOut(const std::string& out, String& a, String& b) {
        // the last two lines of output
        String s = out.c_str();
        s.trim();
        int nl = s.lastIndexOf('\n');
        if (nl < 0) { a = s; b = ""; return; }
        b = s.substring(nl + 1);
        String head = s.substring(0, nl);
        int nl2 = head.lastIndexOf('\n');
        a = nl2 < 0 ? head : head.substring(nl2 + 1);
    }

    void _run(bool check) {
        _con1 = check ? "Checking..." : "Running..."; _con1Col = MUI::YELLOW; _con2 = "";
        _drawTask();
        auto& lib = _lib();
        lib.hal.screen.clear();
        study::GradeResult g = _lesson->grade(_code().c_str());
        std::string shown = g.run.output + lib.hal.screen;
        if (!check) {
            if (!g.run.ok) {
                _con1 = "error"; _con1Col = MUI::RED;
                _con2 = g.run.error.c_str(); _con2Col = MUI::RED;
            } else if (shown.empty()) {
                _con1 = "(ran, printed nothing)"; _con1Col = MUI::DIM; _con2 = "";
            } else {
                String a, b; _splitOut(shown, a, b);
                _con1 = "> " + a; _con1Col = MUI::GREEN;
                _con2 = b.length() ? "> " + b : ""; _con2Col = MUI::GREEN;
            }
            OsUI::waitRelease();
            _drawTask();
            return;
        }
        _passed = g.passed;
        _feedback = g.feedback.c_str();
        _kiPaid = 0;
        if (g.passed) {
            _firstPass = !lib.progress.hasWard(_lesson->id());
            lib.progress.recordTask(_lesson->id(), true);
            lib.save();
            _kiPaid = Ki::get().add(Ki::S_SHUGYO, _firstPass ? KI_PASS : KI_REPASS);
        } else {
            lib.progress.recordTask(_lesson->id(), false);
        }
        OsUI::waitRelease();
        _scr = S_RESULT;
        _draw();
    }

    void _drawTask() {
        _c.fillSprite(MUI::BG);
        String t = "TASK  -  "; String ti = _lesson->title().c_str(); ti.toUpperCase(); t += ti;
        char pos[12]; snprintf(pos, sizeof(pos), "%d:%d", _row + 1, _col + 1);
        StudyUI::head(_c, t.c_str(), pos);
        // editor: the built-in 6x8 font, so columns line up
        const int ey = HEADER_H + 2, lh = 10, vis = 8, gx = 16, cw = 6;
        const int cols = (SCREEN_W - gx - 2) / cw;
        if (_row < _top) _top = _row;
        if (_row >= _top + vis) _top = _row - vis + 1;
        if (_col < _left) _left = _col;
        if (_col >= _left + cols) _left = _col - cols + 1;
        _c.setFont(&lgfx::fonts::Font0); _c.setTextSize(1); _c.setTextDatum(top_left);
        for (int i = _top; i < (int)_lines.size() && i < _top + vis; i++) {
            int y = ey + (i - _top) * lh;
            char n[4]; snprintf(n, sizeof(n), "%2d", i + 1);
            _c.setTextColor(i == _row ? MUI::YELLOW_DIM : MUI::BONES_DIM);
            _c.drawString(n, 2, y + 1);
            const String& L = _lines[i];
            String vis_ = (int)L.length() > _left ? L.substring(_left, _left + cols) : "";
            String trimmed = L; trimmed.trim();
            _c.setTextColor(trimmed.startsWith("--") ? MUI::DIM : i == _row ? MUI::WHITE : MUI::LABEL);
            _c.drawString(vis_.c_str(), gx, y + 1);
        }
        int cx = gx + (_col - _left) * cw, cy = ey + (_row - _top) * lh;
        _c.fillRect(cx, cy, 1, 9, MUI::YELLOW);
        // console
        _c.fillRect(0, 98, SCREEN_W, 1, MUI::BONES_DIM);
        _c.setClipRect(0, 99, SCREEN_W, 22);
        MUI::text(_c, MUI::small(), _con1.c_str(), 4, 99, _con1Col);
        MUI::text(_c, MUI::small(), _con2.c_str(), 4, 109, _con2Col);
        _c.clearClipRect();
        StudyUI::foot(_c, "fn+R run  fn+C check  fn+H hint", "` page");
        StudyUI::push(_c);
    }

    // ---- check result ------------------------------------------------
    void _keysResult(bool back, char k, bool enter) {
        if (back) { _scr = S_READ; return; }
        if (k == 'r' || k == 'R' || (!_passed && enter)) { _scr = S_TASK; _typing = OsUI::Typing(); OsUI::waitRelease(); _drawTask(); return; }
        if (enter) { if (!_lesson->quiz().empty()) _startQuiz(); else _scr = S_REFLECT; }
    }

    void _drawResult() {
        String t = "TASK  -  "; String ti = _lesson->title().c_str(); ti.toUpperCase(); t += ti;
        StudyUI::head(_c, t.c_str(), _passed ? "PASSED" : "NOT YET", _passed ? MUI::GREEN : MUI::BONES);
        uint16_t col = _passed ? MUI::GREEN : MUI::BONES;
        MUI::enso(_c, 40, 64, 22, col);
        MUI::text(_c, MUI::display(), _passed ? "Check passed" : "Not yet", 80, 26, col);
        int lines = StudyUI::wrap(_c, MUI::small(), _feedback.c_str(), 80, 52, 152, 12, 3, MUI::LABEL);
        int y = 52 + min(lines, 3) * 12 + 4;
        if (_passed) {
            char k[16], d[32];
            snprintf(k, sizeof(k), "+%u ki", (unsigned)_kiPaid);
            snprintf(d, sizeof(d), "%u of %u today", (unsigned)Ki::get().today(Ki::S_SHUGYO), (unsigned)Ki::get().cap(Ki::S_SHUGYO));
            int x = MUI::text(_c, MUI::big(), k, 80, y, MUI::YELLOW);
            MUI::text(_c, MUI::small(), d, x + 6, y + 2, MUI::DIM);
            if (_firstPass) MUI::text(_c, MUI::small(), "Ward earned for Harae", 80, y + 14, MUI::CYAN);
        }
        StudyUI::foot(_c, _passed ? (_lesson->quiz().empty() ? "ENT reflect   R retry   ` back" : "ENT quiz   R retry   ` back")
                                  : "ENT try again   ` back");
    }

    // ---- quiz --------------------------------------------------------
    void _startQuiz() { _q = 0; _qSel = 0; _qRight = 0; _qAnswered = false; _scr = S_QUIZ; }

    void _keysQuiz(bool back, bool up, bool down, char k, bool enter) {
        auto& Q = _lesson->quiz();
        if (back) { _scr = S_READ; return; }
        if (_q >= (int)Q.size()) { if (enter) _scr = S_REFLECT; return; }
        auto& item = Q[_q];
        if (_qAnswered) {
            if (enter) { _q++; _qSel = 0; _qAnswered = false; if (_q >= (int)Q.size()) _finishQuiz(); }
            return;
        }
        int n = (int)item.choices.size();
        if (up) _qSel = max(0, _qSel - 1);
        if (down) _qSel = min(n - 1, _qSel + 1);
        if (k >= '1' && k <= '9' && k - '1' < n) { _qSel = k - '1'; enter = true; }
        if (enter) {
            _qAnswered = true;
            _qWasRight = _qSel == item.answer;
            if (_qWasRight) _qRight++;
        }
    }
    void _finishQuiz() {
        auto& lib = _lib();
        const study::LessonRecord* before = lib.progress.lesson(_lesson->id());
        bool firstPerfect = !(before && before->quizTotal && before->quizScore == before->quizTotal);
        int total = (int)_lesson->quiz().size();
        lib.progress.recordQuiz(_lesson->id(), _qRight, total);
        lib.save();
        _kiPaid = (firstPerfect && _qRight == total) ? Ki::get().add(Ki::S_SHUGYO, KI_QUIZ * total) : 0;
    }

    void _drawQuiz() {
        auto& Q = _lesson->quiz();
        String t = "QUIZ  -  "; String ti = _lesson->title().c_str(); ti.toUpperCase(); t += ti;
        char r[12]; snprintf(r, sizeof(r), "%d/%d", min(_q + 1, (int)Q.size()), (int)Q.size());
        StudyUI::head(_c, t.c_str(), r);
        if (_q >= (int)Q.size()) {
            char s[32]; snprintf(s, sizeof(s), "%d of %d right", _qRight, (int)Q.size());
            MUI::text(_c, MUI::display(), s, 8, 30, _qRight == (int)Q.size() ? MUI::GREEN : MUI::YELLOW);
            if (_kiPaid) { char k[16]; snprintf(k, sizeof(k), "+%u ki", (unsigned)_kiPaid); MUI::text(_c, MUI::big(), k, 8, 58, MUI::YELLOW); }
            else MUI::text(_c, MUI::small(), "Ki pays once, for the first perfect quiz.", 8, 60, MUI::DIM);
            StudyUI::foot(_c, "ENT reflect   ` back");
            return;
        }
        auto& item = Q[_q];
        int ql = StudyUI::wrap(_c, MUI::big(), item.question.c_str(), 5, StudyUI::TOP, 230, 14, 2, MUI::WHITE);
        int y0 = StudyUI::TOP + min(ql, 2) * 14 + 6;
        if (_qAnswered) {
            uint16_t col = _qWasRight ? MUI::GREEN : MUI::BONES;
            MUI::text(_c, MUI::big(), _qWasRight ? "Right." : "Not quite.", 5, y0, col);
            if (!_qWasRight) {
                String a = "Answer: "; a += item.choices[item.answer].c_str();
                MUI::text(_c, MUI::small(), a.c_str(), 80, y0 + 2, MUI::LABEL);
            }
            StudyUI::wrap(_c, MUI::small(), item.why.c_str(), 5, y0 + 18, 230, 12, 4, MUI::CREAM);
            StudyUI::foot(_c, "ENT next");
            return;
        }
        for (int i = 0; i < (int)item.choices.size() && i < 5; i++) {
            int y = y0 + i * 15;
            bool sel = i == _qSel;
            if (sel) StudyUI::bar(_c, 3, y - 2, 234, 14);
            char n[3]; snprintf(n, sizeof(n), "%d", i + 1);
            MUI::text(_c, MUI::small(), n, 9, y, sel ? MUI::YELLOW : MUI::DIM);
            _c.setClipRect(22, y - 2, 212, 14);
            MUI::text(_c, MUI::small(), item.choices[i].c_str(), 22, y, sel ? MUI::WHITE : MUI::LABEL);
            _c.clearClipRect();
        }
        StudyUI::foot(_c, "1-4 or ; . pick   ENT answer");
    }

    void _drawReflect() {
        StudyUI::head(_c, "REFLECT", "");
        const char* r = _lesson->reflect().empty() ? "Think about where you'd use this next." : _lesson->reflect().c_str();
        StudyUI::wrap(_c, MUI::big(), r, 5, StudyUI::TOP + 2, 230, 15, 5, MUI::CREAM);
        MUI::text(_c, MUI::small(), "No grade. Say it out loud, or write it in Grimoire.", 5, 104, MUI::DIM);
        StudyUI::foot(_c, "ENT done");
    }

    // ---- records -----------------------------------------------------
    void _drawRecords() {
        auto& lib = _lib();
        StudyUI::head(_c, "SHUGYO  -  RECORDS", "");
        int y = StudyUI::TOP;
        MUI::text(_c, MUI::small(), "HARAE DRILLS", 5, y, MUI::CYAN); y += 12;
        int shown = 0;
        for (auto& d : lib.drills) {
            if (shown >= 3) break;
            int best = lib.progress.bestScenarioScore(d.id);
            _c.setClipRect(5, y, 150, 12);
            MUI::text(_c, MUI::small(), d.title.c_str(), 5, y, MUI::LABEL);
            _c.clearClipRect();
            char b[12]; snprintf(b, sizeof(b), "%d", best);
            StudyUI::textRight(_c, MUI::small(), best < 0 ? "-" : b, 234, y, best < 0 ? MUI::DIM : MUI::WHITE);
            y += 12; shown++;
        }
        StudyUI::rule(_c, y + 1); y += 4;
        int passed = (int)lib.progress.lessonsPassed(), total = (int)lib.lessons.size();
        char s[32];
        snprintf(s, sizeof(s), "%d of %d passed", passed, total);
        MUI::text(_c, MUI::small(), "Lessons", 5, y, MUI::LABEL);
        MUI::text(_c, MUI::small(), s, 60, y, MUI::WHITE);
        StudyUI::meter(_c, 150, y + 3, 84, total ? passed / (float)total : 0, MUI::GREEN);
        y += 12;
        int qr = 0, qt = 0;
        for (auto& l : lib.lessons) { auto* r = lib.progress.lesson(l.id); if (r) { qr += r->quizScore; qt += r->quizTotal; } }
        snprintf(s, sizeof(s), "%d of %d right", qr, qt);
        MUI::text(_c, MUI::small(), "Quizzes", 5, y, MUI::LABEL);
        StudyUI::textRight(_c, MUI::small(), s, 234, y, MUI::WHITE); y += 12;
        snprintf(s, sizeof(s), "%u today", (unsigned)Ki::get().today(Ki::S_SHUGYO));
        MUI::text(_c, MUI::small(), "Shugyo ki", 5, y, MUI::LABEL);
        StudyUI::textRight(_c, MUI::small(), s, 234, y, MUI::YELLOW); y += 12;
        snprintf(s, sizeof(s), "%u day%s", (unsigned)Ki::get().streak(), Ki::get().streak() == 1 ? "" : "s");
        MUI::text(_c, MUI::small(), "Practice streak", 5, y, MUI::LABEL);
        StudyUI::textRight(_c, MUI::small(), s, 234, y, MUI::WHITE);
        StudyUI::foot(_c, "` back");
    }

    void _draw() {
        _c.fillSprite(MUI::BG);
        switch (_scr) {
            case S_HOME:    _drawHome(); break;
            case S_LIST:    _drawList(); break;
            case S_READ:    _drawRead(); break;
            case S_TASK:    _drawTask(); return;
            case S_RESULT:  _drawResult(); break;
            case S_QUIZ:    _drawQuiz(); break;
            case S_REFLECT: _drawReflect(); break;
            case S_RECORDS: _drawRecords(); break;
        }
        StudyUI::push(_c);
    }
};
