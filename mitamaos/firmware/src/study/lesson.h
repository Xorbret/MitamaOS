// A lesson: reading pages, a code task graded by the lesson's own check(),
// a short quiz and a reflection prompt. See content/README.md for the format.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "sandbox.h"

namespace study {

struct QuizItem {
  std::string question;
  std::vector<std::string> choices;
  int answer = 0;  // 0-based index into choices
  std::string why;
};

struct GradeResult {
  bool passed = false;
  std::string feedback;
  RunResult run;  // the learner's run, for showing output and errors
};

class Lesson {
 public:
  explicit Lesson(Hal* hal) : hal_(hal) {}

  bool load(const std::string& source, std::string& err);

  // Runs the learner's code in a fresh sandbox with only the permissions
  // this lesson asked for, then passes the result to the lesson's check().
  GradeResult grade(const std::string& learnerCode);

  const std::string& id() const { return id_; }
  const std::string& title() const { return title_; }
  int track() const { return track_; }
  const std::vector<std::string>& pages() const { return pages_; }
  const std::string& task() const { return task_; }
  const std::string& starter() const { return starter_; }
  const std::vector<QuizItem>& quiz() const { return quiz_; }
  const std::string& reflect() const { return reflect_; }
  const std::string& hint() const { return hint_; }
  uint32_t permissions() const { return permissions_; }
  bool hasTask() const { return checkRef_ != LUA_NOREF; }

  // Trial extras (all optional). `requires` (prereqs()) lists lesson ids that must be
  // passed first. `spirit` and `taunt` add a closing page where the
  // faction's spirit speaks. `card` names the Harae card the task earns,
  // `field` is a task on the learner's own gear (honour system, never
  // needed for the card), `tag` is a cert-domain label.
  const std::vector<std::string>& prereqs() const { return prereqs_; }
  const std::string& spirit() const { return spirit_; }
  const std::string& taunt() const { return taunt_; }
  const std::string& card() const { return card_; }
  const std::string& field() const { return field_; }
  const std::string& tag() const { return tag_; }
  // The lesson's `lab` table as Lua source, given to learner code as the
  // global `lab` (sample data, so tasks never need a live network).
  const std::string& labSource() const { return lab_; }

 private:
  Hal* hal_;
  std::unique_ptr<Sandbox> content_;
  int checkRef_ = LUA_NOREF;
  std::string id_, title_, task_, starter_, reflect_, hint_;
  std::string spirit_, taunt_, card_, field_, tag_, lab_;
  std::vector<std::string> prereqs_;
  int track_ = 0;
  uint32_t permissions_ = kPermNone;
  std::vector<std::string> pages_;
  std::vector<QuizItem> quiz_;
};

}  // namespace study
