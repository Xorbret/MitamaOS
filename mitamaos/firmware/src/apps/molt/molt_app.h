#pragma once
#include <M5Cardputer.h>
#include <WiFi.h>
#include "../../system/sd_settings.h"
#include "../../os/app.h"
#include "../../os/app_manager.h"
#include "../../os/os_ui.h"
#include "../../os/disp_lock.h"
#include "../../config.h"
#include "../../ui/mitama_ui.h"
#include "../../detect/detection.h"
#include "../../net/update.h"
#include "../../system/crash_log.h"
#include "../../study/library.h"

// Molt - updates MitamaOS. Checks the MitamaOS site for a newer build and
// downloads it to the SD card (/MitamaOS/update/), then restarts so it can
// be installed from Launcher > SD. Nothing on the computer needed.
// It also brings the lesson pack (Shugyo Trials, Harae drills) onto the
// card in /MitamaOS/study/; new lessons alone need no restart.
//
// Keys: Enter check / download, R restart, ` back.
class MoltApp : public App {
public:
    MoltApp(LGFX_Sprite& canvas) : _canvas(canvas) {}

    const char* getName() const override { return "Molt"; }
    const char* getIcon() const override { return "U"; }
    const char* getDesc() const override { return "Update MitamaOS"; }

    void onStart() override {
        OsUI::waitRelease();
        _state = S_IDLE; _latest = ""; _path = ""; _pct = -1;
        _lessons = OsUpdate::studyInstalled(); _lessonsNew = "";
        _say("Press Enter to look for an update.", MUI::DIM);
        _draw();
    }
    void onStop() override {}

    void update() override {
        M5Cardputer.update();
        auto& kb = M5Cardputer.Keyboard;
        if (kb.isKeyPressed('`')) { OsUI::waitRelease(); AppManager::get().goHome(); return; }
        if (kb.isKeyPressed('r') && _state == S_READY) { OsUI::waitRelease(); SdSettings::flush(); ESP.restart(); }
        if (kb.keysState().enter) {
            OsUI::waitRelease();
            if (_state == S_IDLE || _state == S_CURRENT) _check();
            else if (_state == S_NEWER) _download();
            else if (_state == S_LESSONS) _getLessons(false);
            _draw();
        }
    }

private:
    enum State : uint8_t { S_IDLE, S_CURRENT, S_NEWER, S_LESSONS, S_READY };
    LGFX_Sprite& _canvas;
    State    _state = S_IDLE;
    String   _latest, _path, _msg;
    String   _lessons, _lessonsNew;   // installed / newer lesson pack ("" = none)
    uint16_t _msgCol = MUI::DIM;
    int      _pct = -1;
    static inline MoltApp* _self = nullptr;

    void _say(const String& m, uint16_t col) { _msg = m; _msgCol = col; }

    bool _online() {
        if (WiFi.status() == WL_CONNECTED) return true;
        _say("Join Wi-Fi in Settings first.", MUI::YELLOW);
        return false;
    }

    // TLS needs ~40 KB: pause the detection radios while talking to the site.
    struct Hold {
        Hold()  { auto& d = DetectionEngine::get(); d.hold(true); for (int i = 0; i < 40 && !d.paused(); i++) delay(50); }
        ~Hold() { DetectionEngine::get().hold(false); }
    };

    void _check() {
        if (!_online()) return;
        _say("Looking...", MUI::CYAN); _draw();
        String msg;
        CrashLog::crumb("molt check");
        String smsg;
        int engine = 0;
        String pack;
        {
            Hold h;
            _latest = OsUpdate::latest(msg);
            if (_latest.length()) pack = OsUpdate::studyLatest(engine, smsg);
        }
        CrashLog::done();
        if (!_latest.length()) { _say(msg, MUI::BONES); return; }
        _lessons = OsUpdate::studyInstalled();
        // Lessons this build can run and doesn't have yet.
        _lessonsNew = pack.length() && pack != _lessons && engine <= study::kEngine ? pack : String();
        if (_latest != OsUpdate::current()) {
            _state = S_NEWER;
            _say(_lessonsNew.length() ? "New build and lessons found. Enter downloads them."
                                      : "New build found. Enter downloads it.", MUI::YELLOW);
        } else if (_lessonsNew.length()) {
            _state = S_LESSONS;
            _say("New lessons found. Enter installs them.", MUI::YELLOW);
        } else {
            _state = S_CURRENT;
            _say(pack.length() || _lessons.length() ? "You have the newest build and lessons."
                                                    : "You have the newest build.", MUI::GREEN);
        }
    }

    // Downloads and unpacks the lesson pack. After a build download
    // (`quiet`), a failure here doesn't hide that the build is saved.
    bool _getLessons(bool quiet) {
        if (!_online()) return false;
        _self = this; _pct = 0;
        _say("Getting lessons...", MUI::CYAN); _draw();
        String msg;
        CrashLog::crumb("molt lessons");
        int n;
        { Hold h; n = OsUpdate::studyUpdate(&MoltApp::_progress, msg); }
        CrashLog::done();
        _pct = -1;
        if (n < 0) {
            if (!quiet) _say(String("Lessons: ") + msg, MUI::BONES);
            return false;
        }
        study::Library::get().invalidate();
        _lessons = OsUpdate::studyInstalled();
        _lessonsNew = "";
        if (!quiet) {
            _state = S_CURRENT;
            _say(String("Lessons updated (") + n + " files). Open Shugyo or Harae.", MUI::GREEN);
        }
        return true;
    }

    static void _progress(size_t done, size_t total) {
        int pct = (int)(done * 100ULL / total);
        if (!_self || pct == _self->_pct) return;
        _self->_pct = pct;
        _self->_draw();
    }

    void _download() {
        if (!_online()) return;
        _self = this; _pct = 0;
        _say("Downloading...", MUI::CYAN); _draw();
        String msg;
        CrashLog::crumb("molt download");
        { Hold h; _path = OsUpdate::download(_latest, &MoltApp::_progress, msg); }
        CrashLog::done();
        _pct = -1;
        if (!_path.length()) { _say(msg, MUI::BONES); return; }
        bool lessonsOk = !_lessonsNew.length() || _getLessons(true);
        _state = S_READY;
        _say(lessonsOk ? "Saved. R restarts; install it from Launcher > SD."
                       : "Build saved, lessons failed (try again later). R restarts; install from Launcher > SD.",
             lessonsOk ? MUI::GREEN : MUI::YELLOW);
    }

    void _draw() {
        _canvas.fillSprite(MUI::BG);
        MUI::header(_canvas, "MOLT", MUI::GREEN);
        MUI::sigil(_canvas, MUI::SIG_MOLT, 30, 52, 40, MUI::GREEN);
        const int x = 64;
        int y = HEADER_H + 3;
        char buf[64];
        int e = MUI::text(_canvas, MUI::small(), "THIS ", x, y, MUI::WHITE);
        MUI::text(_canvas, MUI::small(), OsUpdate::current(), e, y, MUI::LABEL);
        y += 13;
        e = MUI::text(_canvas, MUI::small(), "NEWEST ", x, y, MUI::WHITE);
        MUI::text(_canvas, MUI::small(), _latest.length() ? _latest.c_str() : "?", e, y,
                  _state == S_NEWER || _state == S_READY ? MUI::YELLOW : MUI::LABEL);
        y += 13;
        e = MUI::text(_canvas, MUI::small(), "LESSONS ", x, y, MUI::WHITE);
        MUI::text(_canvas, MUI::small(), _lessonsNew.length() ? _lessonsNew.c_str() : _lessons.length() ? _lessons.c_str() : "none",
                  e, y, _lessonsNew.length() ? MUI::YELLOW : MUI::LABEL);
        y += 16;
        if (_pct >= 0) {
            snprintf(buf, sizeof(buf), "%d%%", _pct);
            int w = SCREEN_W - 3 - x;
            _canvas.drawRect(x, y, w, 9, MUI::DIM);
            _canvas.fillRect(x + 1, y + 1, (w - 2) * _pct / 100, 7, MUI::CYAN);
            y += 13;
        }
        if (_state == S_READY) {
            String f = _path.substring(_path.lastIndexOf('/') + 1);
            MUI::text(_canvas, MUI::small(), ("update/" + f).c_str(), x, y, MUI::CYAN);
            y += 13;
        }
        // Word-wrapped: error reasons can be long.
        {
            const int w = SCREEN_W - 3 - x;
            String rest = _msg, line;
            for (int row = 0; row < 5 && rest.length() && y + 10 <= SCREEN_H - FOOTER_H; row++) {
                int cut = rest.length();
                while (cut > 0 && MUI::width(_canvas, MUI::small(), rest.substring(0, cut).c_str()) > w) {
                    int sp = rest.lastIndexOf(' ', cut - 1);
                    cut = sp > 0 ? sp : cut - 1;
                }
                if (cut <= 0) cut = 1;
                MUI::text(_canvas, MUI::small(), rest.substring(0, cut).c_str(), x, y, _msgCol);
                rest = rest.substring(cut); rest.trim();
                y += 11;
            }
        }

        const char* foot = _state == S_NEWER ? "ENT download" : _state == S_LESSONS ? "ENT install" :
                           _state == S_READY ? "R restart" : "ENT check";
        MUI::footer(_canvas, foot, MUI::DIM, nullptr, MUI::GREEN);
        { DispLock _lk; _canvas.pushSprite(0, 0); }
    }
};
