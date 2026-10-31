#pragma once
// Drawing helpers Shugyo and Harae share, in the Night City look of the
// approved mockups: black ground, yellow selection bar on dark violet, red
// bones rules, Rajdhani text.
#include <M5Cardputer.h>
#include <string>
#include "../../config.h"
#include "../../os/disp_lock.h"
#include "../../ui/mitama_ui.h"

namespace StudyUI {

static constexpr int TOP = HEADER_H + 3;               // first content row
static constexpr int BOTTOM = SCREEN_H - FOOTER_H - 1; // last content row

// Title bar for an inner screen: title left, a note right, accent rule.
inline void head(LGFX_Sprite& c, const char* title, const char* right = nullptr, uint16_t rule = MUI::BONES) {
    c.fillRect(0, 0, SCREEN_W, HEADER_H, MUI::BG);
    c.fillRect(0, HEADER_H - 1, SCREEN_W, 1, rule);
    int rx = SCREEN_W - 4;
    if (right && *right) {
        rx -= MUI::width(c, MUI::small(), right);
        MUI::text(c, MUI::small(), right, rx, 2, MUI::DIM);
        rx -= 6;
    }
    c.setClipRect(0, 0, rx, HEADER_H - 1);
    MUI::text(c, MUI::title(), title, 4, 1, MUI::YELLOW, 1);
    c.clearClipRect();
}

inline void foot(LGFX_Sprite& c, const char* keys, const char* right = nullptr) {
    MUI::footer(c, keys, MUI::DIM, right);
}

// The selection bar: dark violet with a yellow outline.
inline void bar(LGFX_Sprite& c, int x, int y, int w, int h) {
    c.fillRect(x, y, w, h, MUI::PANEL);
    c.drawRect(x, y, w, h, MUI::YELLOW);
}

inline void rule(LGFX_Sprite& c, int y) { c.fillRect(4, y, SCREEN_W - 8, 1, MUI::BONES_DIM); }

inline void meter(LGFX_Sprite& c, int x, int y, int w, float v, uint16_t col) {
    c.drawRect(x, y, w, 6, MUI::BONES_DIM);
    int f = (int)((w - 2) * constrain(v, 0.0f, 1.0f) + 0.5f);
    if (f > 0) c.fillRect(x + 1, y + 1, f, 4, col);
}

inline int textRight(LGFX_Sprite& c, const lgfx::IFont* f, const char* s, int right, int y, uint16_t col) {
    int x = right - MUI::width(c, f, s);
    MUI::text(c, f, s, x, y, col);
    return x;
}

// Word-wraps s into width w. Draws lines [skip, skip+maxLines) starting at
// y, `lh` apart, and returns the total number of lines (so callers can
// scroll). Honours '\n'. maxLines 0 = measure only.
inline int wrap(LGFX_Sprite& c, const lgfx::IFont* f, const char* s, int x, int y, int w, int lh, int maxLines,
                uint16_t col, int skip = 0) {
    std::string line, word;
    int n = 0;
    auto emit = [&](const std::string& l) {
        if (n >= skip && n < skip + maxLines) MUI::text(c, f, l.c_str(), x, y + (n - skip) * lh, col);
        n++;
    };
    auto fits = [&](const std::string& l) { return MUI::width(c, f, l.c_str()) <= w; };
    for (const char* p = s;; p++) {
        char ch = *p;
        if (ch == ' ' || ch == '\n' || ch == 0) {
            if (!word.empty()) {
                std::string t = line.empty() ? word : line + " " + word;
                if (!line.empty() && !fits(t)) { emit(line); line = word; }
                else line = t;
                while (!fits(line) && line.size() > 1) {          // a word wider than the box
                    size_t k = line.size() - 1;
                    while (k > 1 && !fits(line.substr(0, k))) k--;
                    emit(line.substr(0, k)); line = line.substr(k);
                }
                word.clear();
            }
            if (ch == '\n') { emit(line); line.clear(); }
            if (ch == 0) break;
        } else {
            word += ch;
        }
    }
    if (!line.empty()) emit(line);
    return n;
}

inline void push(LGFX_Sprite& c) { DispLock _lk; c.pushSprite(0, 0); }

}  // namespace StudyUI
