/**
 * @file test_ast_visitor.cpp
 * @brief AST Visitor infrastructure tests.
 */

#include "../framework/test_framework.hpp"
#include "compiler/ast.hpp"
#include "compiler/ast_visitor.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "AST Visitor";

struct ExprNameVisitor : ExprVisitor<ExprNameVisitor, const char*> {
    const char* visitNode(const NilExpr&) { return "nil"; }
    const char* visitNode(const BoolExpr&) { return "bool"; }
    const char* visitNode(const NumberExpr&) { return "number"; }
    const char* visitNode(const StringExpr&) { return "string"; }
    const char* visitNode(const VarargExpr&) { return "vararg"; }
    const char* visitNode(const NameExpr&) { return "name"; }
    const char* visitNode(const BinaryExpr&) { return "binary"; }
    const char* visitNode(const UnaryExpr&) { return "unary"; }
    const char* visitNode(const TableExpr&) { return "table"; }
    const char* visitNode(const CallExpr&) { return "call"; }
    const char* visitNode(const IndexExpr&) { return "index"; }
    const char* visitNode(const MemberExpr&) { return "member"; }
    const char* visitNode(const FunctionExpr&) { return "function"; }
    const char* visitNode(const ParenExpr&) { return "paren"; }
};

struct StmtNameVisitor : StmtVisitor<StmtNameVisitor, const char*> {
    const char* visitNode(const EmptyStmt&) { return "empty"; }
    const char* visitNode(const AssignStmt&) { return "assign"; }
    const char* visitNode(const LocalStmt&) { return "local"; }
    const char* visitNode(const CallStmt&) { return "call"; }
    const char* visitNode(const IfStmt&) { return "if"; }
    const char* visitNode(const WhileStmt&) { return "while"; }
    const char* visitNode(const RepeatStmt&) { return "repeat"; }
    const char* visitNode(const ForNumStmt&) { return "fornum"; }
    const char* visitNode(const ForInStmt&) { return "forin"; }
    const char* visitNode(const FunctionStmt&) { return "function"; }
    const char* visitNode(const ReturnStmt&) { return "return"; }
    const char* visitNode(const BreakStmt&) { return "break"; }
    const char* visitNode(const DoStmt&) { return "do"; }
};

void testExprVisitorDispatchesVariant(TestSuite& suite) {
    NumberExpr number{};
    number.value = 42.0;
    Expr expr(std::move(number));

    ExprNameVisitor visitor;
    ASSERT_EQ(suite, std::string("number"), std::string(visitor.visit(expr)),
              "ExprVisitor dispatches NumberExpr");
}

void testStmtVisitorDispatchesVariant(TestSuite& suite) {
    EmptyStmt empty{};
    Stmt stmt(std::move(empty));

    StmtNameVisitor visitor;
    ASSERT_EQ(suite, std::string("empty"), std::string(visitor.visit(stmt)),
              "StmtVisitor dispatches EmptyStmt");
}

} // namespace

void registerAstVisitorTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "expr visitor dispatch", testExprVisitorDispatchesVariant);
    registry.registerTest(kSuiteName, "stmt visitor dispatch", testStmtVisitorDispatchesVariant);
}

