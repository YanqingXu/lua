/**
 * @file codegen_binding.cpp
 * @brief CodeGenerator symbol binding and upvalue resolution.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

// =====================================================================
// 符号绑定（PR-8 Symbol Binding）
// =====================================================================

SymbolRef CodeGenerator::resolve(const Str& name) {
    SymbolRef result;
    result.name = name;

    i32 reg = scopes_.findLocalVar(name);
    if (reg >= 0) {
        result.kind = SymbolRef::Kind::Local;
        result.index = reg;
        return result;
    }

    i32 up = scopes_.resolveUpvalue(name);
    if (up >= 0) {
        result.kind = SymbolRef::Kind::Upvalue;
        result.index = up;
        return result;
    }

    result.kind = SymbolRef::Kind::Global;
    result.index = state_.bytecode.addStringConstant(name);
    return result;
}

ValueResult CodeGenerator::symbolToValue(const SymbolRef& sym) {
    ValueResult result;

    switch (sym.kind) {
        case SymbolRef::Kind::Local:
            result = ValueResult::makeRegister(sym.index, false, ValueResult::AccessKind::Local);
            break;

        case SymbolRef::Kind::Upvalue:
            result = ValueResult::makePendingLoad(ValueResult::AccessKind::Upvalue, -1, -1, sym.index);
            break;

        case SymbolRef::Kind::Global:
            result = ValueResult::makePendingLoad(ValueResult::AccessKind::Global, -1, sym.index, -1);
            break;

        default:
            break;
    }

    return result;
}

LValueRef CodeGenerator::symbolToLValue(const SymbolRef& sym) {
    LValueRef result;

    switch (sym.kind) {
        case SymbolRef::Kind::Local:
            result.kind = LValueRef::Kind::Local;
            result.slot = sym.index;
            break;

        case SymbolRef::Kind::Upvalue:
            result.kind = LValueRef::Kind::Upvalue;
            result.slot = sym.index;
            break;

        case SymbolRef::Kind::Global:
            result.kind = LValueRef::Kind::Global;
            result.slot = sym.index;
            break;

        default:
            break;
    }

    return result;
}


}  // namespace Lua
