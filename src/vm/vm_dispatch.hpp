#pragma once

/**
 * @file vm_dispatch.hpp
 * @brief Opcode grouping helpers for VM dispatch refactoring.
 */

#include "compiler/opcode.hpp"

namespace Lua::VM {

inline constexpr OpcodeGroup opcodeGroup(OpCode op) noexcept {
    return opcodeMetadata(op).group;
}

inline constexpr bool isDataMoveOpcode(OpCode op) noexcept {
    return opcodeGroup(op) == OpcodeGroup::DataMove;
}

inline constexpr bool isTableOpcode(OpCode op) noexcept {
    return opcodeGroup(op) == OpcodeGroup::Table;
}

inline constexpr bool isArithmeticOpcode(OpCode op) noexcept {
    return opcodeGroup(op) == OpcodeGroup::Arithmetic;
}

inline constexpr bool isComparisonOpcode(OpCode op) noexcept {
    return opcodeGroup(op) == OpcodeGroup::Comparison;
}

inline constexpr bool isCallOpcode(OpCode op) noexcept {
    return opcodeGroup(op) == OpcodeGroup::Call;
}

inline constexpr bool mayInvokeMetamethod(OpCode op) noexcept {
    return opcodeMetadata(op).mayInvokeMetamethod;
}

}  // namespace Lua::VM
