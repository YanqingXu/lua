/**
 * @file codegen_binding.cpp
 * @brief 代码生成器的符号绑定外观封装
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

// =====================================================================
/** @brief 符号绑定（第 8 次拉取请求）。 */
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
