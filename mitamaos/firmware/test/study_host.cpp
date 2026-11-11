// Host check for Shugyo and Harae content (no device needed). CI runs it:
//   g++ -std=gnu++17 -DLUA_32BITS=1 -Ilib/lua/src -Isrc/study test/study_host.cpp \
//       src/study/*.cpp -x c lib/lua/src/lua_all.c -lm -o study_host && ./study_host
// Every lesson and drill (the built-in set and the SD pack) must load, every
// drill must be fair (each round stoppable), every Ward and study link must
// name a real lesson with a task, and the hardware-free lessons must have a
// solution that passes. The built-in set must also stand on its own, since a
// device without the SD pack sees only that.
#include <cstdio>
#include <map>
#include <set>
#include <string>

#include "builtin_content.h"
#include "study_all_content.h"
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
   "local w=0 for _,n in ipairs(wifi.scan()) do print(n.ssid..', '..n.auth) if n.auth=='open' or n.auth=='wep' then w=w+1 end end print(w)"},
  {"t1-01-imu",
   "local ax,ay,az=imu.read() print(math.max(math.abs(ax),math.abs(ay),math.abs(az)))"},
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

// What a learner ends up with after following the hint word for word, for
// hints that aren't "Replace the -- line with: <code>" (those are applied
// automatically). Every lesson with a task must pass this way, so no hint
// leaves someone stuck.
static const std::map<std::string, std::string> kHintResult = {
  {"t0-01-hello-lua", "print(\"hello, shugyo\")"},
  {"t0-04-tickets-cia",
   "local words = {\n  C = { \"leak\", \"outside\", \"photo\" },\n  I = { \"changed\", \"edited\" },\n"
   "  A = { \"offline\", \"down\", \"timing out\" },\n}\nfor _, t in ipairs(lab.tickets) do\n  local tag = \"?\"\n"
   "  local text = t.text:lower()\n  print(t.text)\n  for leg, list in pairs(words) do\n    for _, w in ipairs(list) do\n"
   "      if text:find(w, 1, true) then tag = leg end\n    end\n  end\n  print(t.id .. \" \" .. tag)\nend"},
  {"t1-01-imu", "local ax, ay, az = imu.read()\n-- find the biggest\nprint(math.max(math.abs(ax), math.abs(ay), math.abs(az)))"},
  {"t1-02-i2c-scan", "local found = i2c.scan()\nfor _, addr in ipairs(found) do\n  print(string.format(\"0x%02X\", addr))\nend\nprint(#found)"},
  {"t1-03-gpio-blink", "gpio.mode(2, \"out\")\nfor i = 1, 5 do\n  gpio.write(2, 1)\n  sys.sleep(200)\n  gpio.write(2, 0)\n  sys.sleep(200)\nend\nprint(\"done\")"},
};

// Applies a "Replace the -- line with: <code>" hint to the starter.
static bool applyHint(const std::string& starter, const std::string& hint, std::string& out) {
  const std::string lead = "Replace the -- line with:\n|";
  if (hint.compare(0, lead.size(), lead) != 0) return false;
  std::string code = hint.substr(lead.size());
  size_t from = 0;
  out.clear();
  bool done = false;
  while (from <= starter.size()) {
    size_t nl = starter.find('\n', from);
    if (nl == std::string::npos) nl = starter.size();
    std::string line = starter.substr(from, nl - from);
    size_t ind = line.find_first_not_of(' ');
    if (!done && ind != std::string::npos && line.compare(ind, 2, "--") == 0) { line = line.substr(0, ind) + code; done = true; }
    out += line;
    if (nl < starter.size()) out += '\n';
    from = nl + 1;
  }
  return done;
}

// Lessons whose starter code must not pass as it stands (the learner has
// something to do).
static const char* const kStarterMustFail[] = {
  "t0-02-network-refresher", "t4-01-wifi-recon", "t1-01-imu", "t0-04-tickets-cia", "t0-05-rules-of-engagement", "b1-02-pick-a-network",
  "b1-03-interference-or-jamming", "b1-04-wired-backups",
};

// Checks one content set. `print` lists what was found.
static void checkSet(FakeHal& hal, const char* set, const BuiltinFile* lessons, int nLessons, const BuiltinFile* drills,
                     int nDrills, bool print) {
  std::map<std::string, bool> lessonHasTask;

  for (int i = 0; i < nLessons; i++) {
    Lesson l(&hal);
    std::string err;
    bool ok = l.load(lessons[i].source, err);
    CHECK(ok, "%s lesson %s: %s", set, lessons[i].name, err.c_str());
    if (!ok) continue;
    CHECK(!lessonHasTask.count(l.id()), "lesson id %s used twice", l.id().c_str());
    lessonHasTask[l.id()] = l.hasTask();
    CHECK(!l.pages().empty(), "lesson %s has no pages", l.id().c_str());
    auto it = kSolutions.find(l.id());
    if (it != kSolutions.end()) {
      GradeResult g = l.grade(it->second);
      CHECK(g.passed, "lesson %s: solution did not pass: %s", l.id().c_str(), g.feedback.c_str());
    }
    if (l.hasTask()) {
      std::string code;
      auto hr = kHintResult.find(l.id());
      bool have = hr != kHintResult.end() ? (code = hr->second, true) : applyHint(l.starter(), l.hint(), code);
      CHECK(!l.hint().empty(), "%s lesson %s has no hint", set, l.id().c_str());
      CHECK(have, "%s lesson %s: hint isn't checked (add it to kHintResult)", set, l.id().c_str());
      if (have) {
        GradeResult g = l.grade(code);
        CHECK(g.passed, "%s lesson %s: following the hint doesn't pass: %s", set, l.id().c_str(), g.feedback.c_str());
      }
    }
    for (const char* id : kStarterMustFail) {
      if (l.id() != id) continue;
      GradeResult g = l.grade(l.starter());
      CHECK(!g.passed, "lesson %s: starter code already passes", l.id().c_str());
    }
    for (const std::string& p : l.prereqs())
      CHECK(p != l.id(), "lesson %s requires itself", l.id().c_str());
    if (print)
      std::printf("lesson   %-30s %zu pages, %zu quiz%s%s\n", l.id().c_str(), l.pages().size(), l.quiz().size(),
                  l.labSource().empty() ? "" : ", lab", l.card().empty() ? "" : (", card " + l.card()).c_str());
  }

  for (int i = 0; i < nDrills; i++) {
    Scenario s;
    std::string err;
    bool ok = loadScenario(&hal, drills[i].source, s, err);
    CHECK(ok, "%s drill %s: %s", set, drills[i].name, err.c_str());
    if (!ok) continue;
    for (const Round& r : s.rounds) {
      for (const Defense& d : r.defenses) {
        if (d.ward.empty()) continue;
        CHECK(lessonHasTask.count(d.ward) && lessonHasTask[d.ward], "%s drill %s: ward %s is not a lesson with a task",
              set, s.id.c_str(), d.ward.c_str());
      }
      if (!r.reveal.lesson.empty())
        CHECK(lessonHasTask.count(r.reveal.lesson), "%s drill %s: study link %s is not a lesson", set, s.id.c_str(),
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
          CHECK(lessonHasTask.count(e.freeWith), "%s drill %s: free_with %s is not a lesson", set, s.id.c_str(),
                e.freeWith.c_str());
    if (print)
      std::printf("drill    %-30s %zu rounds, best %d, needs %zu ward(s)\n", s.id.c_str(), s.rounds.size(), g.score(),
                  need.size());
  }
}

int main() {
  FakeHal hal;
  checkSet(hal, "pack", kAll_lessons, kAll_lessons_count, kAll_scenarios, kAll_scenarios_count, true);
  checkSet(hal, "built-in", kBuiltin_lessons, kBuiltin_lessons_count, kBuiltin_scenarios, kBuiltin_scenarios_count, false);
  std::printf("built-in fallback: %d lessons, %d drills\n", kBuiltin_lessons_count, kBuiltin_scenarios_count);

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
  // Progress split into NVS-sized pieces comes back whole.
  {
    Progress big;
    for (int i = 0; i < 200; i++) big.recordTask("lesson-with-a-long-name-" + std::to_string(i), true);
    big.recordScenario("s02-smart-home", 120);
    std::vector<std::string> parts = big.chunks(1000);
    bool fits = parts.size() > 1;
    for (auto& c : parts) fits = fits && c.size() <= 1000;
    CHECK(fits, "progress chunks too big or not split (%zu)", parts.size());
    Progress back;
    back.parseChunks(parts);
    CHECK(back.serialize() == big.serialize(), "progress chunks round trip");
  }

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
    // A loop of sleeps would freeze the device: the sleep budget stops it.
    Sandbox box(&hal, kPermSys);
    RunResult rr = box.run("for i = 1, 100 do sys.sleep(5000) end");
    CHECK(!rr.ok && rr.error.find("sleep limit") != std::string::npos, "sleep loop not stopped: %s", rr.error.c_str());
    rr = box.run("sys.sleep(5000) sys.sleep(5000)");
    CHECK(rr.ok, "sleep budget not reset between runs: %s", rr.error.c_str());
  }

  // A drill where an attack got through is not broken (cleared).
  {
    Scenario s;
    std::string err;
    loadScenario(&hal, "return { id='z', rounds={ { symptom='s', damage={uptime=10}, defenses={ {name='a',result='stop'}, "
                       "{name='b',result='partial'}, {name='c',result='fail'} } }, { symptom='t', defenses={ "
                       "{name='a',result='stop'}, {name='c',result='fail'} } } } }", s, err);
    Game won(s);
    won.defend(1); won.defend(0);
    CHECK(won.finished() && won.broken(), "stop/partial run should break the hex");
    Game lost(s);
    lost.defend(0); lost.defend(1);
    CHECK(lost.finished() && !lost.broken() && lost.held() == 1, "a failed round should leave the hex standing");
  }

  std::printf(fails ? "\n%d check(s) failed\n" : "\nall content checks passed\n", fails);
  return fails ? 1 : 0;
}
