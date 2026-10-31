// Host check for Shugyo and Harae content (no device needed). CI runs it:
//   g++ -std=gnu++17 -DLUA_32BITS=1 -Ilib/lua/src -Isrc/study test/study_host.cpp \
//       src/study/*.cpp -x c lib/lua/src/lua_all.c -lm -o study_host && ./study_host
// Every built-in lesson and drill must load, every drill must be fair (each
// round stoppable), every Ward and study link must name a real lesson with a
// task, and the hardware-free lessons must have a solution that passes.
#include <cstdio>
#include <map>
#include <set>
#include <string>

#include "builtin_content.h"
#include "game.h"
#include "lesson.h"
#include "progress.h"

using namespace study;

// The OS build of Lua reads the Arduino clock for os.clock().
extern "C" unsigned long millis(void) { return 0; }

struct FakeHal : Hal {
  std::string screen;
  void displayClear() override { screen.clear(); }
  void displayPrint(const std::string& t) override { screen += t; }
  bool gpioAllowed(int pin) override { return pin == 1 || pin == 2; }
  void gpioMode(int, bool) override {}
  void gpioWrite(int, int) override {}
  int gpioRead(int) override { return 0; }
  bool imuRead(ImuSample& s) override { s = ImuSample(); s.az = 1.0f; return true; }
  std::vector<uint8_t> i2cScan() override { return {0x44}; }
  std::vector<WifiNetwork> wifiScan() override { return {{"Home", -50, "wpa2", 6}, {"Cafe", -70, "open", 1}}; }
  bool fileRead(const std::string&, std::string&) override { return false; }
  bool fileWrite(const std::string&, const std::string&) override { return true; }
  uint32_t millis() override { return 0; }
  void delayMs(uint32_t) override {}
};

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); fails++; } } while (0)

// Answers for lessons that can be graded without hardware.
static const std::map<std::string, std::string> kSolutions = {
  {"t0-01-hello-lua", "print('hello, shugyo')"},
  {"t0-02-network-refresher", "for _, c in ipairs({'wifi','ip','gateway','dns'}) do print(c) end"},
  {"t4-01-wifi-recon",
   "local w=0 for _,n in ipairs(wifi.scan()) do print(n.ssid..', '..n.auth) if n.auth=='open' then w=w+1 end end print(w)"},
};

int main() {
  FakeHal hal;
  std::map<std::string, bool> lessonHasTask;

  for (int i = 0; i < kBuiltin_lessons_count; i++) {
    Lesson l(&hal);
    std::string err;
    bool ok = l.load(kBuiltin_lessons[i].source, err);
    CHECK(ok, "lesson %s: %s", kBuiltin_lessons[i].name, err.c_str());
    if (!ok) continue;
    CHECK(!lessonHasTask.count(l.id()), "lesson id %s used twice", l.id().c_str());
    lessonHasTask[l.id()] = l.hasTask();
    CHECK(!l.pages().empty(), "lesson %s has no pages", l.id().c_str());
    auto it = kSolutions.find(l.id());
    if (it != kSolutions.end()) {
      GradeResult g = l.grade(it->second);
      CHECK(g.passed, "lesson %s: solution did not pass: %s", l.id().c_str(), g.feedback.c_str());
    }
    std::printf("lesson   %-28s %zu pages, %zu quiz\n", l.id().c_str(), l.pages().size(), l.quiz().size());
  }

  for (int i = 0; i < kBuiltin_scenarios_count; i++) {
    Scenario s;
    std::string err;
    bool ok = loadScenario(&hal, kBuiltin_scenarios[i].source, s, err);
    CHECK(ok, "drill %s: %s", kBuiltin_scenarios[i].name, err.c_str());
    if (!ok) continue;
    for (const Round& r : s.rounds) {
      for (const Defense& d : r.defenses) {
        if (d.ward.empty()) continue;
        CHECK(lessonHasTask.count(d.ward) && lessonHasTask[d.ward], "drill %s: ward %s is not a lesson with a task",
              s.id.c_str(), d.ward.c_str());
      }
      if (!r.reveal.lesson.empty())
        CHECK(lessonHasTask.count(r.reveal.lesson), "drill %s: study link %s is not a lesson", s.id.c_str(),
              r.reveal.lesson.c_str());
    }
    // With every Ward held, nothing is sealed; and a perfect run scores.
    auto none = missingWards(s, [](const std::string&) { return true; });
    CHECK(none.empty(), "drill %s: sealed even with every Ward", s.id.c_str());
    Game g(s);
    while (!g.finished()) {
      size_t pick = 0;
      for (size_t j = 0; j < g.round().defenses.size(); j++)
        if (g.round().defenses[j].result == Outcome::Stopped) { pick = j; break; }
      CHECK(g.defend(pick) == Outcome::Stopped, "drill %s: stop response did not stop", s.id.c_str());
    }
    auto need = missingWards(s, [](const std::string&) { return false; });
    std::printf("drill    %-28s %zu rounds, best %d, needs %zu ward(s)\n", s.id.c_str(), s.rounds.size(), g.score(),
                need.size());
  }

  Progress p;
  p.recordTask("t0-01-hello-lua", true);
  p.recordQuiz("t0-01-hello-lua", 1, 1);
  p.recordScenario("s02-smart-home", 230);
  Progress q;
  q.parse(p.serialize());
  CHECK(q.hasWard("t0-01-hello-lua") && q.bestScenarioScore("s02-smart-home") == 230, "progress round trip");

  // Sandbox limits hold.
  {
    Lesson l(&hal);
    std::string err;
    l.load(kBuiltin_lessons[0].source, err);
    GradeResult g = l.grade("while true do end");
    CHECK(!g.run.ok && g.run.error.find("time limit") != std::string::npos, "runaway loop not stopped: %s",
          g.run.error.c_str());
    g = l.grade("local t={} for i=1,1e6 do t[i]=('x'):rep(100) end");
    CHECK(!g.run.ok, "memory cap not enforced");
    g = l.grade("os.execute('x')");
    CHECK(!g.run.ok, "os library reachable");
  }

  std::printf(fails ? "\n%d check(s) failed\n" : "\nall content checks passed\n", fails);
  return fails ? 1 : 0;
}
