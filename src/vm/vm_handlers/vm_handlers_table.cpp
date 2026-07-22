/**
 * @file vm_handlers_table.cpp
 * @brief 表操作码处理器
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "common/lua_error.hpp"
#include "core/table.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm_handlers/vm_diagnostics.hpp"

#include <string>

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
    try {
        detail::gettable(state, table, key, result);
    } catch (const RuntimeError& error) {
        if (std::string(error.what()).find("attempt to index a non-table value") == std::string::npos) {
            throw;
        }
        Str sourceName = diagnostics::describeRegister(context.proto, b, context.instructionPc).value_or(Str());
        throw RuntimeError(diagnostics::formatTypeActionError("index", table, sourceName));
    }
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
    [[maybe_unused]] const usize preCreateCollected =
        state->getGlobalState().getGC().maybeCollectAutomatic(state);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleNewTable(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);

    Table* table = state->getGlobalState().getGC().create<Table>();
    context.base[a] = Value(table);
    [[maybe_unused]] const usize postCreateCollected =
        state->getGlobalState().getGC().maybeCollectAutomatic(state);
    context.base = refreshBase(state);
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
    try {
        detail::gettable(state, obj, key, result);
    } catch (const RuntimeError& error) {
        if (std::string(error.what()).find("attempt to index a non-table value") == std::string::npos) {
            throw;
        }
        Str sourceName = diagnostics::describeRegister(context.proto, b, context.instructionPc).value_or(Str());
        throw RuntimeError(diagnostics::formatTypeActionError("index", obj, sourceName));
    }
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleSetList(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);
    if (c == 0) {
        Proto* proto = requireProto(context);
        const auto code = proto->getInstructionSpan();
        if (context.pc >= code.size()) {
            throw RuntimeError("VM: SETLIST missing extended block operand");
        }

        c = static_cast<i32>(code[context.pc++]);
        state->getCurrentCallInfo().savedpc = code.data() + context.pc;
    }

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
