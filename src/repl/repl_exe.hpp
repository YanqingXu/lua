#ifndef LUA_REPL_EXE_HPP
#define LUA_REPL_EXE_HPP

#include "compiler/ast.hpp"
#include "common/lua_error.hpp"
#include "repl/repl_ctx.hpp"

#include <expected>
#include <iosfwd>

namespace Lua {

class LuaState;

namespace REPL::detail {

struct PreparedInput {
    Chunk chunk;
    Str source;
    bool isExpression = false;
};

Str tryAsExpression(const Str& source, bool& wasExplicitReturn);
bool isIncompleteInput(const Str& errorMessage);

std::expected<PreparedInput, ParseError> prepareInputForExecution(LuaState* L, const Str& source,
                                                                 bool isExpression);
int executePreparedInput(ReplContext& context, LuaState* L, PreparedInput&& input,
                         std::ostream& out, std::ostream& err);

}  // namespace REPL::detail
}  // namespace Lua

#endif  // LUA_REPL_EXE_HPP
