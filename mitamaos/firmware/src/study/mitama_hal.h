#pragma once
// The hardware lessons may touch, over MitamaOS's own services. Learner code
// only reaches it through the sandbox, and only for the modules the lesson
// asked for: Port A pins, the IMU, a Grove I2C scan, a passive Wi-Fi scan,
// files in /MitamaOS/user/Study. Nothing here can transmit at another device.
#include <M5Cardputer.h>
#include <SD.h>
#include <WiFi.h>
#include "hal.h"
#include "../system/tsukumo.h"

namespace study {

class MitamaHal : public Hal {
public:
    static constexpr const char* USER_DIR = "/MitamaOS/user/Study";

    // display.print() lands in the lesson console, not over the app's screen.
    std::string screen;

    void displayClear() override { screen.clear(); }
    void displayPrint(const std::string& text) override {
        screen += text;
        if (screen.size() > 1024) screen.erase(0, screen.size() - 1024);
    }

    // Port A (Grove) only, and not while an M5MonsterC5 holds it.
    bool gpioAllowed(int pin) override {
        if (Tsukumo::monsterLinked) return false;
        return pin == Tsukumo::G_SDA || pin == Tsukumo::G_SCL;
    }
    void gpioMode(int pin, bool output) override {
        if (Tsukumo::luaWire) { Wire.end(); Tsukumo::luaWire = false; }
        pinMode(pin, output ? OUTPUT : INPUT);
    }
    void gpioWrite(int pin, int value) override { digitalWrite(pin, value ? HIGH : LOW); }
    int gpioRead(int pin) override { return digitalRead(pin); }

    bool imuRead(ImuSample& out) override {
        if (!M5.Imu.isEnabled()) return false;
        M5.Imu.update();
        return M5.Imu.getAccel(&out.ax, &out.ay, &out.az) && M5.Imu.getGyro(&out.gx, &out.gy, &out.gz);
    }

    std::vector<uint8_t> i2cScan() override {
        std::vector<uint8_t> found;
        if (Tsukumo::monsterLinked) return found;
        if (Tsukumo::luaWire) { Wire.end(); Tsukumo::luaWire = false; }
        auto& ex = m5::Ex_I2C;
        if (!ex.begin()) return found;
        bool seen[0x80] = {};
        ex.scanID(seen);
        for (uint8_t a = 0x08; a < 0x78; a++) if (seen[a]) found.push_back(a);
        ex.release();
        pinMode(Tsukumo::G_SDA, INPUT); pinMode(Tsukumo::G_SCL, INPUT);
        return found;
    }

    // Listens for beacons (passive), like the Settings network picker.
    std::vector<WifiNetwork> wifiScan() override {
        std::vector<WifiNetwork> nets;
        if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
        int n = WiFi.scanNetworks(false, true, true);
        for (int i = 0; i < n && i < 40; i++) {
            WifiNetwork w;
            w.ssid = WiFi.SSID(i).c_str();
            if (w.ssid.empty()) w.ssid = "(hidden)";
            w.rssi = WiFi.RSSI(i);
            w.auth = _auth(WiFi.encryptionType(i));
            w.channel = WiFi.channel(i);
            nets.push_back(w);
        }
        WiFi.scanDelete();
        return nets;
    }

    bool fileRead(const std::string& path, std::string& out) override {
        File f = SD.open((std::string(USER_DIR) + "/" + path).c_str(), FILE_READ);
        if (!f) return false;
        out.clear();
        while (f.available() && out.size() < 16384) out += (char)f.read();
        f.close();
        return true;
    }
    bool fileWrite(const std::string& path, const std::string& data) override {
        if (data.size() > 16384 || SD.cardType() == CARD_NONE) return false;
        if (!SD.exists("/MitamaOS/user")) SD.mkdir("/MitamaOS/user");
        if (!SD.exists(USER_DIR)) SD.mkdir(USER_DIR);
        File f = SD.open((std::string(USER_DIR) + "/" + path).c_str(), FILE_WRITE);
        if (!f) return false;
        size_t n = f.write((const uint8_t*)data.data(), data.size());
        f.close();
        return n == data.size();
    }

    uint32_t millis() override { return ::millis(); }
    void delayMs(uint32_t ms) override { ::delay(ms); }

private:
    static const char* _auth(wifi_auth_mode_t m) {
        switch (m) {
            case WIFI_AUTH_OPEN: return "open";
            case WIFI_AUTH_WEP: return "wep";
            case WIFI_AUTH_WPA_PSK: return "wpa";
            case WIFI_AUTH_WPA2_PSK: case WIFI_AUTH_WPA_WPA2_PSK: return "wpa2";
            case WIFI_AUTH_WPA2_ENTERPRISE: return "wpa2-ent";
            case WIFI_AUTH_WPA3_PSK: return "wpa3";
            case WIFI_AUTH_WPA2_WPA3_PSK: return "wpa2/wpa3";
            default: return "other";
        }
    }
};

}  // namespace study
