/**
 * @file vm_handlers_closure.cpp
 * @brief 闭包与可变参数操作码处理器
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleClosure(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);

    detail::closure(state, context.base, proto, function, context.pc, a, bx);
    return HandlerStatus::Continue;
}

HandlerStatus handleVararg(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    detail::vararg(state, context.base, proto, a, b);
    return HandlerStatus::Continue;
}

}  // namespace

void registerClosureHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::CLOSURE)].handler = handleClosure;
    table[opcodeIndex(OpCode::VARARG)].handler = handleVararg;
}

}  // namespace Lua::VM::handlers
