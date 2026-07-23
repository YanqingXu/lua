#pragma once

/**
 * @file vm_switch_dispatch.hpp
 * @brief 虚拟机基于 switch 语句的调度后端操作码专用内联入口
 */

#include "vm/vm_handlers.hpp"

#include <utility>

namespace Lua::VM::detail {

using SwitchOpHandler = HandlerStatus (*)(OpExecutionContext& context, Instruction inst);

inline HandlerStatus execOpMove(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpLoadK(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpLoadBool(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpLoadNil(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpGetUpval(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpGetGlobal(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpGetTable(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpSetGlobal(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpSetUpval(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpSetTable(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpNewTable(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpSelf(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpAdd(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpSub(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpMul(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpDiv(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpMod(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpPow(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpUnm(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpNot(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpLen(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpConcat(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpJmp(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpEq(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpLt(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpLe(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpTest(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpTestSet(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpCall(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpTailCall(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpReturn(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpForLoop(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpForPrep(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpTForLoop(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpSetList(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpClose(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpClosure(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

inline HandlerStatus execOpVararg(OpExecutionContext& context, Instruction inst) {
    return runHandler(context, inst);
}

/** @brief switch 调度后端使用的操作码处理器条目。 */
struct SwitchHandlerEntry {
    OpCode opcode;
    SwitchOpHandler handler;
};

inline constexpr std::array<SwitchHandlerEntry, static_cast<usize>(NUM_OPCODES)> kSwitchHandlers = {{
    {OpCode::MOVE, execOpMove},
    {OpCode::LOADK, execOpLoadK},
    {OpCode::LOADBOOL, execOpLoadBool},
    {OpCode::LOADNIL, execOpLoadNil},
    {OpCode::GETUPVAL, execOpGetUpval},
    {OpCode::GETGLOBAL, execOpGetGlobal},
    {OpCode::GETTABLE, execOpGetTable},
    {OpCode::SETGLOBAL, execOpSetGlobal},
    {OpCode::SETUPVAL, execOpSetUpval},
    {OpCode::SETTABLE, execOpSetTable},
    {OpCode::NEWTABLE, execOpNewTable},
    {OpCode::SELF, execOpSelf},
    {OpCode::ADD, execOpAdd},
    {OpCode::SUB, execOpSub},
    {OpCode::MUL, execOpMul},
    {OpCode::DIV, execOpDiv},
    {OpCode::MOD, execOpMod},
    {OpCode::POW, execOpPow},
    {OpCode::UNM, execOpUnm},
    {OpCode::NOT, execOpNot},
    {OpCode::LEN, execOpLen},
    {OpCode::CONCAT, execOpConcat},
    {OpCode::JMP, execOpJmp},
    {OpCode::EQ, execOpEq},
    {OpCode::LT, execOpLt},
    {OpCode::LE, execOpLe},
    {OpCode::TEST, execOpTest},
    {OpCode::TESTSET, execOpTestSet},
    {OpCode::CALL, execOpCall},
    {OpCode::TAILCALL, execOpTailCall},
    {OpCode::RETURN, execOpReturn},
    {OpCode::FORLOOP, execOpForLoop},
    {OpCode::FORPREP, execOpForPrep},
    {OpCode::TFORLOOP, execOpTForLoop},
    {OpCode::SETLIST, execOpSetList},
    {OpCode::CLOSE, execOpClose},
    {OpCode::CLOSURE, execOpClosure},
    {OpCode::VARARG, execOpVararg},
}};

static_assert(kSwitchHandlers.size() == static_cast<usize>(NUM_OPCODES),
              "switch dispatch handler table must cover every opcode");

consteval bool switchHandlersMatchOpcodeOrder() {
    for (usize index = 0; index < kSwitchHandlers.size(); ++index) {
        if (kSwitchHandlers[index].opcode != static_cast<OpCode>(index)) {
            return false;
        }
        if (kSwitchHandlers[index].handler == nullptr) {
            return false;
        }
    }
    return true;
}

static_assert(switchHandlersMatchOpcodeOrder(),
              "kSwitchHandlers must stay in exact OpCode order and contain non-null handlers");

inline Opt<SwitchOpHandler> switchHandlerFor(OpCode op) noexcept {
    const usize index = static_cast<usize>(op);
    if (index >= kSwitchHandlers.size()) {
        return std::nullopt;
    }

    const SwitchHandlerEntry& entry = kSwitchHandlers[index];
    if (entry.opcode != op) {
        return std::nullopt;
    }
    return entry.handler;
}

} // namespace Lua::VM::detail
