#pragma once
// Kagami (mirror): settings for the optional second screen, system/kagami.h.
// Opened from Settings; off the grid. Each change is saved and shown on the
// second screen's test card straight away, so the right settings can be
// found by looking at the panel.
#include <M5Cardputer.h>
#include "../../os/app.h"
#include "../../os/app_manager.h"
#include "../../os/os_ui.h"
#include "../../os/disp_lock.h"
#include "../../config.h"
#include "../../ui/mitama_ui.h"
#include "../../system/kagami.h"

class KagamiApp : public App {
public:
    KagamiApp(LGFX_Sprite& canvas) : _canvas(canvas) {}

    const char* getName() const override { return "Kagami"; }
    const char* getIcon() const override { return "K"; }
    const char* getDesc() const override { return "Second screen"; }

    void onStart() override {
        OsUI::waitRelease();
        _sel = 0;
        _draw();
    }
    void onStop() override {}

    void update() override {
        M5Cardputer.update();
        auto& kb = M5Cardputer.Keyboard;
        if (!(kb.isChange() && kb.isPressed())) return;
        auto st = kb.keysState();
        for (char c : st.word) {
            if (c == '`') { OsUI::waitRelease(); AppManager::get().goHome(); return; }
            if (c == ';' && _sel > 0) _sel--;
            if (c == '.' && _sel < ROWS - 1) _sel++;
            if (c == '/') _change(+1);
            if (c == ',') _change(-1);
            if (c == 't' || c == 'T') _show();
        }
        if (st.enter) _change(+1);
        _draw();
    }

private:
    LGFX_Sprite& _canvas;
    int _sel = 0;
    static constexpr int ROWS = 4;

    void _change(int d) {
        auto& c = Kagami::cfg;
        switch (_sel) {
        case 0:
            c.chip = (c.chip + 3 + d) % 3;
            c.turn = c.swap = c.invert = 0;   // start each chip from its usual settings
            Kagami::save();
            _drawBusy();
            Kagami::end();
            if (Kagami::begin()) Kagami::testCard();
            return;
        case 1: c.turn = (c.turn + 4 + d) & 3; break;
        case 2: c.swap ^= 1; break;
        case 3: c.invert ^= 1; break;
        }
        Kagami::save();
        if (Kagami::running) { _drawBusy(); Kagami::applyLook(); Kagami::testCard(); }
    }

    // Redraws the test card, or retries bringing the panel up.
    void _show() {
        _drawBusy();
        if (!Kagami::running) Kagami::begin();
        Kagami::testCard();
    }

    void _drawBusy() {
        _canvas.fillSprite(MUI::BG);
        MUI::header(_canvas, "KAGAMI", MUI::YELLOW);
        MUI::text(_canvas, MUI::small(), "Drawing on the second screen...", 6, HEADER_H + 30, MUI::CYAN);
        { DispLock _lk; _canvas.pushSprite(0, 0); }
    }

    void _draw() {
        auto& c = Kagami::cfg;
        _canvas.fillSprite(MUI::BG);
        MUI::header(_canvas, "KAGAMI", MUI::YELLOW);

        static const char* NAMES[ROWS] = { "Screen", "Direction", "Colours", "Invert" };
        char val[ROWS][24];
        snprintf(val[0], 24, "%s", Kagami::CHIP_NAME[c.chip]);
        snprintf(val[1], 24, "%d of 4", c.turn + 1);
        snprintf(val[2], 24, "%s", c.swap ? "swapped" : "normal");
        snprintf(val[3], 24, "%s", c.invert ? "on" : "off");

        int y = HEADER_H + 4;
        for (int i = 0; i < ROWS; i++) {
            bool s = i == _sel;
            if (s) { _canvas.fillRect(2, y - 1, SCREEN_W - 4, 17, MUI::YELLOW_DIM); _canvas.fillRect(2, y - 1, 2, 17, MUI::YELLOW); }
            bool dim = i > 0 && c.chip == Kagami::OFF;
            MUI::text(_canvas, MUI::small(), NAMES[i], 10, y, dim ? MUI::DIM : (s ? MUI::WHITE : MUI::LABEL));
            MUI::text(_canvas, MUI::small(), val[i], 110, y, dim ? MUI::DIM : (s ? MUI::YELLOW : MUI::CYAN));
            y += 18;
        }

        const char* state;
        uint16_t col = MUI::DIM;
        if (Kagami::running) { state = "On: showing the test card"; col = MUI::GREEN; }
        else if (c.chip == Kagami::OFF) state = "Off";
        else { state = Kagami::why; col = MUI::MAGENTA; }
        MUI::text(_canvas, MUI::small(), state, 10, y + 2, col);

        MUI::footer(_canvas, "ENT/,/ change   T redraw", MUI::DIM, "` back");
        { DispLock _lk; _canvas.pushSprite(0, 0); }
    }
};
