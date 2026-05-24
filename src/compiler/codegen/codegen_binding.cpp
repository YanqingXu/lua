/**
 * @file codegen_binding.cpp
 * @brief CodeGenerator symbol binding facade wrappers.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

// =====================================================================
// 符号绑定（PR-8 Symbol Binding）
// =====================================================================

SymbolRef CodeGenerator::resolve(const Str& name) {
    return binder_.resolve(name);
}

ValueResult CodeGenerator::symbolToValue(const SymbolRef& sym) {
    return binder_.symbolToValue(sym);
}

LValueRef CodeGenerator::symbolToLValue(const SymbolRef& sym) {
    return binder_.symbolToLValue(sym);
}


}  // namespace Lua
