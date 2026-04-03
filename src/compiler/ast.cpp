/**
 * @file ast.cpp
 * @brief AST节点实现
 */

#include "ast.hpp"

namespace Lua {

// =====================================================================
// 辅助函数：从variant中获取行号和列号
// =====================================================================

namespace {

// 获取表达式的行号
struct GetExprLine {
    i32 operator()(const NilExpr& e) const { return e.line; }
    i32 operator()(const BoolExpr& e) const { return e.line; }
    i32 operator()(const NumberExpr& e) const { return e.line; }
    i32 operator()(const StringExpr& e) const { return e.line; }
    i32 operator()(const VarargExpr& e) const { return e.line; }
    i32 operator()(const NameExpr& e) const { return e.line; }
    i32 operator()(const BinaryExpr& e) const { return e.line; }
    i32 operator()(const UnaryExpr& e) const { return e.line; }
    i32 operator()(const TableExpr& e) const { return e.line; }
    i32 operator()(const CallExpr& e) const { return e.line; }
    i32 operator()(const IndexExpr& e) const { return e.line; }
    i32 operator()(const MemberExpr& e) const { return e.line; }
    i32 operator()(const FunctionExpr& e) const { return e.line; }
    i32 operator()(const ParenExpr& e) const { return e.line; }
};

// 获取表达式的列号
struct GetExprColumn {
    i32 operator()(const NilExpr& e) const { return e.column; }
    i32 operator()(const BoolExpr& e) const { return e.column; }
    i32 operator()(const NumberExpr& e) const { return e.column; }
    i32 operator()(const StringExpr& e) const { return e.column; }
    i32 operator()(const VarargExpr& e) const { return e.column; }
    i32 operator()(const NameExpr& e) const { return e.column; }
    i32 operator()(const BinaryExpr& e) const { return e.column; }
    i32 operator()(const UnaryExpr& e) const { return e.column; }
    i32 operator()(const TableExpr& e) const { return e.column; }
    i32 operator()(const CallExpr& e) const { return e.column; }
    i32 operator()(const IndexExpr& e) const { return e.column; }
    i32 operator()(const MemberExpr& e) const { return e.column; }
    i32 operator()(const FunctionExpr& e) const { return e.column; }
    i32 operator()(const ParenExpr& e) const { return e.column; }
};

// 获取语句的行号
struct GetStmtLine {
    i32 operator()(const EmptyStmt& s) const { return s.line; }
    i32 operator()(const AssignStmt& s) const { return s.line; }
    i32 operator()(const LocalStmt& s) const { return s.line; }
    i32 operator()(const CallStmt& s) const { return s.line; }
    i32 operator()(const IfStmt& s) const { return s.line; }
    i32 operator()(const WhileStmt& s) const { return s.line; }
    i32 operator()(const RepeatStmt& s) const { return s.line; }
    i32 operator()(const ForNumStmt& s) const { return s.line; }
    i32 operator()(const ForInStmt& s) const { return s.line; }
    i32 operator()(const FunctionStmt& s) const { return s.line; }
    i32 operator()(const ReturnStmt& s) const { return s.line; }
    i32 operator()(const BreakStmt& s) const { return s.line; }
    i32 operator()(const DoStmt& s) const { return s.line; }
};

// 获取语句的列号
struct GetStmtColumn {
    i32 operator()(const EmptyStmt& s) const { return s.column; }
    i32 operator()(const AssignStmt& s) const { return s.column; }
    i32 operator()(const LocalStmt& s) const { return s.column; }
    i32 operator()(const CallStmt& s) const { return s.column; }
    i32 operator()(const IfStmt& s) const { return s.column; }
    i32 operator()(const WhileStmt& s) const { return s.column; }
    i32 operator()(const RepeatStmt& s) const { return s.column; }
    i32 operator()(const ForNumStmt& s) const { return s.column; }
    i32 operator()(const ForInStmt& s) const { return s.column; }
    i32 operator()(const FunctionStmt& s) const { return s.column; }
    i32 operator()(const ReturnStmt& s) const { return s.column; }
    i32 operator()(const BreakStmt& s) const { return s.column; }
    i32 operator()(const DoStmt& s) const { return s.column; }
};

} // anonymous namespace

// =====================================================================
// Expr实现
// =====================================================================

i32 Expr::getLine() const {
    return std::visit(GetExprLine{}, variant);
}

i32 Expr::getColumn() const {
    return std::visit(GetExprColumn{}, variant);
}

// =====================================================================
// Stmt实现
// =====================================================================

i32 Stmt::getLine() const {
    return std::visit(GetStmtLine{}, variant);
}

i32 Stmt::getColumn() const {
    return std::visit(GetStmtColumn{}, variant);
}

} // namespace Lua

