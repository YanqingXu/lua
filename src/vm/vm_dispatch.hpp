#pragma once

/**
 * @file vm_dispatch.hpp
 * @brief Opcode grouping helpers for VM dispatch refactoring.
 */

#include "compiler/opcode.hpp"

namespace Lua::VM {

enum class OpcodeGroup : u8 {
    Unknown,
    DataMove,
    Global,
    Upvalue,
    Table,
    Arithmetic,
    Unary,
    Branch,
    Comparison,
    Call,
    Loop,
    Closure,
    Vararg,
};

inline OpcodeGroup opcodeGroup(OpCode op) {
    switch (op) {
        case OpCode::MOVE:
        case OpCode::LOADK:
        case OpCode::LOADBOOL:
        case OpCode::LOADNIL:
            return OpcodeGroup::DataMove;

        case OpCode::GETGLOBAL:
        case OpCode::SETGLOBAL:
            return OpcodeGroup::Global;

        case OpCode::GETUPVAL:
        case OpCode::SETUPVAL:
            return OpcodeGroup::Upvalue;

        case OpCode::GETTABLE:
        case OpCode::SETTABLE:
        case OpCode::NEWTABLE:
        case OpCode::SELF:
        case OpCode::SETLIST:
            return OpcodeGroup::Table;

        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::POW:
            return OpcodeGroup::Arithmetic;

        case OpCode::UNM:
        case OpCode::NOT:
        case OpCode::LEN:
        case OpCode::CONCAT:
            return OpcodeGroup::Unary;

        case OpCode::JMP:
        case OpCode::TEST:
        case OpCode::TESTSET:
        case OpCode::CLOSE:
            return OpcodeGroup::Branch;

        case OpCode::EQ:
        case OpCode::LT:
        case OpCode::LE:
            return OpcodeGroup::Comparison;

        case OpCode::CALL:
        case OpCode::TAILCALL:
        case OpCode::RETURN:
            return OpcodeGroup::Call;

        case OpCode::FORLOOP:
        case OpCode::FORPREP:
        case OpCode::TFORLOOP:
            return OpcodeGroup::Loop;

        case OpCode::CLOSURE:
            return OpcodeGroup::Closure;

        case OpCode::VARARG:
            return OpcodeGroup::Vararg;
    }

    return OpcodeGroup::Unknown;
}

inline bool isDataMoveOpcode(OpCode op) {
    return opcodeGroup(op) == OpcodeGroup::DataMove;
}

inline bool isTableOpcode(OpCode op) {
    return opcodeGroup(op) == OpcodeGroup::Table;
}

inline bool isArithmeticOpcode(OpCode op) {
    return opcodeGroup(op) == OpcodeGroup::Arithmetic;
}

inline bool isComparisonOpcode(OpCode op) {
    return opcodeGroup(op) == OpcodeGroup::Comparison;
}

inline bool isCallOpcode(OpCode op) {
    return opcodeGroup(op) == OpcodeGroup::Call;
}

inline bool mayInvokeMetamethod(OpCode op) {
    switch (op) {
        case OpCode::GETTABLE:
        case OpCode::SETTABLE:
        case OpCode::SELF:
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::POW:
        case OpCode::UNM:
        case OpCode::LEN:
        case OpCode::CONCAT:
        case OpCode::EQ:
        case OpCode::LT:
        case OpCode::LE:
        case OpCode::CALL:
        case OpCode::TAILCALL:
            return true;
        default:
            return false;
    }
}

}  // namespace Lua::VM
