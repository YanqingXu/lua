#ifndef LUA_REPL_COMP_HPP
#define LUA_REPL_COMP_HPP

#include "repl.hpp"

#include <iosfwd>

namespace Lua::REPL::detail {

void applyInteractiveCompletion(LuaState* L, const Str& prompt, Str& line, std::ostream& out);
void applySubmittedTabCompletion(LuaState* L, Str& line, std::ostream& out);

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_COMP_HPP
