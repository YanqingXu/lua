#pragma once

/**
 * @file vm_handler_utils.hpp
 * @brief VM 操作码处理器分片使用的内部辅助函数
 */

#include "common/lua_error.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_handlers.hpp"
#include "vm/vm_internal.hpp"

namespace Lua::VM::handlers {

inline usize opcodeIndex(OpCode op) noexcept {
    return static_cast<usize>(op);
}

inline LuaState* requireState(const OpExecutionContext& context) {
    if (!context.state) {
        throw RuntimeError("VM handler requires LuaState");
    }
    return context.state;
}

inline Function* requireFunction(const OpExecutionContext& context) {
    if (!context.function) {
        throw RuntimeError("VM handler requires Function");
    }
    return context.function;
}

inline Proto* requireProto(const OpExecutionContext& context) {
    if (!context.proto) {
        throw RuntimeError("VM handler requires Proto");
    }
    return context.proto;
}

inline Value* refreshBase(LuaState* state) {
    return &state->getStack()[state->getCurrentCallInfo().base];
}

inline Value getRK(const OpExecutionContext& context, i32 rk) {
    if (ISK(rk)) {
        return context.proto->getConstant(INDEXK(rk));
    }
    return context.base[rk];
}

void registerDataHandlers(HandlerTable& table) noexcept;
void registerGlobalUpvalueHandlers(HandlerTable& table) noexcept;
void registerTableHandlers(HandlerTable& table) noexcept;
void registerArithmeticHandlers(HandlerTable& table) noexcept;
void registerUnaryHandlers(HandlerTable& table) noexcept;
void registerBranchHandlers(HandlerTable& table) noexcept;
void registerLoopHandlers(HandlerTable& table) noexcept;
void registerClosureHandlers(HandlerTable& table) noexcept;
void registerCallHandlers(HandlerTable& table) noexcept;

}  // namespace Lua::VM::handlers
