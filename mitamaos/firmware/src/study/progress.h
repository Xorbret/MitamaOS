// Learner progress: one line per lesson and drill, kept in NVS "study" and
// mirrored to settings.conf, exportable as JSON for a portfolio. A passed
// lesson task is also the Harae Ward that lesson teaches.
#pragma once

#include <map>
#include <string>

namespace study {

struct LessonRecord {
  bool taskPassed = false;
  int quizScore = 0;
  int quizTotal = 0;
  bool fieldDone = false;  // the optional field task, done on the learner's own gear

  bool quizPerfect() const { return quizTotal > 0 && quizScore == quizTotal; }
  // Three marks: task passed, perfect quiz, field task.
  int marks() const { return (taskPassed ? 1 : 0) + (quizPerfect() ? 1 : 0) + (fieldDone ? 1 : 0); }
};

class Progress {
 public:
  void recordTask(const std::string& lessonId, bool passed);
  void recordQuiz(const std::string& lessonId, int score, int total);
  void recordField(const std::string& lessonId);
  void recordScenario(const std::string& scenarioId, int score);

  const LessonRecord* lesson(const std::string& lessonId) const;
  int bestScenarioScore(const std::string& scenarioId) const;  // -1 if never played
  size_t lessonsPassed() const;
  bool hasWard(const std::string& lessonId) const {
    const LessonRecord* r = lesson(lessonId);
    return r != nullptr && r->taskPassed;
  }

  // Line format: "L|id|passed|score|total|field" and "S|id|score".
  // Older lines without the field flag still parse.
  std::string serialize() const;
  void parse(const std::string& text);
  std::string toJson() const;

 private:
  std::map<std::string, LessonRecord> lessons_;
  std::map<std::string, int> scenarios_;
};

}  // namespace study
