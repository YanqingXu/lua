#ifndef LUA_REPL_META_HPP
#define LUA_REPL_META_HPP

/**
 * @file repl_meta.hpp
 * @brief REPL 元命令解析与执行接口
 */

#include "repl.hpp"
#include "repl/repl_ctx.hpp"

namespace Lua::REPL::detail {

/** @brief 编译并打印源码对应的字节码。 */
int printBytecode(ReplContext& context, LuaState* L, const Str& source, std::ostream& out,
                  std::ostream& err);
/** @brief 解析并打印源码对应的 AST。 */
int printAst(ReplContext& context, LuaState* L, const Str& source, std::ostream& out,
             std::ostream& err);
/** @brief 打印或触发垃圾回收相关操作。 */
int printGc(ReplContext& context, LuaState* L, const Str& argument, std::ostream& out,
            std::ostream& err);
/** @brief 执行一条已解析的 REPL 元命令。 */
int runMetaCommand(ReplContext& context, LuaState* L, const MetaCommand& command,
                   std::ostream& out, std::ostream& err);

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_META_HPP
