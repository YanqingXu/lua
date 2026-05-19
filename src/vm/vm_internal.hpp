#pragma once

/**
 * @file vm_internal.hpp
 * @brief Internal VM helpers shared by implementation slices.
 */

#include "common/types.hpp"

namespace Lua {

class LuaState;

namespace VM::detail {

void dispatchCallHook(LuaState* L);
bool precall(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults);
void postcall(LuaState* L, i32 funcPos, i32 wantedResults, usize firstResult = 0);

}  // namespace VM::detail

}  // namespace Lua
