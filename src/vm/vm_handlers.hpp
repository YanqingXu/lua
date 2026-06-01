#pragma once

/**
 * @file vm_handlers.hpp
 * @brief Opcode command handler table for VM dispatch.
 */

#include "common/types.hpp"
#include "compiler/opcode.hpp"
#include "vm/vm_dispatch.hpp"

#include <array>

namespace Lua {

struct RuntimeServices;
class Function;
class LuaState;
class Proto;
class Value;

namespace VM {

struct OpExecutionContext {
    RuntimeServices& services;
    LuaState* state;
    Function* function;
    Proto* proto;
    Value*& base;
    usize& pc;
    usize instructionPc;
    i32 nexeccalls;
};

enum class HandlerStatus : u8 {
    Continue,
    Reenter,
    Yielded,
    Returned,
};

using OpHandler = HandlerStatus (*)(OpExecutionContext& context, Instruction inst);

struct HandlerEntry {
    OpCode opcode;
    const char* name;
    OpcodeGroup group;
    OpHandler handler;
};

using HandlerTable = std::array<HandlerEntry, static_cast<usize>(NUM_OPCODES)>;

const HandlerTable& handlerTable() noexcept;
Opt<OpHandler> handlerFor(OpCode op) noexcept;
bool hasHandler(OpCode op) noexcept;
HandlerStatus runHandler(OpExecutionContext& context, Instruction inst);

}  // namespace VM
}  // namespace Lua
