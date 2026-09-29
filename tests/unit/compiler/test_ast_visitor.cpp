/**
 * @file test_ast_visitor.cpp
 * @brief AST Visitor infrastructure tests.
 */

#include "common/types.hpp"
#include "../framework/test_framework.hpp"
#include "compiler/ast.hpp"
#include "compiler/ast_visitor.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr Lua::CharPtr kSuiteName = "AST Visitor";

struct ExprNameVisitor : ExprVisitor<ExprNameVisitor, Lua::CharPtr> {
    Lua::CharPtr visitNode(const NilExpr&) {
        return "nil";
    }
    Lua::CharPtr visitNode(const BoolExpr&) {
        return "bool";
    }
    Lua::CharPtr visitNode(const NumberExpr&) {
        return "number";
    }
    Lua::CharPtr visitNode(const StringExpr&) {
        return "string";
    }
    Lua::CharPtr visitNode(const VarargExpr&) {
        return "vararg";
    }
    Lua::CharPtr visitNode(const NameExpr&) {
        return "name";
    }
    Lua::CharPtr visitNode(const BinaryExpr&) {
        return "binary";
    }
    Lua::CharPtr visitNode(const UnaryExpr&) {
        return "unary";
    }
    Lua::CharPtr visitNode(const TableExpr&) {
        return "table";
    }
    Lua::CharPtr visitNode(const CallExpr&) {
        return "call";
    }
    Lua::CharPtr visitNode(const IndexExpr&) {
        return "index";
    }
    Lua::CharPtr visitNode(const MemberExpr&) {
        return "member";
    }
    Lua::CharPtr visitNode(const FunctionExpr&) {
        return "function";
    }
    Lua::CharPtr visitNode(const ParenExpr&) {
        return "paren";
    }
};

struct StmtNameVisitor : StmtVisitor<StmtNameVisitor, Lua::CharPtr> {
    Lua::CharPtr visitNode(const EmptyStmt&) {
        return "empty";
    }
    Lua::CharPtr visitNode(const AssignStmt&) {
        return "assign";
    }
    Lua::CharPtr visitNode(const LocalStmt&) {
        return "local";
    }
    Lua::CharPtr visitNode(const CallStmt&) {
        return "call";
    }
    Lua::CharPtr visitNode(const IfStmt&) {
        return "if";
    }
    Lua::CharPtr visitNode(const WhileStmt&) {
        return "while";
    }
    Lua::CharPtr visitNode(const RepeatStmt&) {
        return "repeat";
    }
    Lua::CharPtr visitNode(const ForNumStmt&) {
        return "fornum";
    }
    Lua::CharPtr visitNode(const ForInStmt&) {
        return "forin";
    }
    Lua::CharPtr visitNode(const FunctionStmt&) {
        return "function";
    }
    Lua::CharPtr visitNode(const ReturnStmt&) {
        return "return";
    }
    Lua::CharPtr visitNode(const BreakStmt&) {
        return "break";
    }
    Lua::CharPtr visitNode(const DoStmt&) {
        return "do";
    }
};

struct AstNameVisitor : AstVisitor<AstNameVisitor, Lua::CharPtr> {
    Lua::CharPtr visitNode(const NumberExpr&) {
        return "number";
    }
    Lua::CharPtr visitNode(const EmptyStmt&) {
        return "empty";
    }

    template <typename Node> Lua::CharPtr visitNode(const Node&) {
        return "other";
    }
};

struct ConstExprNameVisitor : ExprVisitor<ConstExprNameVisitor, Lua::CharPtr> {
    template <typename Node> Lua::CharPtr visitNode(const Node&) const {
        return "node";
    }
};

struct VoidExprVisitor : ExprVisitor<VoidExprVisitor> {
    template <typename Node> void visitNode(const Node&) {}
};

struct PartialExprVisitor {
    Lua::CharPtr visitNode(const NumberExpr&) {
        return "number";
    }
};

static_assert(kExprNodeCount == 14);
static_assert(kStmtNodeCount == 13);
static_assert(std::variant_size_v<ExprVariant> == kExprNodeCount);
static_assert(std::variant_size_v<StmtVariant> == kStmtNodeCount);
static_assert(VisitsNode<ExprNameVisitor, NumberExpr>);
static_assert(!VisitsNode<PartialExprVisitor, NilExpr>);
static_assert(VisitsNodeAs<ExprNameVisitor, NumberExpr, Lua::CharPtr>);
static_assert(!VisitsNodeAs<ExprNameVisitor, NumberExpr, int>);
static_assert(VisitsExprNodes<ExprNameVisitor, Lua::CharPtr>);
static_assert(VisitsExprNodes<ConstExprNameVisitor, Lua::CharPtr>);
static_assert(VisitsExprNodes<VoidExprVisitor>);
static_assert(!VisitsExprNodes<VoidExprVisitor, Lua::CharPtr>);
static_assert(!VisitsExprNodes<PartialExprVisitor, Lua::CharPtr>);
static_assert(VisitsStmtNodes<StmtNameVisitor, Lua::CharPtr>);
static_assert(VisitsAstNodes<AstNameVisitor, Lua::CharPtr>);
static_assert(!VisitsAstNodes<PartialExprVisitor, Lua::CharPtr>);

void testExprVisitorDispatchesVariant(TestSuite& suite) {
    NumberExpr number{};
    number.value = 42.0;
    Expr expr(std::move(number));

    ExprNameVisitor visitor;
    ASSERT_EQ(suite, Lua::Str("number"), Lua::Str(visitor.visit(expr)), "ExprVisitor dispatches NumberExpr");
}

void testStmtVisitorDispatchesVariant(TestSuite& suite) {
    EmptyStmt empty{};
    Stmt stmt(std::move(empty));

    StmtNameVisitor visitor;
    ASSERT_EQ(suite, Lua::Str("empty"), Lua::Str(visitor.visit(stmt)), "StmtVisitor dispatches EmptyStmt");
}

void testAstVisitorDispatchesExprAndStmtVariants(TestSuite& suite) {
    NumberExpr number{};
    number.value = 7.0;
    Expr expr(std::move(number));

    EmptyStmt empty{};
    Stmt stmt(std::move(empty));

    AstNameVisitor visitor;
    ASSERT_EQ(suite, Lua::Str("number"), Lua::Str(visitor.visit(expr)), "AstVisitor dispatches Expr variants");
    ASSERT_EQ(suite, Lua::Str("empty"), Lua::Str(visitor.visit(stmt)), "AstVisitor dispatches Stmt variants");
}

} // namespace

void registerAstVisitorTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "expr visitor dispatch", testExprVisitorDispatchesVariant);
    registry.registerTest(kSuiteName, "stmt visitor dispatch", testStmtVisitorDispatchesVariant);
    registry.registerTest(kSuiteName, "combined visitor dispatch", testAstVisitorDispatchesExprAndStmtVariants);
}
