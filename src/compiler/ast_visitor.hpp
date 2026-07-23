/**
 * @file ast_visitor.hpp
 * @brief Lua 抽象语法树变体使用的奇异递归模板模式访问器
 */

#pragma once

#include "compiler/ast.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace Lua {

namespace detail {

template <typename Visitor, typename Node, typename R> consteval bool canVisitNode() {
    if constexpr (std::is_void_v<R>) {
        return requires(Visitor& visitor, const Node& node) { visitor.visitNode(node); };
    } else {
        return requires(Visitor& visitor, const Node& node) {
            { visitor.visitNode(node) } -> std::convertible_to<R>;
        };
    }
}

} // namespace detail

template <typename Visitor, typename Node>
concept VisitsNode = detail::canVisitNode<Visitor, Node, void>();

template <typename Visitor, typename Node, typename R>
concept VisitsNodeAs = detail::canVisitNode<Visitor, Node, R>();

namespace detail {

template <typename Visitor, typename Variant, typename R, std::size_t... I>
consteval bool visitsVariantNodes(std::index_sequence<I...>) {
    return (canVisitNode<Visitor, std::variant_alternative_t<I, Variant>, R>() && ...);
}

} // namespace detail

template <typename Visitor, typename R = void>
concept VisitsExprNodes =
    detail::visitsVariantNodes<Visitor, ExprVariant, R>(std::make_index_sequence<std::variant_size_v<ExprVariant>>{});

template <typename Visitor, typename R = void>
concept VisitsStmtNodes =
    detail::visitsVariantNodes<Visitor, StmtVariant, R>(std::make_index_sequence<std::variant_size_v<StmtVariant>>{});

template <typename Visitor, typename R = void>
concept VisitsAstNodes = VisitsExprNodes<Visitor, R> && VisitsStmtNodes<Visitor, R>;

template <typename Derived, typename R = void>
/** @brief 基于奇异递归模板模式的表达式访问器。 */
struct ExprVisitor {
    R visit(const Expr& expr) {
        static_assert(detail::visitsVariantNodes<Derived, ExprVariant, R>(
                          std::make_index_sequence<std::variant_size_v<ExprVariant>>{}),
                      "ExprVisitor requires Derived::visitNode(const Expr node&) for every ExprVariant "
                      "alternative with a result compatible with R");

        auto dispatch = [this](const auto& node) -> R {
            using Node = std::remove_cvref_t<decltype(node)>;
            static_assert(detail::canVisitNode<Derived, Node, R>(),
                          "ExprVisitor requires Derived::visitNode(const Expr node&) with a compatible result");

            if constexpr (std::is_void_v<R>) {
                static_cast<Derived*>(this)->visitNode(node);
            } else {
                return static_cast<Derived*>(this)->visitNode(node);
            }
        };

        if constexpr (std::is_void_v<R>) {
            std::visit(dispatch, expr.variant);
        } else {
            return std::visit(dispatch, expr.variant);
        }
    }
};

template <typename Derived, typename R = void>
/** @brief 基于奇异递归模板模式的语句访问器。 */
struct StmtVisitor {
    R visit(const Stmt& stmt) {
        static_assert(detail::visitsVariantNodes<Derived, StmtVariant, R>(
                          std::make_index_sequence<std::variant_size_v<StmtVariant>>{}),
                      "StmtVisitor requires Derived::visitNode(const Stmt node&) for every StmtVariant "
                      "alternative with a result compatible with R");

        auto dispatch = [this](const auto& node) -> R {
            using Node = std::remove_cvref_t<decltype(node)>;
            static_assert(detail::canVisitNode<Derived, Node, R>(),
                          "StmtVisitor requires Derived::visitNode(const Stmt node&) with a compatible result");

            if constexpr (std::is_void_v<R>) {
                static_cast<Derived*>(this)->visitNode(node);
            } else {
                return static_cast<Derived*>(this)->visitNode(node);
            }
        };

        if constexpr (std::is_void_v<R>) {
            std::visit(dispatch, stmt.variant);
        } else {
            return std::visit(dispatch, stmt.variant);
        }
    }
};

template <typename Derived, typename R = void>
/** @brief 同时支持表达式和语句的抽象语法树访问器。 */
struct AstVisitor : ExprVisitor<Derived, R>, StmtVisitor<Derived, R> {
    using ExprVisitor<Derived, R>::visit;
    using StmtVisitor<Derived, R>::visit;
};

} // namespace Lua
