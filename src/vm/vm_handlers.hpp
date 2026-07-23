#pragma once

/**
 * @file vm_handlers.hpp
 * @brief VM 调度使用的操作码命令处理器表
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

/** @brief 单条操作码处理器使用的执行上下文。 */
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

/** @brief 操作码处理器执行后的控制流状态。 */
enum class HandlerStatus : u8 {
    Continue,
    Reenter,
    Yielded,
    Returned,
};

using OpHandler = HandlerStatus (*)(OpExecutionContext& context, Instruction inst);

/** @brief 操作码与对应处理器函数的映射条目。 */
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

} // namespace VM
} // namespace Lua
