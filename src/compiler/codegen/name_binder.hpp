#pragma once

/**
 * @file name_binder.hpp
 * @brief 代码生成器辅助逻辑的名称解析边界
 */

#include "compiler/codegen/codegen_state.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/scope_manager.hpp"

namespace Lua {

/**
 * @brief 解析名称并将绑定转换为值或左值通道
 *
 * NameBinder 负责“局部变量 → 上值 → 全局变量”的查找规则。它与代码生成器其余部分共享
 * CodegenState 和 ScopeManager，但不拥有其中任何对象。
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

} // namespace Lua
