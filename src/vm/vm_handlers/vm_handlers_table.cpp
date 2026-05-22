/**
 * @file vm_handlers_table.cpp
 * @brief Table opcode handlers.
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "core/table.hpp"
#include "vm/state/global_state.hpp"

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleGetTable(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    Value table = context.base[b];
    Value key = getRK(context, c);
    Value result;
    detail::gettable(state, table, key, result);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleSetTable(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    Value table = context.base[a];
    Value key = getRK(context, b);
    Value val = getRK(context, c);
    detail::settable(state, table, key, val);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleNewTable(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);

    Table* table = new Table();
    state->getGlobalState().getGC().registerObject(table);
    context.base[a] = Value(table);
    return HandlerStatus::Continue;
}

HandlerStatus handleSelf(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    Value obj = context.base[b];
    context.base[a + 1] = obj;
    Value key = getRK(context, c);
    Value result;
    detail::gettable(state, obj, key, result);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleSetList(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    detail::setList(state, context.base, a, b, c);
    return HandlerStatus::Continue;
}

}  // namespace

void registerTableHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::GETTABLE)].handler = handleGetTable;
    table[opcodeIndex(OpCode::SETTABLE)].handler = handleSetTable;
    table[opcodeIndex(OpCode::NEWTABLE)].handler = handleNewTable;
    table[opcodeIndex(OpCode::SELF)].handler = handleSelf;
    table[opcodeIndex(OpCode::SETLIST)].handler = handleSetList;
}

}  // namespace Lua::VM::handlers
