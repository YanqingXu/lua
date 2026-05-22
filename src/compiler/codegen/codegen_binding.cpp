/**
 * @file codegen_binding.cpp
 * @brief CodeGenerator symbol binding and upvalue resolution.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

i32 CodeGenerator::findUpvalue(const Str& name) {
    return scopes_.findUpvalue(name);
}

i32 CodeGenerator::addUpvalue(const Str& name, bool inStack, i32 index) {
    return scopes_.addUpvalue(name, inStack, index);
}

i32 CodeGenerator::resolveUpvalue(const Str& name) {
    return scopes_.resolveUpvalue(name);
}

// =====================================================================
// 符号绑定（PR-8 Symbol Binding）
// =====================================================================

SymbolRef CodeGenerator::resolve(const Str& name) {
    SymbolRef result;
    result.name = name;

    i32 reg = findLocalVar(name);
    if (reg >= 0) {
        result.kind = SymbolRef::Kind::Local;
        result.index = reg;
        return result;
    }

    i32 up = resolveUpvalue(name);
    if (up >= 0) {
        result.kind = SymbolRef::Kind::Upvalue;
        result.index = up;
        return result;
    }

    result.kind = SymbolRef::Kind::Global;
    result.index = stringConstant(name);
    return result;
}

ValueResult CodeGenerator::symbolToValue(const SymbolRef& sym) {
    ValueResult result;

    switch (sym.kind) {
        case SymbolRef::Kind::Local:
            result.kind = ValueResult::Kind::Register;
            result.access = ValueResult::AccessKind::Local;
            result.reg = sym.index;
            result.ownsRegister = false;
            break;

        case SymbolRef::Kind::Upvalue:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Upvalue;
            result.aux = sym.index;
            break;

        case SymbolRef::Kind::Global:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Global;
            result.constIndex = sym.index;
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
