/**
 * @file ast_visitor.hpp
 * @brief CRTP visitors for Lua AST variants.
 */

#pragma once

#include "compiler/ast.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace Lua {

template <typename Visitor, typename Node>
concept VisitsNode = requires(Visitor& visitor, const Node& node) {
    visitor.visitNode(node);
};

template <typename Visitor, typename Node, typename R>
concept VisitsNodeAs =
    (std::is_void_v<R> && VisitsNode<Visitor, Node>) ||
    (!std::is_void_v<R> && requires(Visitor& visitor, const Node& node) {
        { visitor.visitNode(node) } -> std::convertible_to<R>;
    });

namespace detail {

template <typename Visitor, typename Variant, typename R, std::size_t... I>
consteval bool visitsVariantNodes(std::index_sequence<I...>) {
    return (VisitsNodeAs<Visitor, std::variant_alternative_t<I, Variant>, R> && ...);
}

} // namespace detail

template <typename Visitor, typename R = void>
concept VisitsExprNodes = detail::visitsVariantNodes<Visitor, ExprVariant, R>(
    std::make_index_sequence<std::variant_size_v<ExprVariant>>{});

template <typename Visitor, typename R = void>
concept VisitsStmtNodes = detail::visitsVariantNodes<Visitor, StmtVariant, R>(
    std::make_index_sequence<std::variant_size_v<StmtVariant>>{});

template <typename Visitor, typename R = void>
concept VisitsAstNodes = VisitsExprNodes<Visitor, R> && VisitsStmtNodes<Visitor, R>;

template <typename Derived, typename R = void>
struct ExprVisitor {
    R visit(const Expr& expr) {
        static_assert(canVisitAll(std::make_index_sequence<std::variant_size_v<ExprVariant>>{}),
                      "ExprVisitor requires Derived::visitNode(const Expr node&) for every ExprVariant "
                      "alternative with a result compatible with R");

        auto dispatch = [this](const auto& node) -> R {
            using Node = std::remove_cvref_t<decltype(node)>;
            static_assert(canVisitNode<Node>(),
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

private:
    template <typename Node>
    static consteval bool canVisitNode() {
        if constexpr (std::is_void_v<R>) {
            return requires(Derived& visitor, const Node& node) {
                visitor.visitNode(node);
            };
        } else {
            return requires(Derived& visitor, const Node& node) {
                { visitor.visitNode(node) } -> std::convertible_to<R>;
            };
        }
    }

    template <std::size_t... I>
    static consteval bool canVisitAll(std::index_sequence<I...>) {
        return (canVisitNode<std::variant_alternative_t<I, ExprVariant>>() && ...);
    }
};

template <typename Derived, typename R = void>
struct StmtVisitor {
    R visit(const Stmt& stmt) {
        static_assert(canVisitAll(std::make_index_sequence<std::variant_size_v<StmtVariant>>{}),
                      "StmtVisitor requires Derived::visitNode(const Stmt node&) for every StmtVariant "
                      "alternative with a result compatible with R");

        auto dispatch = [this](const auto& node) -> R {
            using Node = std::remove_cvref_t<decltype(node)>;
            static_assert(canVisitNode<Node>(),
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

private:
    template <typename Node>
    static consteval bool canVisitNode() {
        if constexpr (std::is_void_v<R>) {
            return requires(Derived& visitor, const Node& node) {
                visitor.visitNode(node);
            };
        } else {
            return requires(Derived& visitor, const Node& node) {
                { visitor.visitNode(node) } -> std::convertible_to<R>;
            };
        }
    }

    template <std::size_t... I>
    static consteval bool canVisitAll(std::index_sequence<I...>) {
        return (canVisitNode<std::variant_alternative_t<I, StmtVariant>>() && ...);
    }
};

template <typename Derived, typename R = void>
struct AstVisitor : ExprVisitor<Derived, R>, StmtVisitor<Derived, R> {
    using ExprVisitor<Derived, R>::visit;
    using StmtVisitor<Derived, R>::visit;
};

} // namespace Lua
