/**
 * @file ast_visitor.hpp
 * @brief CRTP visitors for Lua AST variants.
 */

#pragma once

#include "compiler/ast.hpp"

#include <variant>

namespace Lua {

template <typename Derived, typename R = void>
struct ExprVisitor {
    R visit(const Expr& expr) {
        return std::visit([this](const auto& node) -> R {
            return static_cast<Derived*>(this)->visitNode(node);
        }, expr.variant);
    }
};

template <typename Derived, typename R = void>
struct StmtVisitor {
    R visit(const Stmt& stmt) {
        return std::visit([this](const auto& node) -> R {
            return static_cast<Derived*>(this)->visitNode(node);
        }, stmt.variant);
    }
};

} // namespace Lua

