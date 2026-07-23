/**
 * @file vm_handlers.cpp
 * @brief VM 调度策略使用的操作码命令处理器注册表
 */

#include "vm/vm_handlers.hpp"
#include "common/lua_error.hpp"
#include "vm/vm_handlers/vm_handler_utils.hpp"

namespace Lua::VM {

namespace {

HandlerTable makeHandlerTable() {
    HandlerTable table{};

    for (usize index = 0; index < table.size(); ++index) {
        OpCode op = static_cast<OpCode>(index);
        table[index] = HandlerEntry{op, getOpName(op), opcodeGroup(op), nullptr};
    }

    handlers::registerDataHandlers(table);
    handlers::registerGlobalUpvalueHandlers(table);
    handlers::registerTableHandlers(table);
    handlers::registerArithmeticHandlers(table);
    handlers::registerUnaryHandlers(table);
    handlers::registerBranchHandlers(table);
    handlers::registerLoopHandlers(table);
    handlers::registerClosureHandlers(table);
    handlers::registerCallHandlers(table);

    return table;
}

} // namespace

const HandlerTable& handlerTable() noexcept {
    static const HandlerTable table = makeHandlerTable();
    return table;
}

Opt<OpHandler> handlerFor(OpCode op) noexcept {
    usize index = handlers::opcodeIndex(op);
    const HandlerTable& table = handlerTable();
    if (index >= table.size()) {
        return std::nullopt;
    }
    if (table[index].handler == nullptr) {
        return std::nullopt;
    }
    return table[index].handler;
}

bool hasHandler(OpCode op) noexcept {
    return handlerFor(op).has_value();
}

HandlerStatus runHandler(OpExecutionContext& context, Instruction inst) {
    OpCode op = GET_OPCODE(inst);
    Opt<OpHandler> handler = handlerFor(op);
    if (!handler.has_value()) {
        throw RuntimeError("VM: no handler registered for opcode: " + Str(getOpName(op)));
    }
    return handler.value()(context, inst);
}

} // namespace Lua::VM
