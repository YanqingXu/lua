#ifndef LUA_REPL_COMP_HPP
#define LUA_REPL_COMP_HPP

/**
 * @file repl_comp.hpp
 * @brief REPL 交互式补全接口
 */

#include "repl.hpp"

#include <iosfwd>

namespace Lua::REPL::detail {

/** @brief 对终端中的当前输入行应用交互式补全。 */
void applyInteractiveCompletion(LuaState* L, const Str& prompt, Str& line, std::ostream& out);
/** @brief 处理已提交输入行中的制表符补全。 */
void applySubmittedTabCompletion(LuaState* L, Str& line, std::ostream& out);

} // namespace Lua::REPL::detail

#endif // LUA_REPL_COMP_HPP
