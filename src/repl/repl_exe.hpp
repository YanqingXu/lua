#ifndef LUA_REPL_EXE_HPP
#define LUA_REPL_EXE_HPP

/**
 * @file repl_exe.hpp
 * @brief REPL 源码编译与执行接口
 */

#include "compiler/ast.hpp"
#include "common/lua_error.hpp"
#include "repl/repl_ctx.hpp"

#include <expected>
#include <iosfwd>

namespace Lua {

class LuaState;

namespace REPL::detail {

/** @brief 等待执行的已解析代码块及其源码形态。 */
struct PreparedInput {
    Chunk chunk;
    Str source;
    bool isExpression = false;
};

/** @brief 尝试将输入改写为表达式返回语句。 */
Str tryAsExpression(const Str& source, bool& wasExplicitReturn);
/** @brief 判断语法错误是否表示输入尚未完成。 */
bool isIncompleteInput(const Str& errorMessage);

/** @brief 解析并准备一段 REPL 输入供后续执行。 */
std::expected<PreparedInput, ParseError> prepareInputForExecution(LuaState* L, const Str& source, bool isExpression);
/** @brief 执行已准备输入并输出结果或错误。 */
int executePreparedInput(ReplContext& context, LuaState* L, PreparedInput&& input, std::ostream& out,
                         std::ostream& err);

} // namespace REPL::detail
} // namespace Lua

#endif // LUA_REPL_EXE_HPP
