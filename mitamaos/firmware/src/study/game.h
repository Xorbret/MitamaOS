// Harae drills (from CardLab's "Defend the Network"): a turn-based blue-team
// game. Every attack is simulated on screen; nothing is transmitted.
//
// Fair by rule: every round must have at least one response that stops the
// attack, or the file is refused. A response can need a Ward (the id of a
// Shugyo lesson); a drill whose rounds can't all be stopped with the Wards
// the player holds is sealed, never offered as an unwinnable board.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "hal.h"

namespace study {

enum class Outcome { Stopped, Partial, Failed };

struct Evidence {
  std::string name;
  int cost = 1;
  std::string text;
  std::string freeWith;  // lesson id whose card (a Blessing) makes this free
};

struct Defense {
  std::string name;
  Outcome result = Outcome::Failed;
  std::string text;  // why it worked or did not
  std::string ward;  // lesson id that teaches this response ("" = always known)
};

struct Reveal {
  std::string attack;     // e.g. "Deauthentication attack"
  std::string target;     // what it goes after, e.g. "802.11 management frames"
  std::string cia;        // Confidentiality / Integrity / Availability
  std::string technique;  // optional framework reference
  std::string lesson;     // lesson id to study next
};

struct Round {
  std::string symptom;
  int uptimeDamage = 0;
  int dataDamage = 0;
  std::vector<Evidence> evidence;
  std::vector<Defense> defenses;
  Reveal reveal;
};

struct Scenario {
  std::string id;
  std::string title;
  std::string briefing;
  std::string faction;  // Harae faction, e.g. "Static"
  std::string demon;    // the demon behind the drill
  int pointsPerRound = 3;
  // Babel's Surge: each action point spent on evidence also costs this much
  // uptime. Files where reading everything could take uptime to zero are
  // refused, so a careful player can never lose to Surge alone.
  int surge = 0;
  std::vector<Round> rounds;
};

// Parses a scenario file (a Lua file returning a table).
bool loadScenario(Hal* hal, const std::string& source, Scenario& out, std::string& err);

class Game {
 public:
  explicit Game(Scenario scenario);

  const Scenario& scenario() const { return scenario_; }
  bool finished() const { return round_ >= scenario_.rounds.size(); }
  size_t roundIndex() const { return round_; }
  const Round& round() const { return scenario_.rounds[round_]; }

  int uptime() const { return uptime_; }
  int data() const { return data_; }
  int pointsLeft() const { return points_; }
  int score() const { return score_; }

  bool seen(size_t evidenceIndex) const;

  // Which cards the player holds, for evidence a Blessing makes free.
  void setWards(std::function<bool(const std::string&)> has) { has_ = std::move(has); }
  int evidenceCost(size_t evidenceIndex) const;

  // Spends action points to look at a piece of evidence. Re-reading is
  // free. Returns false if the index is bad or points run out.
  bool investigate(size_t evidenceIndex);

  // Chooses a response, applies damage, scores the round and moves on.
  // The finished round stays readable through lastRound() for the reveal.
  Outcome defend(size_t defenseIndex);
  const Round* lastRound() const { return last_; }
  const Defense* lastDefense() const { return lastDefense_; }

 private:
  void startRound();

  Scenario scenario_;
  size_t round_ = 0;
  int uptime_ = 100;
  int data_ = 100;
  int points_ = 0;
  int score_ = 0;
  std::vector<bool> seen_;
  const Round* last_ = nullptr;
  const Defense* lastDefense_ = nullptr;
  std::function<bool(const std::string&)> has_;
};

const char* outcomeName(Outcome o);

// Wards a drill needs before it can be won: for each round with no stopping
// response open to the player, the ward of its first stopping response.
// Empty means the drill is fair to play now.
template <typename HasWard>
std::vector<std::string> missingWards(const Scenario& s, HasWard has) {
  std::vector<std::string> out;
  for (const Round& r : s.rounds) {
    const Defense* sealed = nullptr;
    bool open = false;
    for (const Defense& d : r.defenses) {
      if (d.result != Outcome::Stopped) continue;
      if (d.ward.empty() || has(d.ward)) { open = true; break; }
      if (!sealed) sealed = &d;
    }
    if (!open && sealed) {
      bool dup = false;
      for (const std::string& w : out) dup = dup || w == sealed->ward;
      if (!dup) out.push_back(sealed->ward);
    }
  }
  return out;
}

}  // namespace study
