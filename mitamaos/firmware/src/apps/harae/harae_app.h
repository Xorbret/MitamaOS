#pragma once
// Harae (祓, "purification"): the game. Demons have hexed the aether over
// your domain; each drill is one of their attacks, played turn by turn.
// Read the symptom, spend action points on evidence, then answer with a
// defence. Everything is simulated on screen and nothing is transmitted.
//
// Phase 1 is the Domain: drills grouped by the faction that casts them.
// Phase 2, the Deep Aether (the roguelike post-game), opens once the Domain
// is cleared; it is a sealed door for now.
//
// Fair by construction (study/game.h): every round can be stopped, and a
// drill that needs a Ward you haven't earned is sealed, naming the Shugyo
// lesson that teaches it, instead of being offered as an unwinnable board.
// Responses that need an unearned Ward are left out of the choices.
#include <M5Cardputer.h>
#include <memory>
#include "../../os/app.h"
#include "../../os/app_manager.h"
#include "../../os/os_ui.h"
#include "../../config.h"
#include "../../ui/mitama_ui.h"
#include "../../mitama/ki.h"
#include "../../study/library.h"
#include "../shugyo/study_ui.h"

class HaraeApp : public App {
public:
    HaraeApp(LGFX_Sprite& canvas) : _c(canvas) {}

    const char* getName() const override { return "Harae"; }
    const char* getIcon() const override { return "H"; }
    const char* getDesc() const override { return "Break the demons' hexes"; }

    void onStart() override {
        OsUI::waitRelease();
        study::Library::get().ensure();
        _scr = H_HOME;
        _homeSel = 0;
        _draw();
    }
    void onStop() override { _game.reset(); }

    void update() override {
        M5Cardputer.update();
        auto& kb = M5Cardputer.Keyboard;
        if (!(kb.isChange() && kb.isPressed())) return;
        auto st = kb.keysState();
        char k = st.word.empty() ? 0 : st.word.back();
        bool back = k == '`', up = k == ';', down = k == '.';
        switch (_scr) {
            case H_HOME:     _keysHome(back, up, down, st.enter); break;
            case H_DRILLS:   _keysDrills(back, up, down, st.enter); break;
            case H_SEALED:   if (back || st.enter) _scr = H_DRILLS; break;
            case H_BRIEF:    if (back) _scr = H_DRILLS; else if (st.enter) _beginRounds(); break;
            case H_ROUND:    _keysRound(back, up, down, k, st.enter); break;
            case H_EVIDENCE: if (back || st.enter) _scr = H_ROUND; break;
            case H_DEFEND:   _keysDefend(back, up, down, st.enter); break;
            case H_REVEAL:   if (st.enter) _afterReveal(); break;
            case H_DONE:     if (back || st.enter) { _game.reset(); _scr = H_DRILLS; _sealedCache.clear(); } break;
            case H_DEEP:     if (back || st.enter) _scr = H_HOME; break;
        }
        if (_exit) { _exit = false; OsUI::waitRelease(); AppManager::get().goHome(); return; }
        _draw();
    }

private:
    enum Screen : uint8_t { H_HOME, H_DRILLS, H_SEALED, H_BRIEF, H_ROUND, H_EVIDENCE, H_DEFEND, H_REVEAL, H_DONE, H_DEEP };
    static constexpr int KI_FIRST_CLEAR = 40;

    LGFX_Sprite& _c;
    Screen _scr = H_HOME;
    int _homeSel = 0, _sel = 0, _top = 0;
    bool _exit = false;

    // the drill in play
    int _drill = -1;
    std::unique_ptr<study::Game> _game;
    std::vector<std::string> _need;       // Wards a sealed drill needs
    int _evSel = 0, _evOpen = 0, _defSel = 0;
    std::vector<int> _defs;               // response indices open to the player
    int _roundScore = 0, _scoreBefore = 0;
    bool _cleared = false, _newBest = false;
    uint32_t _kiPaid = 0;
    std::vector<int8_t> _sealedCache;     // per drill: -1 unknown, 0 open, 1 sealed

    study::Library& _lib() { return study::Library::get(); }

    static uint16_t _factionColor(const std::string& f) {
        if (f == "Babel") return 0xFC80;   // ember orange
        if (f == "Static") return MUI::YELLOW;
        if (f == "Mirror Court") return MUI::CYAN;
        if (f == "Hollow Ones") return MUI::PURPLE;
        if (f == "Leeches") return MUI::MAGENTA;
        return MUI::DIM;
    }
    static const char* _factionName(const std::string& f) { return f.empty() ? "PRACTICE" : f.c_str(); }

    int _clearedCount() {
        int n = 0;
        for (auto& d : _lib().drills) n += _lib().progress.bestScenarioScore(d.id) >= 0 ? 1 : 0;
        return n;
    }

    // ---- home --------------------------------------------------------
    void _keysHome(bool back, bool up, bool down, bool enter) {
        if (back) { _exit = true; return; }
        if (up) _homeSel = 0;
        if (down) _homeSel = 1;
        if (enter) {
            if (_homeSel == 0) { _scr = H_DRILLS; _sel = 1; _fixSel(1); _sealedCache.clear(); }
            else _scr = H_DEEP;
        }
    }

    void _drawHome() {
        auto& lib = _lib();
        auto& ki = Ki::get();
        char r[32]; snprintf(r, sizeof(r), "Lv %d  Warden", ki.level());
        StudyUI::head(_c, "HARAE", r);
        MUI::sigil(_c, MUI::SIG_HARAE, 204, 42, 40, MUI::CREAM);
        char s[32];
        snprintf(s, sizeof(s), "%d Ward%s", lib.wardsHeld(), lib.wardsHeld() == 1 ? "" : "s");
        MUI::textCentered(_c, MUI::small(), s, 204, 70, MUI::CYAN);
        snprintf(s, sizeof(s), "+%u ki today", (unsigned)ki.today(Ki::S_HARAE));
        MUI::textCentered(_c, MUI::small(), s, 204, 82, MUI::GREEN);

        int cleared = _clearedCount(), total = (int)lib.drills.size();
        bool deepOpen = total > 0 && cleared >= total;
        char a[32]; snprintf(a, sizeof(a), "%d of %d hexes broken", cleared, total);
        const char* names[2] = { "The Domain", "Deep Aether" };
        const char* subs[2] = { a, deepOpen ? "the hunt beyond your walls" : "sealed until the Domain is clear" };
        for (int i = 0; i < 2; i++) {
            int y = StudyUI::TOP + 6 + i * 32;
            bool sel = i == _homeSel;
            if (sel) StudyUI::bar(_c, 3, y - 3, 170, 27);
            MUI::text(_c, MUI::big(), names[i], 9, y - 1, sel ? MUI::WHITE : (i == 1 && !deepOpen) ? MUI::DIM : MUI::LABEL);
            MUI::text(_c, MUI::small(), subs[i], 9, y + 12, sel ? MUI::YELLOW : MUI::DIM);
        }
        StudyUI::rule(_c, 102);
        StudyUI::wrap(_c, MUI::small(), "\"Something is gnawing at the edges of your network. Let's go say hello.\"", 6, 105, 228, 11, 1, MUI::CREAM);
        StudyUI::foot(_c, "; . move   ENT open   ` back");
    }

    void _drawDeep() {
        StudyUI::head(_c, "DEEP AETHER", "sealed");
        MUI::enso(_c, 40, 64, 20, MUI::BONES_DIM);
        StudyUI::wrap(_c, MUI::big(), "The door below your domain is shut.", 80, StudyUI::TOP + 6, 150, 15, 2, MUI::WHITE);
        StudyUI::wrap(_c, MUI::small(), "Break every hex in the Domain and it opens: an endless descent where you hunt demons before they reach you.",
                      80, 56, 150, 12, 4, MUI::CREAM);
        StudyUI::foot(_c, "` back");
    }

    // ---- drill list --------------------------------------------------
    struct Row { int drill; std::string faction; };
    std::vector<Row> _rows() {
        std::vector<Row> rows;
        auto& D = _lib().drills;
        // factions in the order they first appear, the Borderlands last
        std::vector<std::string> order;
        for (auto& d : D) if (!d.faction.empty() && std::find(order.begin(), order.end(), d.faction) == order.end()) order.push_back(d.faction);
        order.push_back("");
        for (auto& f : order) {
            bool any = false;
            for (int i = 0; i < (int)D.size(); i++) {
                if (D[i].faction != f) continue;
                if (!any) { rows.push_back({ -1, f }); any = true; }
                rows.push_back({ i, f });
            }
        }
        return rows;
    }
    void _fixSel(int dir) {
        auto rows = _rows();
        if (rows.empty()) { _sel = 0; return; }
        _sel = constrain(_sel, 0, (int)rows.size() - 1);
        while (_sel >= 0 && _sel < (int)rows.size() && rows[_sel].drill < 0) _sel += dir;
        if (_sel < 0 || _sel >= (int)rows.size()) { _sel = 0; while (_sel < (int)rows.size() - 1 && rows[_sel].drill < 0) _sel++; }
        const int vis = 8;
        if (_sel < _top + 1) _top = max(0, _sel - 1);
        if (_sel >= _top + vis) _top = _sel - vis + 1;
    }
    bool _sealedInfo(int drill, std::vector<std::string>& need) {
        need.clear();
        auto& d = _lib().drills[drill];
        for (auto& w : d.wards) if (!_lib().progress.hasWard(w)) { need.push_back(w); }
        if (need.empty()) return false;
        // a Ward-tagged response may not be the only way through: check properly
        study::Scenario s; std::string err;
        if (!_lib().loadDrill(d, s, err)) return false;
        need = _lib().sealedBy(s);
        return !need.empty();
    }
    void _keysDrills(bool back, bool up, bool down, bool enter) {
        if (back) { _scr = H_HOME; return; }
        if (up) { _sel--; _fixSel(-1); }
        if (down) { _sel++; _fixSel(1); }
        if (enter) {
            auto rows = _rows();
            if (_sel >= (int)rows.size() || rows[_sel].drill < 0) return;
            _drill = rows[_sel].drill;
            if (_sealedInfo(_drill, _need)) { _scr = H_SEALED; return; }
            study::Scenario s; std::string err;
            if (!_lib().loadDrill(_lib().drills[_drill], s, err)) { OsUI::showToast(_c, "Drill won't load"); return; }
            _game.reset(new study::Game(std::move(s)));
            _game->setWards([this](const std::string& w) { return _lib().progress.hasWard(w); });
            _scr = H_BRIEF;
        }
    }

    void _drawDrills() {
        auto& lib = _lib();
        char r[16]; snprintf(r, sizeof(r), "%d/%d", _clearedCount(), (int)lib.drills.size());
        StudyUI::head(_c, "HARAE  -  THE DOMAIN", r);
        auto rows = _rows();
        if (rows.empty()) MUI::textCentered(_c, MUI::small(), "No drills found", SCREEN_W / 2, 60, MUI::DIM);
        std::vector<std::string> need;
        for (int i = _top; i < (int)rows.size() && i < _top + 8; i++) {
            int y = StudyUI::TOP + (i - _top) * 12 + 1;
            auto& row = rows[i];
            if (row.drill < 0) {
                uint16_t fc = _factionColor(row.faction);
                String n = _factionName(row.faction); n.toUpperCase();
                MUI::text(_c, MUI::small(), n.c_str(), 5, y, fc);
                continue;
            }
            auto& d = lib.drills[row.drill];
            bool sel = i == _sel;
            int best = lib.progress.bestScenarioScore(d.id);
            if (_sealedCache.size() != lib.drills.size()) _sealedCache.assign(lib.drills.size(), -1);
            if (_sealedCache[row.drill] < 0) _sealedCache[row.drill] = _sealedInfo(row.drill, need) ? 1 : 0;
            bool sealed = best < 0 && _sealedCache[row.drill] == 1;
            if (sel) StudyUI::bar(_c, 2, y - 1, 236, 12);
            uint16_t col = sel ? MUI::WHITE : sealed ? MUI::DIM : best >= 0 ? MUI::GREEN : MUI::LABEL;
            _c.setClipRect(14, y - 1, 168, 12);
            MUI::text(_c, MUI::small(), d.title.c_str(), 14, y, col);
            _c.clearClipRect();
            char b[16];
            if (sealed) StudyUI::textRight(_c, MUI::small(), "SEALED", 234, y, MUI::BONES);
            else if (best >= 0) { snprintf(b, sizeof(b), "%d", best); StudyUI::textRight(_c, MUI::small(), b, 234, y, MUI::GREEN); }
            else StudyUI::textRight(_c, MUI::small(), "OPEN", 234, y, MUI::YELLOW);
        }
        StudyUI::foot(_c, "; . move   ENT enter   ` back");
    }

    void _drawSealed() {
        auto& d = _lib().drills[_drill];
        StudyUI::head(_c, "SEALED", d.title.c_str(), MUI::BONES);
        MUI::sigil(_c, MUI::SIG_HARAE, 36, 60, 40, MUI::BONES_DIM);
        int y = StudyUI::TOP + 2;
        y += 15 * StudyUI::wrap(_c, MUI::big(), "You don't have the Ward for this one yet.", 72, y, 160, 15, 2, MUI::WHITE);
        y += 4;
        MUI::text(_c, MUI::small(), "Earn it in Shugyo:", 72, y, MUI::DIM); y += 13;
        for (size_t i = 0; i < _need.size() && i < 3; i++) {
            _c.setClipRect(72, y, 164, 12);
            String w = _lib().wardName(_need[i]).c_str();
            String lt = _lib().lessonTitle(_need[i]).c_str();
            if (w != lt) { w += "  ("; w += lt; w += ")"; }
            MUI::text(_c, MUI::small(), w.c_str(), 72, y, MUI::CYAN);
            _c.clearClipRect();
            y += 12;
        }
        StudyUI::foot(_c, "` back");
    }

    // ---- briefing ----------------------------------------------------
    void _drawBrief() {
        auto& s = _game->scenario();
        String t = s.title.c_str(); t.toUpperCase();
        char r[16]; snprintf(r, sizeof(r), "%d round%s", (int)s.rounds.size(), s.rounds.size() == 1 ? "" : "s");
        StudyUI::head(_c, t.c_str(), r);
        int y = StudyUI::TOP;
        if (!s.demon.empty() || !s.faction.empty()) {
            uint16_t fc = _factionColor(s.faction);
            int x = MUI::text(_c, MUI::big(), s.demon.empty() ? "An unnamed hex" : s.demon.c_str(), 5, y, fc);
            if (!s.faction.empty()) {
                String f = "of the "; f += s.faction.c_str();
                MUI::text(_c, MUI::small(), f.c_str(), x + 6, y + 2, MUI::DIM);
            }
            y += 17;
        }
        StudyUI::wrap(_c, MUI::small(), s.briefing.c_str(), 5, y, 230, 12, (100 - y) / 12, MUI::CREAM);
        char ap[64];
        if (s.surge > 0) snprintf(ap, sizeof(ap), "%d AP a round. Surge: each AP on evidence costs %d uptime.", s.pointsPerRound, s.surge);
        else snprintf(ap, sizeof(ap), "%d action points a round. Evidence costs them.", s.pointsPerRound);
        MUI::text(_c, MUI::small(), ap, 5, 104, MUI::DIM);
        StudyUI::foot(_c, "ENT begin   ` back");
    }

    // ---- a round -----------------------------------------------------
    void _beginRounds() { _evSel = 0; _scr = H_ROUND; }

    void _keysRound(bool back, bool up, bool down, char k, bool enter) {
        auto& r = _game->round();
        int n = (int)r.evidence.size();
        if (back) {
            if (OsUI::confirmExit(_c, "Leave this drill?")) { _game.reset(); _scr = H_DRILLS; }
            return;
        }
        if (up) _evSel = max(0, _evSel - 1);
        if (down) _evSel = min(n - 1, _evSel + 1);
        if (enter && n > 0) {
            if (_game->investigate(_evSel)) { _evOpen = _evSel; _scr = H_EVIDENCE; }
            else OsUI::showToast(_c, "Not enough action points");
        }
        if (k == 'd' || k == 'D') _openDefend();
    }

    void _drawRound() {
        auto& s = _game->scenario();
        auto& r = _game->round();
        String t = s.title.c_str(); t.toUpperCase();
        char rr[20]; snprintf(rr, sizeof(rr), "Round %d/%d", (int)_game->roundIndex() + 1, (int)s.rounds.size());
        StudyUI::head(_c, t.c_str(), rr);
        MUI::text(_c, MUI::small(), "SYMPTOM", 5, StudyUI::TOP - 1, MUI::CYAN);
        StudyUI::wrap(_c, MUI::small(), r.symptom.c_str(), 5, StudyUI::TOP + 10, 230, 11, 3, MUI::CREAM);
        int y = 56;
        MUI::text(_c, MUI::small(), "UPTIME", 5, y, MUI::DIM);
        StudyUI::meter(_c, 44, y + 3, 70, _game->uptime() / 100.0f, _game->uptime() > 50 ? MUI::GREEN : MUI::RED);
        MUI::text(_c, MUI::small(), "DATA", 126, y, MUI::DIM);
        StudyUI::meter(_c, 152, y + 3, 82, _game->data() / 100.0f, _game->data() > 50 ? MUI::CYAN : MUI::RED);
        y += 12;
        MUI::text(_c, MUI::small(), "AP", 5, y, MUI::DIM);
        for (int i = 0; i < s.pointsPerRound && i < 8; i++)
            _c.fillRect(22 + i * 9, y + 3, 6, 6, i < _game->pointsLeft() ? MUI::YELLOW : MUI::BONES_DIM);
        StudyUI::textRight(_c, MUI::small(), "EVIDENCE", 234, y, MUI::CYAN);
        y += 14;
        int n = (int)r.evidence.size();
        int first = constrain(_evSel - 2, 0, max(0, n - 3));
        for (int i = first; i < n && i < first + 3; i++) {
            int yy = y + (i - first) * 12;
            bool sel = i == _evSel, seen = _game->seen(i);
            if (sel) StudyUI::bar(_c, 3, yy - 1, 234, 12);
            _c.setClipRect(8, yy - 1, 190, 12);
            MUI::text(_c, MUI::small(), r.evidence[i].name.c_str(), 8, yy, sel ? MUI::WHITE : seen ? MUI::GREEN : MUI::LABEL);
            _c.clearClipRect();
            int cost = _game->evidenceCost(i);
            char c[8]; snprintf(c, sizeof(c), "%d AP", cost);
            bool blessed = cost < r.evidence[i].cost;   // a Blessing made it free
            StudyUI::textRight(_c, MUI::small(), seen ? "read" : blessed ? "free" : c, 234, yy, blessed && !seen ? MUI::CYAN : MUI::DIM);
        }
        StudyUI::foot(_c, "; . move   ENT spend   D defend");
    }

    void _drawEvidence() {
        auto& e = _game->round().evidence[_evOpen];
        String t = "EVIDENCE  -  "; String n = e.name.c_str(); n.toUpperCase(); t += n;
        char ap[12]; snprintf(ap, sizeof(ap), "%d AP left", _game->pointsLeft());
        StudyUI::head(_c, t.c_str(), ap);
        StudyUI::wrap(_c, MUI::big(), e.text.c_str(), 5, StudyUI::TOP + 2, 230, 15, 6, MUI::CREAM);
        StudyUI::foot(_c, "ENT back to the round");
    }

    // ---- defend ------------------------------------------------------
    void _openDefend() {
        _defs.clear();
        auto& r = _game->round();
        for (int i = 0; i < (int)r.defenses.size(); i++)
            if (r.defenses[i].ward.empty() || _lib().progress.hasWard(r.defenses[i].ward)) _defs.push_back(i);
        _defSel = 0;
        _scr = H_DEFEND;
    }
    void _keysDefend(bool back, bool up, bool down, bool enter) {
        if (back) { _scr = H_ROUND; return; }
        if (up) _defSel = max(0, _defSel - 1);
        if (down) _defSel = min((int)_defs.size() - 1, _defSel + 1);
        if (enter && !_defs.empty()) {
            _scoreBefore = _game->score();
            _game->defend(_defs[_defSel]);
            _roundScore = _game->score() - _scoreBefore;
            _scr = H_REVEAL;
        }
    }
    void _drawDefend() {
        auto& r = _game->round();
        char ap[12]; snprintf(ap, sizeof(ap), "%d AP left", _game->pointsLeft());
        StudyUI::head(_c, "CHOOSE A DEFENCE", ap);
        int y = StudyUI::TOP;
        for (int i = 0; i < (int)_defs.size(); i++) {
            auto& d = r.defenses[_defs[i]];
            bool sel = i == _defSel;
            int lines = StudyUI::wrap(_c, MUI::small(), d.name.c_str(), 0, 0, 218, 11, 0, 0);
            lines = min(lines, 2);
            int h = lines * 11 + 3;
            if (y + h > StudyUI::BOTTOM) break;
            if (sel) StudyUI::bar(_c, 3, y - 1, 234, h);
            char n[3]; snprintf(n, sizeof(n), "%d", i + 1);
            MUI::text(_c, MUI::small(), n, 8, y, sel ? MUI::YELLOW : MUI::DIM);
            StudyUI::wrap(_c, MUI::small(), d.name.c_str(), 18, y, 216, 11, lines, sel ? MUI::WHITE : MUI::LABEL);
            if (!d.ward.empty()) _c.fillRect(234, y + 3, 2, 5, MUI::CYAN);   // a Ward you earned
            y += h + 2;
        }
        StudyUI::foot(_c, "; . move   ENT play it   ` evidence");
    }

    // ---- reveal ------------------------------------------------------
    void _drawReveal() {
        auto* r = _game->lastRound();
        auto* d = _game->lastDefense();
        if (!r || !d) return;
        uint16_t col = d->result == study::Outcome::Stopped ? MUI::GREEN : d->result == study::Outcome::Partial ? MUI::YELLOW : MUI::RED;
        char t[32], sc[12];
        const char* word = d->result == study::Outcome::Stopped ? "BROKEN" : d->result == study::Outcome::Partial ? "WEAKENED" : "IT HELD";
        snprintf(t, sizeof(t), "ROUND %d  -  %s", (int)_game->roundIndex(), word);
        snprintf(sc, sizeof(sc), "+%d", _roundScore);
        StudyUI::head(_c, t, sc, col);
        int y = StudyUI::TOP - 1;
        StudyUI::wrap(_c, MUI::small(), d->text.c_str(), 5, y, 230, 11, 2, MUI::CREAM);
        y += 24;
        const char* labels[4] = { "Attack", "Target", "CIA", "Technique" };
        const std::string* vals[4] = { &r->reveal.attack, &r->reveal.target, &r->reveal.cia, &r->reveal.technique };
        for (int i = 0; i < 4; i++) {
            if (vals[i]->empty()) continue;
            MUI::text(_c, MUI::small(), labels[i], 5, y, MUI::DIM);
            _c.setClipRect(62, y, 174, 12);
            MUI::text(_c, MUI::small(), vals[i]->c_str(), 62, y, i == 0 ? MUI::YELLOW : MUI::LABEL);
            _c.clearClipRect();
            y += 11;
        }
        if (!r->reveal.lesson.empty() && y < 110) {
            String s = "Study: "; s += _lib().lessonTitle(r->reveal.lesson).c_str();
            MUI::text(_c, MUI::small(), s.c_str(), 5, y + 1, MUI::CYAN);
        }
        StudyUI::foot(_c, _game->finished() ? "ENT results" : "ENT next round");
    }

    void _afterReveal() {
        if (!_game->finished()) { _evSel = 0; _scr = H_ROUND; return; }
        auto& lib = _lib();
        auto& s = _game->scenario();
        int best = lib.progress.bestScenarioScore(s.id);
        _cleared = best < 0;
        _newBest = _game->score() > best;
        lib.progress.recordScenario(s.id, _game->score());
        lib.save();
        uint32_t ki = (uint32_t)_game->score() / 10 + (_cleared ? KI_FIRST_CLEAR : 0);
        _kiPaid = Ki::get().add(Ki::S_HARAE, ki);
        _scr = H_DONE;
    }

    int _maxScore() const {
        auto& s = _game->scenario();
        return (int)s.rounds.size() * (100 + 10 * s.pointsPerRound);
    }
    static char _grade(int score, int max) {
        int pct = max ? score * 100 / max : 0;
        return pct >= 90 ? 'S' : pct >= 75 ? 'A' : pct >= 50 ? 'B' : 'C';
    }

    void _drawDone() {
        auto& s = _game->scenario();
        String t = s.title.c_str(); t.toUpperCase();
        StudyUI::head(_c, t.c_str(), _newBest ? "NEW BEST" : "", MUI::GREEN);
        char g[2] = { _grade(_game->score(), _maxScore()), 0 };
        MUI::enso(_c, 40, 62, 24, MUI::GREEN);
        MUI::textCentered(_c, MUI::display(), g, 40, 52, MUI::YELLOW);
        char sc[24]; snprintf(sc, sizeof(sc), "%d", _game->score());
        MUI::text(_c, MUI::display(), sc, 82, StudyUI::TOP + 2, MUI::WHITE);
        char of[24]; snprintf(of, sizeof(of), "of %d", _maxScore());
        MUI::text(_c, MUI::small(), of, 82, StudyUI::TOP + 24, MUI::DIM);
        int y = StudyUI::TOP + 40;
        MUI::text(_c, MUI::small(), "UPTIME", 82, y, MUI::DIM);
        StudyUI::meter(_c, 126, y + 3, 108, _game->uptime() / 100.0f, MUI::GREEN); y += 12;
        MUI::text(_c, MUI::small(), "DATA", 82, y, MUI::DIM);
        StudyUI::meter(_c, 126, y + 3, 108, _game->data() / 100.0f, MUI::CYAN); y += 14;
        char k[16]; snprintf(k, sizeof(k), "+%u ki", (unsigned)_kiPaid);
        int x = MUI::text(_c, MUI::big(), k, 82, y, MUI::YELLOW);
        if (_cleared) MUI::text(_c, MUI::small(), "first clear", x + 6, y + 2, MUI::GREEN);
        StudyUI::foot(_c, "ENT back to the Domain");
    }

    void _draw() {
        _c.fillSprite(MUI::BG);
        switch (_scr) {
            case H_HOME:     _drawHome(); break;
            case H_DRILLS:   _drawDrills(); break;
            case H_SEALED:   _drawSealed(); break;
            case H_BRIEF:    _drawBrief(); break;
            case H_ROUND:    _drawRound(); break;
            case H_EVIDENCE: _drawEvidence(); break;
            case H_DEFEND:   _drawDefend(); break;
            case H_REVEAL:   _drawReveal(); break;
            case H_DONE:     _drawDone(); break;
            case H_DEEP:     _drawDeep(); break;
        }
        StudyUI::push(_c);
    }
};
