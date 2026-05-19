#pragma once

/**
 * @file vm_internal.hpp
 * @brief Internal VM helpers shared by implementation slices.
 */

#include "common/types.hpp"
#include "compiler/opcode.hpp"
#include "core/value.hpp"
#include "runtime/runtime_services.hpp"

namespace Lua {

class LuaState;

namespace VM::detail {

void dispatchCallHook(LuaState* L);
void dispatchReturnHook(LuaState* L);
bool shouldDumpBytecode();

void gettable(LuaState* L, Value t, const Value& key, Value& result);
void settable(LuaState* L, Value t, const Value& key, const Value& val);
void arith(LuaState* L, Value& result, const Value& left, const Value& right, OpCode op);
bool equal(LuaState* L, const Value& left, const Value& right);
bool lessThan(LuaState* L, const Value& left, const Value& right);
bool lessEqual(LuaState* L, const Value& left, const Value& right);
void unaryMinus(LuaState* L, Value& result, const Value& val);
void length(LuaState* L, Value& result, const Value& val);
void concat(RuntimeServices& services, LuaState* L, Value* base, i32 a, i32 b, i32 c);

bool precall(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults);
void postcall(LuaState* L, i32 funcPos, i32 wantedResults, usize firstResult = 0);
void reuseCurrentFrameForTailCall(LuaState* L, usize callerIndex, usize callerFunc, i32 callerTailcalls);

}  // namespace VM::detail

}  // namespace Lua
