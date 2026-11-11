#pragma once
#include <algorithm>
#include <string>
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
// A paragraph that starts with '|' is code: drawn in the fixed-width font
// in cyan, indented, and broken at spaces if it is too long (Lua doesn't
// mind where a line breaks between words).
inline int wrap(LGFX_Sprite& c, const lgfx::IFont* f, const char* s, int x, int y, int w, int lh, int maxLines,
                uint16_t col, int skip = 0) {
    int n = 0;
    auto emit = [&](const std::string& l) {
        if (n >= skip && n < skip + maxLines) MUI::text(c, f, l.c_str(), x, y + (n - skip) * lh, col);
        n++;
    };
    auto emitCode = [&](const std::string& l) {
        if (n >= skip && n < skip + maxLines) {
            int ly = y + (n - skip) * lh;
            c.fillRect(x, ly, 2, lh - 2, MUI::BONES_DIM);
            c.setFont(&lgfx::fonts::Font0); c.setTextSize(1); c.setTextDatum(top_left);
            c.setTextColor(MUI::CYAN);
            c.drawString(l.c_str(), x + 6, ly + 2);
        }
        n++;
    };
    auto fits = [&](const std::string& l) { return MUI::width(c, f, l.c_str()) <= w; };
    auto prose = [&](const std::string& para) {
        std::string line, word;
        for (size_t i = 0; i <= para.size(); i++) {
            char ch = i < para.size() ? para[i] : ' ';
            if (ch != ' ') { word += ch; continue; }
            if (word.empty()) continue;
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
        emit(line);
    };
    auto code = [&](std::string l) {
        const size_t cols = (size_t)std::max(8, (w - 6) / 6);
        std::string lead;                      // continuation lines are indented by 2
        size_t lim = cols;
        while (l.size() > lim) {
            size_t cut = l.rfind(' ', lim);
            if (cut == std::string::npos || cut == 0) cut = lim;
            emitCode(lead + l.substr(0, cut));
            l = l.substr(cut + (l[cut] == ' ' ? 1 : 0));
            lead = "  "; lim = cols - 2;
        }
        emitCode(lead + l);
    };
    std::string str = s ? s : "";
    size_t from = 0;
    for (;;) {
        size_t nl = str.find('\n', from);
        std::string para = str.substr(from, nl == std::string::npos ? std::string::npos : nl - from);
        if (!para.empty() && para[0] == '|') code(para.substr(1));
        else if (!para.empty() || nl != std::string::npos) prose(para);
        if (nl == std::string::npos) break;
        from = nl + 1;
    }
    return n;
}

inline void push(LGFX_Sprite& c) { DispLock _lk; c.pushSprite(0, 0); }

}  // namespace StudyUI
