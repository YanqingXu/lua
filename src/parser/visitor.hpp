#pragma once

#include <type_traits>
#include "parser.hpp"
#include "../common/types.hpp"

namespace Lua {
    // Forward declarations
    class Expr;
    class Stmt;
    class LiteralExpr;
    class VariableExpr;
    class UnaryExpr;
    class BinaryExpr;
    class CallExpr;
    class MemberExpr;
    class TableExpr;
    class IndexExpr;
    class FunctionExpr;
    class VarargExpr;
    class ExprStmt;
    class BlockStmt;
    class LocalStmt;
    class AssignStmt;
    class IfStmt;
    class ReturnStmt;

    // Abstract visitor interface for expressions
    template<typename R>
    class ExprVisitor {
    public:
        virtual ~ExprVisitor() = default;
        
        virtual R visitLiteralExpr(const LiteralExpr* expr) = 0;
        virtual R visitVariableExpr(const VariableExpr* expr) = 0;
        virtual R visitUnaryExpr(const UnaryExpr* expr) = 0;
        virtual R visitBinaryExpr(const BinaryExpr* expr) = 0;
        virtual R visitCallExpr(const CallExpr* expr) = 0;
        virtual R visitMemberExpr(const MemberExpr* expr) = 0;
        virtual R visitTableExpr(const TableExpr* expr) = 0;
        virtual R visitIndexExpr(const IndexExpr* expr) = 0;
        virtual R visitFunctionExpr(const FunctionExpr* expr) = 0;
        virtual R visitVarargExpr(const VarargExpr* expr) = 0;
    };

    // Abstract visitor interface for statements
    template<typename R>
    class StmtVisitor {
    public:
        virtual ~StmtVisitor() = default;
        
        virtual R visitExprStmt(const ExprStmt* stmt) = 0;
        virtual R visitBlockStmt(const BlockStmt* stmt) = 0;
        virtual R visitLocalStmt(const LocalStmt* stmt) = 0;
        virtual R visitMultiLocalStmt(const MultiLocalStmt* stmt) = 0;
        virtual R visitAssignStmt(const AssignStmt* stmt) = 0;
        virtual R visitIfStmt(const IfStmt* stmt) = 0;
        virtual R visitReturnStmt(const ReturnStmt* stmt) = 0;
    };

    // Combined visitor interface
    template<typename R>
    class ASTVisitor : public ExprVisitor<R>, public StmtVisitor<R> {
    public:
        virtual ~ASTVisitor() = default;
        
        // Dispatch methods for expressions
        R visit(const Expr* expr) {
            if (!expr) {
                if constexpr (std::is_void_v<R>) {
                    return;
                } else {
                    return R{};
                }
            }
            
            switch (expr->getType()) {
                case ExprType::Literal:
                    return this->visitLiteralExpr(static_cast<const LiteralExpr*>(expr));
                case ExprType::Variable:
                    return this->visitVariableExpr(static_cast<const VariableExpr*>(expr));
                case ExprType::Unary:
                    return this->visitUnaryExpr(static_cast<const UnaryExpr*>(expr));
                case ExprType::Binary:
                    return this->visitBinaryExpr(static_cast<const BinaryExpr*>(expr));
                case ExprType::Call:
                    return this->visitCallExpr(static_cast<const CallExpr*>(expr));
                case ExprType::Member:
                    return this->visitMemberExpr(static_cast<const MemberExpr*>(expr));
                case ExprType::Table:
                    return this->visitTableExpr(static_cast<const TableExpr*>(expr));
                case ExprType::Index:
                    return this->visitIndexExpr(static_cast<const IndexExpr*>(expr));
                case ExprType::Function:
                    return this->visitFunctionExpr(static_cast<const FunctionExpr*>(expr));
                case ExprType::Vararg:
                    return this->visitVarargExpr(static_cast<const VarargExpr*>(expr));
                default:
                    throw std::runtime_error("Unknown expression type");
            }
        }
        
        // Dispatch methods for statements
        R visit(const Stmt* stmt) {
            if (!stmt) {
                if constexpr (std::is_void_v<R>) {
                    return;
                } else {
                    return R{};
                }
            }
            
            switch (stmt->getType()) {
                case StmtType::Expression:
                    return this->visitExprStmt(static_cast<const ExprStmt*>(stmt));
                case StmtType::Block:
                    return this->visitBlockStmt(static_cast<const BlockStmt*>(stmt));
                case StmtType::Local:
                    return this->visitLocalStmt(static_cast<const LocalStmt*>(stmt));
                case StmtType::MultiLocal:
                    return this->visitMultiLocalStmt(static_cast<const MultiLocalStmt*>(stmt));
                case StmtType::Assign:
                    return this->visitAssignStmt(static_cast<const AssignStmt*>(stmt));
                case StmtType::If:
                    return this->visitIfStmt(static_cast<const IfStmt*>(stmt));
                case StmtType::Return:
                    return this->visitReturnStmt(static_cast<const ReturnStmt*>(stmt));
                default:
                    throw std::runtime_error("Unknown statement type");
            }
        }
    };

    // Concrete visitor for AST traversal (void return type)
    class ASTTraverser : public ASTVisitor<void> {
    public:
        virtual ~ASTTraverser() = default;
        
        // Expression visitors - default implementations traverse children
        void visitLiteralExpr(const LiteralExpr* expr) override {
            // Literals have no children to traverse
        }
        
        void visitVariableExpr(const VariableExpr* expr) override {
            // Variables have no children to traverse
        }
        
        void visitUnaryExpr(const UnaryExpr* expr) override {
            visit(expr->getRight());
        }
        
        void visitBinaryExpr(const BinaryExpr* expr) override {
            visit(expr->getLeft());
            visit(expr->getRight());
        }
        
        void visitCallExpr(const CallExpr* expr) override {
            visit(expr->getCallee());
            for (const auto& arg : expr->getArguments()) {
                visit(arg.get());
            }
        }
        
        void visitMemberExpr(const MemberExpr* expr) override {
            visit(expr->getObject());
        }
        
        void visitTableExpr(const TableExpr* expr) override {
            for (const auto& field : expr->getFields()) {
                if (field.key) {
                    visit(field.key.get());
                }
                visit(field.value.get());
            }
        }
        
        void visitIndexExpr(const IndexExpr* expr) override {
            visit(expr->getObject());
            visit(expr->getIndex());
        }
        
        void visitFunctionExpr(const FunctionExpr* expr) override {
            visit(expr->getBody());
        }

        void visitVarargExpr(const VarargExpr* expr) override {
            // Vararg expressions have no children to traverse
        }

        // Statement visitors - default implementations traverse children
        void visitExprStmt(const ExprStmt* stmt) override {
            visit(stmt->getExpression());
        }
        
        void visitBlockStmt(const BlockStmt* stmt) override {
            for (const auto& statement : stmt->getStatements()) {
                visit(statement.get());
            }
        }
        
        void visitLocalStmt(const LocalStmt* stmt) override {
            if (stmt->getInitializer()) {
                visit(stmt->getInitializer());
            }
        }

        void visitMultiLocalStmt(const MultiLocalStmt* stmt) override {
            for (const auto& initializer : stmt->getInitializers()) {
                visit(initializer.get());
            }
        }
        
        void visitAssignStmt(const AssignStmt* stmt) override {
            visit(stmt->getTarget());
            visit(stmt->getValue());
        }
        
        void visitIfStmt(const IfStmt* stmt) override {
            visit(stmt->getCondition());
            visit(stmt->getThenBranch());
            if (stmt->getElseBranch()) {
                visit(stmt->getElseBranch());
            }
        }
        
        void visitReturnStmt(const ReturnStmt* stmt) override {
            // Visit all return values
            for (const auto& value : stmt->getValues()) {
                visit(value.get());
            }
        }
        
        // Convenience method to traverse a list of statements
        void traverse(const Vec<UPtr<Stmt>>& statements) {
            for (const auto& stmt : statements) {
                visit(stmt.get());
            }
        }
    };

    // Concrete visitor for AST printing
    class ASTPrinter : public ASTVisitor<Str> {
    private:
        int indentLevel = 0;
        
        Str getIndent() const {
            return Str(indentLevel * 2, ' ');
        }
        
        void increaseIndent() { indentLevel++; }
        void decreaseIndent() { indentLevel--; }
        
    public:
        // Expression visitors
        Str visitLiteralExpr(const LiteralExpr* expr) override {
            return "Literal";
        }
        
        Str visitVariableExpr(const VariableExpr* expr) override {
            return "Variable(" + expr->getName() + ")";
        }
        
        Str visitUnaryExpr(const UnaryExpr* expr) override {
            return "Unary(" + visit(expr->getRight()) + ")";
        }
        
        Str visitBinaryExpr(const BinaryExpr* expr) override {
            return "Binary(" + visit(expr->getLeft()) + ", " + visit(expr->getRight()) + ")";
        }
        
        Str visitCallExpr(const CallExpr* expr) override {
            Str result = "Call(" + visit(expr->getCallee());
            for (const auto& arg : expr->getArguments()) {
                result += ", " + visit(arg.get());
            }
            result += ")";
            return result;
        }
        
        Str visitMemberExpr(const MemberExpr* expr) override {
            return "Member(" + visit(expr->getObject()) + "." + expr->getName() + ")";
        }
        
        Str visitTableExpr(const TableExpr* expr) override {
            return "Table";
        }
        
        Str visitIndexExpr(const IndexExpr* expr) override {
            return "Index(" + visit(expr->getObject()) + "[" + visit(expr->getIndex()) + "])";
        }
        
        Str visitFunctionExpr(const FunctionExpr* expr) override {
            Str result = "Function(";
            const auto& params = expr->getParameters();
            for (size_t i = 0; i < params.size(); ++i) {
                result += params[i];
                if (i < params.size() - 1) {
                    result += ", ";
                }
            }
            result += ") ";
            if (expr->getBody()) {
                result += "{\n";
                increaseIndent();
                result += visit(expr->getBody());
                decreaseIndent();
                result += "\n" + getIndent() + "}";
            } else {
                result += "{};";
            }
            return result;
        }

        Str visitVarargExpr(const VarargExpr* expr) override {
            return "Vararg(...)";
        }

        // Statement visitors
        Str visitExprStmt(const ExprStmt* stmt) override {
            return getIndent() + "ExprStmt(" + visit(stmt->getExpression()) + ")";
        }
        
        Str visitBlockStmt(const BlockStmt* stmt) override {
            Str result = getIndent() + "Block {\n";
            increaseIndent();
            for (const auto& statement : stmt->getStatements()) {
                result += visit(statement.get()) + "\n";
            }
            decreaseIndent();
            result += getIndent() + "}";
            return result;
        }
        
        Str visitLocalStmt(const LocalStmt* stmt) override {
            Str result = getIndent() + "Local(" + stmt->getName();
            if (stmt->getInitializer()) {
                result += " = " + visit(stmt->getInitializer());
            }
            result += ")";
            return result;
        }

        Str visitMultiLocalStmt(const MultiLocalStmt* stmt) override {
            Str result = getIndent() + "MultiLocal(";
            const auto& names = stmt->getNames();
            for (size_t i = 0; i < names.size(); ++i) {
                if (i > 0) result += ", ";
                result += names[i];
            }
            result += " = ";
            const auto& initializers = stmt->getInitializers();
            for (size_t i = 0; i < initializers.size(); ++i) {
                if (i > 0) result += ", ";
                result += visit(initializers[i].get());
            }
            result += ")";
            return result;
        }
        
        Str visitAssignStmt(const AssignStmt* stmt) override {
            return getIndent() + "Assign(" + visit(stmt->getTarget()) + " = " + visit(stmt->getValue()) + ")";
        }
        
        Str visitIfStmt(const IfStmt* stmt) override {
            Str result = getIndent() + "If(" + visit(stmt->getCondition()) + ") {\n";
            increaseIndent();
            result += visit(stmt->getThenBranch()) + "\n";
            decreaseIndent();
            if (stmt->getElseBranch()) {
                result += getIndent() + "} else {\n";
                increaseIndent();
                result += visit(stmt->getElseBranch()) + "\n";
                decreaseIndent();
            }
            result += getIndent() + "}";
            return result;
        }
        
        Str visitReturnStmt(const ReturnStmt* stmt) override {
            Str result = getIndent() + "Return";
            if (stmt->hasValues()) {
                result += "(";
                const auto& values = stmt->getValues();
                for (size_t i = 0; i < values.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += visit(values[i].get());
                }
                result += ")";
            }
            return result;
        }
        
        // Print entire AST
        Str print(const Vec<UPtr<Stmt>>& statements) {
            Str result = "AST {\n";
            increaseIndent();
            for (const auto& stmt : statements) {
                result += visit(stmt.get()) + "\n";
            }
            decreaseIndent();
            result += "}";
            return result;
        }
    };

    namespace ASTUtils {
        // Forward declare or include necessary types if not already visible
        // (e.g., Vec, UPtr, Stmt, Expr, Str, HashSet)
    
        Str printAST(const Vec<UPtr<Stmt>>& statements);
        Str printAST(const Stmt* stmt);
        Str printAST(const Expr* expr);
    
        int countNodes(const Vec<UPtr<Stmt>>& statements);
        int countNodes(const Stmt* stmt);
        int countNodes(const Expr* expr);
    
        bool hasVariable(const Vec<UPtr<Stmt>>& statements, const Str& varName);
        bool hasVariable(const Stmt* stmt, const Str& varName);
        bool hasVariable(const Expr* expr, const Str& varName);
    
        HashSet<Str> collectVariables(const Vec<UPtr<Stmt>>& statements);
        HashSet<Str> collectVariables(const Stmt* stmt);
        HashSet<Str> collectVariables(const Expr* expr);
    
        void printASTToConsole(const Vec<UPtr<Stmt>>& statements);
        void printASTToConsole(const Stmt* stmt);
        void printASTToConsole(const Expr* expr);
    } // namespace ASTUtils
    
    } // namespace Lua