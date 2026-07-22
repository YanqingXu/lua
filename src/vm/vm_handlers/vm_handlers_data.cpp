/**
 * @file vm_handlers_data.cpp
 * @brief 数据移动操作码处理器
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "core/gc_string.hpp"

#include <cstdio>

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleMove(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    if (detail::shouldDumpBytecode(context.state)) {
        std::fprintf(stderr, "[MOVE] pc=%zu a=%d b=%d base[b]=", context.instructionPc, a, b);
        if (context.base[b].isNumber()) std::fprintf(stderr, "%g", context.base[b].asNumber());
        else if (context.base[b].isNil()) std::fprintf(stderr, "nil");
        else if (context.base[b].isFunction()) std::fprintf(stderr, "function");
        else if (context.base[b].isString()) std::fprintf(stderr, "'%s'", context.base[b].asString()->c_str());
        else std::fprintf(stderr, "other");
        std::fprintf(stderr, "\n");
    }

    context.base[a] = context.base[b];
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadK(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);

    context.base[a] = context.proto->getConstant(bx);
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadBool(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    context.base[a] = Value(b != 0);
    if (c != 0) {
        context.pc++;
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadNil(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    for (i32 i = a; i <= b; i++) {
        context.base[i] = Value();
    }
    return HandlerStatus::Continue;
}

}  // namespace

void registerDataHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::MOVE)].handler = handleMove;
    table[opcodeIndex(OpCode::LOADK)].handler = handleLoadK;
    table[opcodeIndex(OpCode::LOADBOOL)].handler = handleLoadBool;
    table[opcodeIndex(OpCode::LOADNIL)].handler = handleLoadNil;
}

}  // namespace Lua::VM::handlers
