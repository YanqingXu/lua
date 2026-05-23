#ifndef LUA_REPL_META_HPP
#define LUA_REPL_META_HPP

#include "repl.hpp"
#include "repl/repl_ctx.hpp"

namespace Lua::REPL::detail {

int printBytecode(ReplContext& context, LuaState* L, const Str& source, std::ostream& out,
                  std::ostream& err);
int printAst(ReplContext& context, LuaState* L, const Str& source, std::ostream& out,
             std::ostream& err);
int printGc(ReplContext& context, LuaState* L, const Str& argument, std::ostream& out,
            std::ostream& err);
int runMetaCommand(ReplContext& context, LuaState* L, const MetaCommand& command,
                   std::ostream& out, std::ostream& err);

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_META_HPP
