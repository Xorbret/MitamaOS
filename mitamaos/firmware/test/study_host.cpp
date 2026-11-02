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
  {"t0-04-tickets-cia",
   "local words={C={'leak','outside','photo'},I={'changed','edited'},A={'offline','down','timing out'}}\n"
   "for _,t in ipairs(lab.tickets) do local tag='?' local text=t.text:lower()\n"
   "for leg,list in pairs(words) do for _,w in ipairs(list) do if text:find(w,1,true) then tag=leg end end end\n"
   "print(t.id..' '..tag) end"},
  {"t0-05-rules-of-engagement",
   "for _,t in ipairs(lab.targets) do if t.owner=='me' or t.permission then print(t.name) end end"},
  {"b1-02-pick-a-network",
   "local f=0 for _,n in ipairs(lab.scan) do if n.ssid==lab.saved.ssid and n.auth~=lab.saved.auth then\n"
   "print(n.ssid..' '..n.auth..' ch'..n.channel) f=f+1 end end print(f)"},
  {"b1-03-interference-or-jamming",
   "for _,r in ipairs(lab.readings) do local v='interference' if #r.busy>=10 then v='jamming' end print(r.label..' '..v) end"},
  {"b1-04-wired-backups",
   "local w=0 for _,d in ipairs(lab.devices) do if d.critical and not d.moves then print(d.name) w=w+1 end end print(w)"},
};

// Lessons whose starter code must not pass as it stands (the learner has
// something to do).
static const char* const kStarterMustFail[] = {
  "t0-04-tickets-cia", "t0-05-rules-of-engagement", "b1-02-pick-a-network",
  "b1-03-interference-or-jamming", "b1-04-wired-backups",
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
    for (const char* id : kStarterMustFail) {
      if (l.id() != id) continue;
      GradeResult g = l.grade(l.starter());
      CHECK(!g.passed, "lesson %s: starter code already passes", l.id().c_str());
    }
    for (const std::string& p : l.prereqs())
      CHECK(p != l.id(), "lesson %s requires itself", l.id().c_str());
    std::printf("lesson   %-30s %zu pages, %zu quiz%s%s\n", l.id().c_str(), l.pages().size(), l.quiz().size(),
                l.labSource().empty() ? "" : ", lab", l.card().empty() ? "" : (", card " + l.card()).c_str());
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
    for (const Round& r : s.rounds)
      for (const Evidence& e : r.evidence)
        if (!e.freeWith.empty())
          CHECK(lessonHasTask.count(e.freeWith), "drill %s: free_with %s is not a lesson", s.id.c_str(), e.freeWith.c_str());
    std::printf("drill    %-30s %zu rounds, best %d, needs %zu ward(s)\n", s.id.c_str(), s.rounds.size(), g.score(),
                need.size());
  }

  // Surge drains uptime but never ends a drill by itself, and a Blessing
  // makes its evidence free.
  {
    Scenario s;
    std::string err;
    CHECK(loadScenario(&hal, "return { id='x', surge=5, points=3, rounds={ { symptom='s', "
                             "evidence={ {name='a',cost=2,free_with='b1-03-interference-or-jamming'}, {name='b',cost=1} }, "
                             "defenses={ {name='d',result='stop'} } } } }", s, err), "surge drill: %s", err.c_str());
    Game g(s);
    CHECK(g.evidenceCost(0) == 2, "free_with applied without the card");
    g.setWards([](const std::string& w) { return w == "b1-03-interference-or-jamming"; });
    CHECK(g.evidenceCost(0) == 0, "free_with not applied with the card");
    g.investigate(0); g.investigate(1);
    CHECK(g.uptime() == 95, "surge: expected 95 uptime, got %d", g.uptime());
    Scenario bad;
    CHECK(!loadScenario(&hal, "return { id='y', surge=40, points=3, rounds={ { symptom='s', evidence={ {name='a',cost=3} }, "
                              "defenses={ {name='d',result='stop'} } } } }", bad, err), "unfair surge drill accepted");
  }

  Progress p;
  p.recordTask("t0-01-hello-lua", true);
  p.recordQuiz("t0-01-hello-lua", 1, 1);
  p.recordScenario("s02-smart-home", 230);
  Progress q;
  q.parse(p.serialize());
  CHECK(q.hasWard("t0-01-hello-lua") && q.bestScenarioScore("s02-smart-home") == 230, "progress round trip");
  p.recordField("t0-01-hello-lua");
  q.parse(p.serialize());
  CHECK(q.lesson("t0-01-hello-lua")->fieldDone && q.lesson("t0-01-hello-lua")->marks() == 3, "field mark round trip");
  q.parse("L|old|1|2|2\n");
  CHECK(q.hasWard("old") && !q.lesson("old")->fieldDone, "old progress lines still parse");

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
