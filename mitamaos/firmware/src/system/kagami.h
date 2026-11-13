#pragma once
// Kagami: the optional second screen (outline §22). A 2.8" 320x240 SPI
// panel (ILI9341 or ST7789) in Prokyon's "Cap TFT V2" clamshell lid, wired
// to the EXT header:
//   RES G3, DC G6, SCL G40, SDA G14, CS G5, 5V OUT -> 3.3 V step-down -> VCC+BLK.
// It shares the SD card's SPI bus (SdBus::spi). Every write runs inside an
// SPI transaction, which takes the bus lock the SD library also uses, so
// card reads and screen writes take turns. The panel has no MISO and no
// backlight control: it can't be detected and is lit whenever it has power,
// so it's switched on in Settings > Kagami and is off by default.
//
// The plain wiring uses the same pins as the Cap LoRa-1262 (CS G5, RST G3,
// and G6 is the radio's BUSY output), so Kagami stays off while a LoRa cap
// is fitted. The stacked LoRa wiring isn't built yet.
//
// No full-frame buffer (no PSRAM): drawing goes straight to the panel in
// rectangles and short sprite strips.
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <M5Cardputer.h>
#include "sd_bus.h"
#include "sd_settings.h"
#include "tsukumo.h"
#include "../ui/mitama_ui.h"

namespace Kagami {

static constexpr int PIN_CS = 5, PIN_DC = 6, PIN_RST = 3;
static constexpr int W = 320, H = 240;
static constexpr uint32_t HZ = 20000000;   // thin wires through a hinge: stay modest

enum Chip : uint8_t { OFF = 0, ILI9341 = 1, ST7789 = 2 };
static constexpr const char* CHIP_NAME[] = { "Off", "ILI9341", "ST7789" };

struct Config {
    uint8_t chip = OFF;
    uint8_t turn = 0;      // 0..3: the four landscape ways a panel can sit
    uint8_t swap = 0;      // red and blue swapped from the chip's usual order
    uint8_t invert = 0;    // colours inverted from the chip's usual setting
};

// Usual landscape settings per chip; turn/swap/invert are relative to these.
inline uint8_t _baseTurn(uint8_t chip) { return chip == ST7789 ? 2 : 0; }
inline bool    _baseBgr(uint8_t chip)  { return chip == ILI9341; }
inline bool    _baseInv(uint8_t chip)  { return chip == ST7789; }   // IPS ST7789 panels want INVON

inline Config  cfg;
inline bool    running = false;
inline const char* why = "";   // why it isn't running, for the Settings page

inline void load() {
    Preferences p; p.begin("kagami", true);
    cfg.chip = p.getUChar("chip", OFF);
    cfg.turn = p.getUChar("turn", 0) & 3;
    cfg.swap = p.getUChar("swap", 0) ? 1 : 0;
    cfg.invert = p.getUChar("inv", 0) ? 1 : 0;
    p.end();
    if (cfg.chip > ST7789) cfg.chip = OFF;
}
inline void save() {
    Preferences p; p.begin("kagami", false);
    p.putUChar("chip", cfg.chip); p.putUChar("turn", cfg.turn);
    p.putUChar("swap", cfg.swap); p.putUChar("inv", cfg.invert);
    p.end();
    SdSettings::changed();
}

inline bool loraCapFitted() { return Tsukumo::Scanner::get().result().capLora; }

// ── bus ─────────────────────────────────────────────────────────────
inline void _begin() {
    SdBus::spi->beginTransaction(SPISettings(HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_CS, LOW);
}
inline void _end() {
    digitalWrite(PIN_CS, HIGH);
    SdBus::spi->endTransaction();
}
inline void _cmd(uint8_t c) {   // inside a transaction
    digitalWrite(PIN_DC, LOW);
    SdBus::spi->transfer(c);
    digitalWrite(PIN_DC, HIGH);
}
inline void _send(uint8_t c, const uint8_t* d = nullptr, size_t n = 0) {
    _begin(); _cmd(c);
    if (n) SdBus::spi->writeBytes(d, n);
    _end();
}
inline void _window(int x, int y, int w, int h) {   // inside a transaction; leaves RAMWR open
    uint8_t ca[4] = { uint8_t(x >> 8), uint8_t(x), uint8_t((x + w - 1) >> 8), uint8_t(x + w - 1) };
    uint8_t ra[4] = { uint8_t(y >> 8), uint8_t(y), uint8_t((y + h - 1) >> 8), uint8_t(y + h - 1) };
    _cmd(0x2A); SdBus::spi->writeBytes(ca, 4);
    _cmd(0x2B); SdBus::spi->writeBytes(ra, 4);
    _cmd(0x2C);
}

// MADCTL for the current settings: MV (landscape) plus one of four flips.
inline uint8_t _madctl() {
    static const uint8_t TURN[4] = { 0x00, 0xC0, 0x40, 0x80 };   // -, MX|MY, MX, MY
    uint8_t t = (_baseTurn(cfg.chip) + cfg.turn) & 3;
    bool bgr = _baseBgr(cfg.chip) != (bool)cfg.swap;
    return 0x20 | TURN[t] | (bgr ? 0x08 : 0x00);
}

// Re-sends orientation and colour settings without a reset.
inline void applyLook() {
    if (!running) return;
    uint8_t m = _madctl();
    _send(0x36, &m, 1);
    _send((_baseInv(cfg.chip) != (bool)cfg.invert) ? 0x21 : 0x20);
}

// ── drawing ─────────────────────────────────────────────────────────
inline void fillRect(int x, int y, int w, int h, uint16_t rgb565) {
    if (!running) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > W) w = W - x;
    if (y + h > H) h = H - y;
    if (w <= 0 || h <= 0) return;
    static uint8_t line[W * 2];
    for (int i = 0; i < w; i++) { line[i * 2] = rgb565 >> 8; line[i * 2 + 1] = rgb565 & 0xFF; }
    // A few rows per transaction so the SD card never waits long.
    for (int r = 0; r < h; r += 16) {
        int rows = min(16, h - r);
        _begin(); _window(x, y + r, w, rows);
        for (int k = 0; k < rows; k++) SdBus::spi->writeBytes(line, w * 2);
        _end();
    }
}
inline void fill(uint16_t c) { fillRect(0, 0, W, H, c); }

// Pushes a 16-bit sprite (LovyanGFX keeps those bytes in panel order).
inline void push(LGFX_Sprite& s, int x, int y) {
    if (!running || s.getColorDepth() != 16) return;
    const uint8_t* b = (const uint8_t*)s.getBuffer();
    int w = s.width(), h = s.height();
    for (int r = 0; r < h; r += 8) {
        int rows = min(8, h - r);
        _begin(); _window(x, y + r, w, rows);
        SdBus::spi->writeBytes(b + (size_t)r * w * 2, (size_t)rows * w * 2);
        _end();
    }
}

// One line of Rajdhani text on a solid background, drawn through a small
// strip sprite (w x 22 px, about 14 KB at full width, freed straight after).
inline void text(const char* s, int x, int y, uint16_t fg, uint16_t bg,
                 const lgfx::IFont* f = nullptr, int w = 0) {
    if (!running) return;
    if (!f) f = MUI::small();
    LGFX_Sprite strip(&M5Cardputer.Display);
    strip.setColorDepth(16);
    if (!w) w = min(W - x, MUI::width(strip, f, s) + 2);
    if (w <= 0 || !strip.createSprite(w, 22)) return;
    strip.fillSprite(bg);
    MUI::text(strip, f, s, 1, 2, fg);
    push(strip, x, y);
    strip.deleteSprite();
}

// ── panel bring-up ──────────────────────────────────────────────────
inline void _initIli9341() {
    static const uint8_t seq[] = {
        // cmd, n, data...   (n | 0x80 = wait 150 ms after)
        0xEF, 3, 0x03, 0x80, 0x02,
        0xCF, 3, 0x00, 0xC1, 0x30,
        0xED, 4, 0x64, 0x03, 0x12, 0x81,
        0xE8, 3, 0x85, 0x00, 0x78,
        0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
        0xF7, 1, 0x20,
        0xEA, 2, 0x00, 0x00,
        0xC0, 1, 0x23,             // power control 1
        0xC1, 1, 0x10,             // power control 2
        0xC5, 2, 0x3E, 0x28,       // VCOM
        0xC7, 1, 0x86,
        0x3A, 1, 0x55,             // 16-bit colour
        0xB1, 2, 0x00, 0x18,       // frame rate
        0xB6, 3, 0x08, 0x82, 0x27,
        0xF2, 1, 0x00,
        0x26, 1, 0x01,             // gamma curve
        0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
        0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
        0x11, 0x80,                // sleep out
        0x29, 0x80,                // display on
        0x00
    };
    for (const uint8_t* p = seq; *p; ) {
        uint8_t c = *p++, n = *p++;
        _send(c, p, n & 0x7F);
        p += n & 0x7F;
        if (n & 0x80) delay(150);
    }
}
inline void _initSt7789() {
    _send(0x11); delay(120);                    // sleep out
    { uint8_t v = 0x55; _send(0x3A, &v, 1); }   // 16-bit colour
    _send(0x13);                                // normal mode
    _send(0x29); delay(20);                     // display on
}

// Brings the panel up with the saved settings. Returns false (and sets
// `why`) if it shouldn't or can't run.
inline bool begin() {
    running = false;
    if (cfg.chip == OFF) { why = "Off"; return false; }
    if (!SdBus::spi)     { why = "SD bus not ready"; return false; }
    if (loraCapFitted()) { why = "LoRa cap fitted: it uses the same pins"; return false; }
    pinMode(PIN_CS, OUTPUT); digitalWrite(PIN_CS, HIGH);
    pinMode(PIN_DC, OUTPUT); digitalWrite(PIN_DC, HIGH);
    pinMode(PIN_RST, OUTPUT);
    digitalWrite(PIN_RST, HIGH); delay(5);
    digitalWrite(PIN_RST, LOW);  delay(20);
    digitalWrite(PIN_RST, HIGH); delay(150);
    _send(0x01); delay(150);                    // software reset as well
    if (cfg.chip == ILI9341) _initIli9341(); else _initSt7789();
    running = true; why = "";
    applyLook();
    return true;
}

// Blanks the panel and puts it to sleep. The backlight is wired straight
// to 3.3 V, so it stays lit; the picture just goes black.
inline void end() {
    if (running) { fill(0x0000); _send(0x28); _send(0x10); }
    running = false;
    digitalWrite(PIN_CS, HIGH);
}

// ── test card ───────────────────────────────────────────────────────
// Everything the wiring check needs at a glance: readable text (orientation
// and mirroring), labelled colour bars (red/blue swap), a 1 px frame (edges
// and offsets) and a white/black split (inversion).
inline void testCard() {
    if (!running) return;
    using MUI::BG, MUI::WHITE, MUI::YELLOW, MUI::CYAN, MUI::CREAM, MUI::DIM, MUI::small, MUI::title;
    fill(BG);
    // frame
    fillRect(0, 0, W, 1, WHITE); fillRect(0, H - 1, W, 1, WHITE);
    fillRect(0, 0, 1, H, WHITE); fillRect(W - 1, 0, 1, H, WHITE);
    // title band
    fillRect(1, 1, W - 2, 26, YELLOW);
    text("KAGAMI  TEST CARD", 8, 3, BG, YELLOW, title());
    text(CHIP_NAME[cfg.chip], W - 70, 3, BG, YELLOW, title(), 62);
    // top-left marker
    fillRect(6, 34, 14, 3, CYAN); fillRect(6, 34, 3, 14, CYAN);
    text("TOP LEFT", 24, 31, CYAN, BG);
    // colour bars, labelled
    static const uint16_t COL[6] = { 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F };
    static const char* NAME[6] = { "RED", "GREEN", "BLUE", "YELLOW", "CYAN", "PINK" };
    for (int i = 0; i < 6; i++) {
        int x = 8 + i * 51;
        fillRect(x, 60, 48, 70, COL[i]);
        text(NAME[i], x, 132, WHITE, BG, small(), 48);
    }
    // white / black split: if white shows black, flip Invert
    fillRect(8, 162, 150, 20, WHITE);
    text("WHITE", 164, 161, WHITE, BG);
    text("If you can read this, the wiring works.", 8, 190, CREAM, BG);
    text("Fix colours or direction in Settings > Kagami.", 8, 210, DIM, BG);
}

// Called once at boot, after the SD card and Tsukumo's scan.
inline void boot() {
    load();
    if (begin()) testCard();
}

} // namespace Kagami
