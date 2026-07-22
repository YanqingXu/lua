/**
 * @file vm_handlers_arith.cpp
 * @brief 算术操作码处理器
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "common/lua_error.hpp"
#include "vm/vm_handlers/vm_diagnostics.hpp"

#include <cctype>
#include <cstdlib>
#include <string>

namespace Lua::VM::handlers {

namespace {

bool canConvertToNumber(const Value& value) {
    if (value.isNumber()) {
        return true;
    }
    if (!value.isString()) {
        return false;
    }

    const char* text = value.asString()->c_str();
    char* end = nullptr;
    std::strtod(text, &end);
    if (end == text) {
        return false;
    }
    while (std::isspace(static_cast<unsigned char>(*end)) != 0) {
        ++end;
    }
    return *end == '\0';
}

Str describeArithmeticOperand(OpExecutionContext& context, i32 rk, const Value& value) {
    Str sourceName;
    if (!ISK(rk)) {
        sourceName = diagnostics::describeRegister(context.proto, rk, context.instructionPc).value_or(Str());
    }
    return diagnostics::formatTypeActionError("perform arithmetic on", value, sourceName);
}

HandlerStatus handleArithmetic(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    try {
        detail::execArithmetic(state, context.proto, context.base, a, b, c, GET_OPCODE(inst));
    } catch (const RuntimeError& error) {
        if (std::string(error.what()).find("attempt to perform arithmetic on non-number values") ==
            std::string::npos) {
            throw;
        }

        Value left = getRK(context, b);
        Value right = getRK(context, c);
        if (!canConvertToNumber(left)) {
            throw RuntimeError(describeArithmeticOperand(context, b, left));
        }
        throw RuntimeError(describeArithmeticOperand(context, c, right));
    }
    return HandlerStatus::Continue;
}

}  // namespace

void registerArithmeticHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::ADD)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::SUB)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::MUL)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::DIV)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::MOD)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::POW)].handler = handleArithmetic;
}

}  // namespace Lua::VM::handlers
