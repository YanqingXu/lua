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
class Function;
class Proto;

namespace VM::detail {

void dispatchCallHook(LuaState* L);
void dispatchReturnHook(LuaState* L);
void dispatchCountHook(LuaState* L);
void dispatchLineHook(LuaState* L, Proto* proto, usize pc);
bool shouldDumpBytecode();
void emitInstructionTrace(Proto* proto, Value* base, usize instructionPc, Instruction inst, i32 callDepth);
void emitCallTrace(Proto* proto, Value* base, usize instructionPc, i32 registerIndex, i32 callDepth);
void emitReturnTrace(i32 callDepth);

void gettable(LuaState* L, Value t, const Value& key, Value& result);
void settable(LuaState* L, Value t, const Value& key, const Value& val);
void arith(LuaState* L, Value& result, const Value& left, const Value& right, OpCode op);
void execArithmetic(LuaState* L, Proto* proto, Value*& base, i32 a, i32 b, i32 c, OpCode op);
bool equal(LuaState* L, const Value& left, const Value& right);
bool lessThan(LuaState* L, const Value& left, const Value& right);
bool lessEqual(LuaState* L, const Value& left, const Value& right);
void unaryMinus(LuaState* L, Value& result, const Value& val);
void length(LuaState* L, Value& result, const Value& val);
void concat(RuntimeServices& services, LuaState* L, Value* base, i32 a, i32 b, i32 c);

bool precall(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults);
void postcall(LuaState* L, i32 funcPos, i32 wantedResults, usize firstResult = 0);
void reuseCurrentFrameForTailCall(LuaState* L, usize callerIndex, usize callerFunc, i32 callerTailcalls);

void setList(LuaState* L, Value* base, i32 a, i32 b, i32 c);
void closure(LuaState* L, Value* base, Proto* currentProto, Function* currentFunc,
             usize& pc, i32 a, i32 bx);
void vararg(LuaState* L, Value*& base, Proto* proto, i32 a, i32 b);
void tforLoop(LuaState* L, Value*& base, Proto* proto, usize& pc, i32 a, i32 c);

}  // namespace VM::detail

}  // namespace Lua
