/**
 * @file vm_handlers_global_upvalue.cpp
 * @brief Global and upvalue opcode handlers.
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleGetGlobal(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);
    const Value& key = context.proto->getConstant(bx);

    Table* env = function->getEnv();
    if (!env) {
        env = state->getGlobalTable();
    }

    Value result;
    detail::gettable(state, Value(env), key, result);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleSetGlobal(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);
    const Value& key = context.proto->getConstant(bx);

    Table* env = function->getEnv();
    if (!env) {
        env = state->getGlobalTable();
    }

    Value val = context.base[a];
    detail::settable(state, Value(env), key, val);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleGetUpval(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Upvalue* uv = function->getUpvalue(b);
    if (!uv) {
        throw RuntimeError("VM: GETUPVAL invalid upvalue index");
    }
    context.base[a] = uv->getValue(state->getStack());
    return HandlerStatus::Continue;
}

HandlerStatus handleSetUpval(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Upvalue* uv = function->getUpvalue(b);
    if (!uv) {
        throw RuntimeError("VM: SETUPVAL invalid upvalue index");
    }
    uv->setValue(state->getStack(), context.base[a]);
    return HandlerStatus::Continue;
}

}  // namespace

void registerGlobalUpvalueHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::GETGLOBAL)].handler = handleGetGlobal;
    table[opcodeIndex(OpCode::SETGLOBAL)].handler = handleSetGlobal;
    table[opcodeIndex(OpCode::GETUPVAL)].handler = handleGetUpval;
    table[opcodeIndex(OpCode::SETUPVAL)].handler = handleSetUpval;
}

}  // namespace Lua::VM::handlers
