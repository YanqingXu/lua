#ifndef LUA_REPL_HISTORY_HPP
#define LUA_REPL_HISTORY_HPP

#include "repl.hpp"

namespace Lua::REPL {

void recordHistory(Vec<Str>& history, const Str& line);
bool loadHistory(const Str& path, Vec<Str>& history);
bool saveHistory(const Str& path, const Vec<Str>& history);

}  // namespace Lua::REPL

#endif  // LUA_REPL_HISTORY_HPP
