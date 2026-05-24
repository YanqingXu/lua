#pragma once

/**
 * @file name_binder.hpp
 * @brief Name resolution boundary for CodeGenerator helpers.
 */

#include "compiler/codegen/codegen_state.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/scope_manager.hpp"

namespace Lua {

/**
 * @brief Resolves names and converts bindings into value/lvalue channels.
 *
 * NameBinder owns the Local -> Upvalue -> Global lookup rule. It shares
 * CodegenState and ScopeManager with the rest of codegen and does not own
 * either object.
 */
class NameBinder {
public:
    NameBinder(CodegenState& state, ScopeManager& scopes) noexcept;

    SymbolRef resolve(const Str& name);
    [[nodiscard]] ValueResult symbolToValue(const SymbolRef& sym) const;
    [[nodiscard]] LValueRef symbolToLValue(const SymbolRef& sym) const;

private:
    CodegenState& state_;
    ScopeManager& scopes_;
};

}  // namespace Lua
