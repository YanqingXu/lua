#ifndef LUA_REPL_PROMPT_HPP
#define LUA_REPL_PROMPT_HPP

/**
 * @file repl_prompt.hpp
 * @brief REPL 提示符生成接口
 */

#include "repl.hpp"

namespace Lua::REPL::detail {

/** @brief 获取首行或续行使用的 REPL 提示符。 */
Str getPrompt(LuaState* L, bool firstLine, usize lineNumber);

} // namespace Lua::REPL::detail

#endif // LUA_REPL_PROMPT_HPP
