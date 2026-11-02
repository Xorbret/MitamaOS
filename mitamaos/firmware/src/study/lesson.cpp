#include "lesson.h"

#include <cstdio>

#include "content.h"

namespace study {

namespace {

void quote(const char* s, size_t len, std::string& out) {
  out += '"';
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
    else if (c == '\n') out += "\\n";
    else if (c < 0x20) { char b[6]; snprintf(b, sizeof(b), "\\%03u", c); out += b; }
    else out += static_cast<char>(c);
  }
  out += '"';
}

// Writes the table at the top of the stack as a Lua constructor. Only
// strings, numbers, booleans and nested tables are kept.
bool serialize(lua_State* L, int depth, std::string& out) {
  if (depth > 6) return false;
  out += '{';
  int t = lua_absindex(L, -1);
  lua_pushnil(L);
  while (lua_next(L, t) != 0) {
    int kt = lua_type(L, -2), vt = lua_type(L, -1);
    bool keyOk = kt == LUA_TSTRING || kt == LUA_TNUMBER;
    bool valOk = vt == LUA_TSTRING || vt == LUA_TNUMBER || vt == LUA_TBOOLEAN || vt == LUA_TTABLE;
    if (keyOk && valOk) {
      out += '[';
      if (kt == LUA_TSTRING) {
        size_t n = 0;
        const char* k = lua_tolstring(L, -2, &n);
        quote(k, n, out);
      } else {
        lua_pushvalue(L, -2);
        out += lua_tostring(L, -1);
        lua_pop(L, 1);
      }
      out += "]=";
      if (vt == LUA_TTABLE) {
        if (!serialize(L, depth + 1, out)) { lua_pop(L, 2); return false; }
      } else if (vt == LUA_TBOOLEAN) {
        out += lua_toboolean(L, -1) ? "true" : "false";
      } else if (vt == LUA_TSTRING) {
        size_t n = 0;
        const char* v = lua_tolstring(L, -1, &n);
        quote(v, n, out);
      } else {
        lua_pushvalue(L, -1);
        out += lua_tostring(L, -1);
        lua_pop(L, 1);
      }
      out += ',';
    }
    lua_pop(L, 1);
  }
  out += '}';
  return true;
}

}  // namespace

bool Lesson::load(const std::string& source, std::string& err) {
  content_.reset(new Sandbox(hal_, kPermReadOnly));
  lua_State* L = content_->state();
  if (!loadContentTable(*content_, source, "=lesson", err)) return false;

  id_ = fieldString(L, -1, "id");
  title_ = fieldString(L, -1, "title", id_);
  if (id_.empty()) {
    lua_pop(L, 1);
    err = "lesson needs an id";
    return false;
  }
  track_ = static_cast<int>(fieldInt(L, -1, "track"));
  task_ = fieldString(L, -1, "task");
  starter_ = fieldString(L, -1, "starter");
  reflect_ = fieldString(L, -1, "reflect");
  hint_ = fieldString(L, -1, "hint");
  pages_ = fieldStringList(L, -1, "pages");
  prereqs_ = fieldStringList(L, -1, "requires");
  spirit_ = fieldString(L, -1, "spirit");
  taunt_ = fieldString(L, -1, "taunt");
  card_ = fieldString(L, -1, "card");
  field_ = fieldString(L, -1, "field");
  tag_ = fieldString(L, -1, "tag");

  lab_.clear();
  if (lua_getfield(L, -1, "lab") == LUA_TTABLE) {
    std::string src = "lab=";
    if (!serialize(L, 0, src)) {
      lua_pop(L, 2);
      err = "lab table is nested too deeply";
      return false;
    }
    lab_ = src;
  }
  lua_pop(L, 1);

  permissions_ = kPermNone;
  for (const std::string& name : fieldStringList(L, -1, "permissions")) {
    bool ok = false;
    uint32_t bit = permissionFromName(name, ok);
    if (!ok) {
      lua_pop(L, 1);
      err = "unknown permission: " + name;
      return false;
    }
    permissions_ |= bit;
  }

  quiz_.clear();
  forEachItem(L, -1, "quiz", [&](size_t) {
    QuizItem q;
    q.question = fieldString(L, -1, "q");
    q.choices = fieldStringList(L, -1, "choices");
    q.answer = static_cast<int>(fieldInt(L, -1, "answer", 1)) - 1;  // 1-based in Lua
    q.why = fieldString(L, -1, "why");
    if (!q.question.empty() && q.answer >= 0 && q.answer < static_cast<int>(q.choices.size())) {
      quiz_.push_back(q);
    }
  });

  if (lua_getfield(L, -1, "check") == LUA_TFUNCTION) {
    checkRef_ = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    lua_pop(L, 1);
    checkRef_ = LUA_NOREF;
  }
  lua_pop(L, 1);  // lesson table
  return true;
}

GradeResult Lesson::grade(const std::string& learnerCode) {
  GradeResult g;
  {
    Sandbox learner(hal_, permissions_);
    if (!lab_.empty()) {
      RunResult setup = learner.run(lab_, "=lab");
      if (!setup.ok) {
        g.run = setup;
        g.feedback = "Lesson sample data failed to load: " + setup.error;
        return g;
      }
    }
    g.run = learner.run(learnerCode);
  }
  if (checkRef_ == LUA_NOREF || !content_) {
    g.passed = g.run.ok;
    g.feedback = g.run.ok ? "Ran without errors." : g.run.error;
    return g;
  }

  lua_State* L = content_->state();
  content_->beginCall();
  lua_rawgeti(L, LUA_REGISTRYINDEX, checkRef_);
  lua_createtable(L, 0, 3);
  lua_pushboolean(L, g.run.ok);
  lua_setfield(L, -2, "ok");
  lua_pushlstring(L, g.run.output.data(), g.run.output.size());
  lua_setfield(L, -2, "output");
  lua_pushlstring(L, g.run.error.data(), g.run.error.size());
  lua_setfield(L, -2, "error");
  lua_pushlstring(L, learnerCode.data(), learnerCode.size());
  lua_setfield(L, -2, "code");

  int status = lua_pcall(L, 1, 2, 0);
  if (status != LUA_OK) {
    g.passed = false;
    g.feedback = "Lesson check failed: " + content_->endCall(status).error;
    return g;
  }
  g.passed = lua_toboolean(L, -2);
  const char* msg = lua_tostring(L, -1);
  g.feedback = msg != nullptr ? msg : (g.passed ? "Passed." : "Not yet. Try again.");
  lua_pop(L, 2);
  return g;
}

}  // namespace study
