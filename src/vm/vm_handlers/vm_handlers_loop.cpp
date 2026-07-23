/**
 * @file vm_handlers_loop.cpp
 * @brief 循环与关闭操作码处理器
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "core/gc_string.hpp"
#include "vm/state/call_info.hpp"

#include <cctype>
#include <cstdlib>

namespace Lua::VM::handlers {

namespace {

bool coerceNumber(Value& value) {
    if (value.isNumber()) {
        return true;
    }

    if (!value.isString()) {
        return false;
    }

    const char* text = value.asString()->c_str();
    char* end = nullptr;
    f64 number = std::strtod(text, &end);
    if (end == text) {
        return false;
    }
    while (std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (*end != '\0') {
        return false;
    }

    value = Value(number);
    return true;
}

HandlerStatus handleClose(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);

    const CallInfo& ci = state->getCurrentCallInfo();
    state->closeUpvalues(ci.base + static_cast<usize>(a));
    return HandlerStatus::Continue;
}

HandlerStatus handleForLoop(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);

    if (!context.base[a].isNumber() || !context.base[a + 1].isNumber() || !context.base[a + 2].isNumber()) {
        throw RuntimeError("VM: FORLOOP requires numeric values");
    }

    f64 step = context.base[a + 2].asNumber();
    f64 idx = context.base[a].asNumber() + step;
    f64 limit = context.base[a + 1].asNumber();

    bool cont = (step > 0) ? (idx <= limit) : (idx >= limit);
    if (cont) {
        context.pc += GETARG_sBx(inst);
        context.base[a] = Value(idx);
        context.base[a + 3] = Value(idx);
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleForPrep(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);

    if (!coerceNumber(context.base[a]) || !coerceNumber(context.base[a + 1]) || !coerceNumber(context.base[a + 2])) {
        throw RuntimeError("VM: FORPREP requires numeric values");
    }

    f64 init = context.base[a].asNumber();
    f64 step = context.base[a + 2].asNumber();
    context.base[a] = Value(init - step);
    context.pc += GETARG_sBx(inst);
    return HandlerStatus::Continue;
}

HandlerStatus handleTForLoop(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 c = GETARG_C(inst);

    detail::tforLoop(state, context.base, proto, context.pc, a, c);
    return HandlerStatus::Continue;
}

} // namespace

void registerLoopHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::CLOSE)].handler = handleClose;
    table[opcodeIndex(OpCode::FORLOOP)].handler = handleForLoop;
    table[opcodeIndex(OpCode::FORPREP)].handler = handleForPrep;
    table[opcodeIndex(OpCode::TFORLOOP)].handler = handleTForLoop;
}

} // namespace Lua::VM::handlers
