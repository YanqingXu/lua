#pragma once

/**
 * @file vm_switch_dispatch.hpp
 * @brief Opcode-specific inline entry points for the VM switch dispatch backend.
 */

#include "vm/vm_handlers.hpp"

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

inline SwitchOpHandler switchHandlerFor(OpCode op) noexcept {
    switch (op) {
        case OpCode::MOVE: return execOpMove;
        case OpCode::LOADK: return execOpLoadK;
        case OpCode::LOADBOOL: return execOpLoadBool;
        case OpCode::LOADNIL: return execOpLoadNil;
        case OpCode::GETUPVAL: return execOpGetUpval;
        case OpCode::GETGLOBAL: return execOpGetGlobal;
        case OpCode::GETTABLE: return execOpGetTable;
        case OpCode::SETGLOBAL: return execOpSetGlobal;
        case OpCode::SETUPVAL: return execOpSetUpval;
        case OpCode::SETTABLE: return execOpSetTable;
        case OpCode::NEWTABLE: return execOpNewTable;
        case OpCode::SELF: return execOpSelf;
        case OpCode::ADD: return execOpAdd;
        case OpCode::SUB: return execOpSub;
        case OpCode::MUL: return execOpMul;
        case OpCode::DIV: return execOpDiv;
        case OpCode::MOD: return execOpMod;
        case OpCode::POW: return execOpPow;
        case OpCode::UNM: return execOpUnm;
        case OpCode::NOT: return execOpNot;
        case OpCode::LEN: return execOpLen;
        case OpCode::CONCAT: return execOpConcat;
        case OpCode::JMP: return execOpJmp;
        case OpCode::EQ: return execOpEq;
        case OpCode::LT: return execOpLt;
        case OpCode::LE: return execOpLe;
        case OpCode::TEST: return execOpTest;
        case OpCode::TESTSET: return execOpTestSet;
        case OpCode::CALL: return execOpCall;
        case OpCode::TAILCALL: return execOpTailCall;
        case OpCode::RETURN: return execOpReturn;
        case OpCode::FORLOOP: return execOpForLoop;
        case OpCode::FORPREP: return execOpForPrep;
        case OpCode::TFORLOOP: return execOpTForLoop;
        case OpCode::SETLIST: return execOpSetList;
        case OpCode::CLOSE: return execOpClose;
        case OpCode::CLOSURE: return execOpClosure;
        case OpCode::VARARG: return execOpVararg;
    }
    return nullptr;
}

}  // namespace Lua::VM::detail
