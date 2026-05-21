/**
 * @file vm_handlers_branch.cpp
 * @brief Comparison and branch opcode handlers.
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleJump(OpExecutionContext& context, Instruction inst) {
    context.pc += GETARG_sBx(inst);
    return HandlerStatus::Continue;
}

HandlerStatus handleComparison(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    Value left = getRK(context, b);
    Value right = getRK(context, c);
    bool result = false;

    switch (GET_OPCODE(inst)) {
        case OpCode::EQ:
            result = detail::equal(state, left, right);
            break;
        case OpCode::LT:
            result = detail::lessThan(state, left, right);
            break;
        case OpCode::LE:
            result = detail::lessEqual(state, left, right);
            break;
        default:
            throw RuntimeError("VM: invalid comparison handler opcode");
    }

    context.base = refreshBase(state);
    if (result != (a != 0)) {
        context.pc++;
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleTest(OpExecutionContext& context, Instruction inst) {
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 c = GETARG_C(inst);

    bool val = context.base[a].isTrue();
    if ((!val) != (c != 0)) {
        const auto code = proto->getInstructionSpan();
        if (context.pc < code.size()) {
            context.pc += GETARG_sBx(code[context.pc]);
        }
    }
    context.pc++;
    return HandlerStatus::Continue;
}

HandlerStatus handleTestSet(OpExecutionContext& context, Instruction inst) {
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    bool val = context.base[b].isTrue();
    if ((!val) != (c != 0)) {
        context.base[a] = context.base[b];
        const auto code = proto->getInstructionSpan();
        if (context.pc < code.size()) {
            context.pc += GETARG_sBx(code[context.pc]);
        }
    }
    context.pc++;
    return HandlerStatus::Continue;
}

}  // namespace

void registerBranchHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::JMP)].handler = handleJump;
    table[opcodeIndex(OpCode::EQ)].handler = handleComparison;
    table[opcodeIndex(OpCode::LT)].handler = handleComparison;
    table[opcodeIndex(OpCode::LE)].handler = handleComparison;
    table[opcodeIndex(OpCode::TEST)].handler = handleTest;
    table[opcodeIndex(OpCode::TESTSET)].handler = handleTestSet;
}

}  // namespace Lua::VM::handlers
