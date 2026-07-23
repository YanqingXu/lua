/**
 * @file name_binder.cpp
 * @brief 名称绑定器实现
 */

#include "compiler/codegen/name_binder.hpp"

namespace Lua {

NameBinder::NameBinder(CodegenState& state, ScopeManager& scopes) noexcept : state_(state), scopes_(scopes) {}

SymbolRef NameBinder::resolve(const Str& name) {
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

ValueResult NameBinder::symbolToValue(const SymbolRef& sym) const {
    switch (sym.kind) {
    case SymbolRef::Kind::Local:
        return ValueResult::makeRegister(sym.index, false, ValueResult::AccessKind::Local);

    case SymbolRef::Kind::Upvalue:
        return ValueResult::makePendingLoad(ValueResult::AccessKind::Upvalue, -1, -1, sym.index);

    case SymbolRef::Kind::Global:
        return ValueResult::makePendingLoad(ValueResult::AccessKind::Global, -1, sym.index, -1);

    default:
        return {};
    }
}

LValueRef NameBinder::symbolToLValue(const SymbolRef& sym) const {
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

} // namespace Lua
