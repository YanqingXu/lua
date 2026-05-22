/**
 * @file codegen_binding.cpp
 * @brief CodeGenerator symbol binding and upvalue resolution.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

i32 CodeGenerator::findUpvalue(const Str& name) {
    return state_.upvalues.find(name);
}

i32 CodeGenerator::addUpvalue(const Str& name, bool inStack, i32 index) {
    return state_.upvalues.add(name, inStack, index);
}

i32 CodeGenerator::resolveUpvalue(const Str& name) {
    if (state_.parent == nullptr) {
        return -1;
    }

    // 优先在直接父函数的局部变量中查找
    i32 local = state_.parent->findLocalVar(name);
    if (local >= 0) {
        return addUpvalue(name, true, local);
    }

    // 否则递归在更外层查找，并在父函数中建立中转upvalue
    i32 parentUp = state_.parent->resolveUpvalue(name);
    if (parentUp >= 0) {
        return addUpvalue(name, false, parentUp);
    }

    return -1;
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