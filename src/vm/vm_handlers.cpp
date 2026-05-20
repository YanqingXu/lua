/**
 * @file vm_handlers.cpp
 * @brief Initial opcode command handlers used by SwitchDispatch.
 */

#include "vm/vm_handlers.hpp"
#include "common/lua_error.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm_internal.hpp"

#include <cstdio>

namespace Lua::VM {

namespace {

usize opcodeIndex(OpCode op) noexcept {
    return static_cast<usize>(op);
}

LuaState* requireState(const OpExecutionContext& context) {
    if (!context.state) {
        throw RuntimeError("VM handler requires LuaState");
    }
    return context.state;
}

Function* requireFunction(const OpExecutionContext& context) {
    if (!context.function) {
        throw RuntimeError("VM handler requires Function");
    }
    return context.function;
}

Proto* requireProto(const OpExecutionContext& context) {
    if (!context.proto) {
        throw RuntimeError("VM handler requires Proto");
    }
    return context.proto;
}

Value* refreshBase(LuaState* state) {
    return &state->getStack()[state->getCurrentCallInfo().base];
}

Value getRK(const OpExecutionContext& context, i32 rk) {
    if (ISK(rk)) {
        return context.proto->getConstant(INDEXK(rk));
    }
    return context.base[rk];
}

HandlerStatus handleMove(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    if (VM::detail::shouldDumpBytecode()) {
        std::fprintf(stderr, "[MOVE] pc=%zu a=%d b=%d base[b]=", context.instructionPc, a, b);
        if (context.base[b].isNumber()) std::fprintf(stderr, "%g", context.base[b].asNumber());
        else if (context.base[b].isNil()) std::fprintf(stderr, "nil");
        else if (context.base[b].isFunction()) std::fprintf(stderr, "function");
        else if (context.base[b].isString()) std::fprintf(stderr, "'%s'", context.base[b].asString()->c_str());
        else std::fprintf(stderr, "other");
        std::fprintf(stderr, "\n");
    }

    context.base[a] = context.base[b];
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadK(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);

    context.base[a] = context.proto->getConstant(bx);
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadBool(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    context.base[a] = Value(b != 0);
    if (c != 0) {
        context.pc++;
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadNil(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    for (i32 i = a; i <= b; i++) {
        context.base[i] = Value();
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleGetGlobal(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);
    const Value& key = context.proto->getConstant(bx);

    Table* env = function->getEnv();
    if (!env) {
        env = state->getGlobalTable();
    }

    Value result;
    VM::detail::gettable(state, Value(env), key, result);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleSetGlobal(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);
    const Value& key = context.proto->getConstant(bx);

    Table* env = function->getEnv();
    if (!env) {
        env = state->getGlobalTable();
    }

    Value val = context.base[a];
    VM::detail::settable(state, Value(env), key, val);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleGetUpval(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Upvalue* uv = function->getUpvalue(b);
    if (!uv) {
        throw RuntimeError("VM: GETUPVAL invalid upvalue index");
    }
    context.base[a] = uv->getValue(state->getStack());
    return HandlerStatus::Continue;
}

HandlerStatus handleSetUpval(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Function* function = requireFunction(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Upvalue* uv = function->getUpvalue(b);
    if (!uv) {
        throw RuntimeError("VM: SETUPVAL invalid upvalue index");
    }
    uv->setValue(state->getStack(), context.base[a]);
    return HandlerStatus::Continue;
}

HandlerStatus handleGetTable(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    Value table = context.base[b];
    Value key = getRK(context, c);
    Value result;
    VM::detail::gettable(state, table, key, result);
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
    VM::detail::settable(state, table, key, val);
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
    VM::detail::gettable(state, obj, key, result);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleSetList(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    VM::detail::setList(state, context.base, a, b, c);
    return HandlerStatus::Continue;
}

HandlerStatus handleArithmetic(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    VM::detail::execArithmetic(state, context.proto, context.base, a, b, c, GET_OPCODE(inst));
    return HandlerStatus::Continue;
}

HandlerStatus handleUnaryMinus(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Value val = context.base[b];
    Value result;
    VM::detail::unaryMinus(state, result, val);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleNot(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    context.base[a] = Value(!context.base[b].isTrue());
    return HandlerStatus::Continue;
}

HandlerStatus handleLength(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Value val = context.base[b];
    Value result;
    VM::detail::length(state, result, val);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleConcat(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    VM::detail::concat(context.services, state, context.base, a, b, c);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleJump(OpExecutionContext& context, Instruction inst) {
    context.pc += GETARG_sBx(inst);
    return HandlerStatus::Continue;
}

HandlerStatus handleComparison(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    Value left = getRK(context, b);
    Value right = getRK(context, c);
    bool result = false;

    switch (GET_OPCODE(inst)) {
        case OpCode::EQ:
            result = VM::detail::equal(state, left, right);
            break;
        case OpCode::LT:
            result = VM::detail::lessThan(state, left, right);
            break;
        case OpCode::LE:
            result = VM::detail::lessEqual(state, left, right);
            break;
        default:
            throw RuntimeError("VM: invalid comparison handler opcode");
    }

    context.base = refreshBase(state);
    if (result != (a != 0)) {
        context.pc++;
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleTest(OpExecutionContext& context, Instruction inst) {
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 c = GETARG_C(inst);

    bool val = context.base[a].isTrue();
    if ((!val) != (c != 0)) {
        const Vec<Instruction>& code = proto->getCode();
        if (context.pc < code.size()) {
            context.pc += GETARG_sBx(code[context.pc]);
        }
    }
    context.pc++;
    return HandlerStatus::Continue;
}

HandlerStatus handleTestSet(OpExecutionContext& context, Instruction inst) {
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    bool val = context.base[b].isTrue();
    if ((!val) != (c != 0)) {
        context.base[a] = context.base[b];
        const Vec<Instruction>& code = proto->getCode();
        if (context.pc < code.size()) {
            context.pc += GETARG_sBx(code[context.pc]);
        }
    }
    context.pc++;
    return HandlerStatus::Continue;
}

HandlerTable makeHandlerTable() {
    HandlerTable table{};

    for (usize index = 0; index < table.size(); ++index) {
        OpCode op = static_cast<OpCode>(index);
        table[index] = HandlerEntry{
            op,
            getOpName(op),
            opcodeGroup(op),
            nullptr
        };
    }

    table[opcodeIndex(OpCode::MOVE)].handler = handleMove;
    table[opcodeIndex(OpCode::LOADK)].handler = handleLoadK;
    table[opcodeIndex(OpCode::LOADBOOL)].handler = handleLoadBool;
    table[opcodeIndex(OpCode::LOADNIL)].handler = handleLoadNil;
    table[opcodeIndex(OpCode::GETGLOBAL)].handler = handleGetGlobal;
    table[opcodeIndex(OpCode::SETGLOBAL)].handler = handleSetGlobal;
    table[opcodeIndex(OpCode::GETUPVAL)].handler = handleGetUpval;
    table[opcodeIndex(OpCode::SETUPVAL)].handler = handleSetUpval;
    table[opcodeIndex(OpCode::GETTABLE)].handler = handleGetTable;
    table[opcodeIndex(OpCode::SETTABLE)].handler = handleSetTable;
    table[opcodeIndex(OpCode::NEWTABLE)].handler = handleNewTable;
    table[opcodeIndex(OpCode::SELF)].handler = handleSelf;
    table[opcodeIndex(OpCode::SETLIST)].handler = handleSetList;
    table[opcodeIndex(OpCode::ADD)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::SUB)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::MUL)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::DIV)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::MOD)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::POW)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::UNM)].handler = handleUnaryMinus;
    table[opcodeIndex(OpCode::NOT)].handler = handleNot;
    table[opcodeIndex(OpCode::LEN)].handler = handleLength;
    table[opcodeIndex(OpCode::CONCAT)].handler = handleConcat;
    table[opcodeIndex(OpCode::JMP)].handler = handleJump;
    table[opcodeIndex(OpCode::EQ)].handler = handleComparison;
    table[opcodeIndex(OpCode::LT)].handler = handleComparison;
    table[opcodeIndex(OpCode::LE)].handler = handleComparison;
    table[opcodeIndex(OpCode::TEST)].handler = handleTest;
    table[opcodeIndex(OpCode::TESTSET)].handler = handleTestSet;

    return table;
}

}  // namespace

const HandlerTable& handlerTable() noexcept {
    static const HandlerTable table = makeHandlerTable();
    return table;
}

OpHandler handlerFor(OpCode op) noexcept {
    usize index = opcodeIndex(op);
    const HandlerTable& table = handlerTable();
    if (index >= table.size()) {
        return nullptr;
    }
    return table[index].handler;
}

bool hasHandler(OpCode op) noexcept {
    return handlerFor(op) != nullptr;
}

HandlerStatus runHandler(OpExecutionContext& context, Instruction inst) {
    OpCode op = GET_OPCODE(inst);
    OpHandler handler = handlerFor(op);
    if (!handler) {
        throw RuntimeError("VM: no handler registered for opcode: " + Str(getOpName(op)));
    }
    return handler(context, inst);
}

}  // namespace Lua::VM
