#ifndef LUA_REPL_PROMPT_HPP
#define LUA_REPL_PROMPT_HPP

#include "repl.hpp"

namespace Lua::REPL::detail {

Str formatLinePrompt(usize lineNumber, bool firstLine);
Str getPrompt(LuaState* L, bool firstLine, usize lineNumber);

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_PROMPT_HPP
