/**
 * @file vm_handlers_unary.cpp
 * @brief 一元运算与拼接操作码处理器
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "common/lua_error.hpp"
#include "vm/vm_handlers/vm_diagnostics.hpp"

#include <string>

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleUnaryMinus(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Value val = context.base[b];
    Value result;
    try {
        detail::unaryMinus(state, result, val);
    } catch (const RuntimeError& error) {
        if (std::string(error.what()).find("attempt to perform arithmetic on a non-number value") ==
            std::string::npos) {
            throw;
        }
        Str sourceName = diagnostics::describeRegister(context.proto, b, context.instructionPc).value_or(Str());
        throw RuntimeError(diagnostics::formatTypeActionError("perform arithmetic on", val, sourceName));
    }
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleNot(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    context.base[a] = Value(!context.base[b].isTrue());
    return HandlerStatus::Continue;
}

HandlerStatus handleLength(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Value val = context.base[b];
    Value result;
    detail::length(state, result, val);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleConcat(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    detail::concat(context.services, state, context.base, a, b, c);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

} // namespace

void registerUnaryHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::UNM)].handler = handleUnaryMinus;
    table[opcodeIndex(OpCode::NOT)].handler = handleNot;
    table[opcodeIndex(OpCode::LEN)].handler = handleLength;
    table[opcodeIndex(OpCode::CONCAT)].handler = handleConcat;
}

} // namespace Lua::VM::handlers
