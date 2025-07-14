#include "visitor.hpp"
#include <iostream>
#include <sstream>

namespace Lua {
    // Utility visitor for counting nodes
    class NodeCounter : public ASTVisitor<int> {
    public:
        // Expression visitors
        int visitLiteralExpr(const LiteralExpr* expr) override {
            return 1;
        }
        
        int visitVariableExpr(const VariableExpr* expr) override {
            return 1;
        }
        
        int visitUnaryExpr(const UnaryExpr* expr) override {
            return 1 + visit(expr->getRight());
        }
        
        int visitBinaryExpr(const BinaryExpr* expr) override {
            return 1 + visit(expr->getLeft()) + visit(expr->getRight());
        }
        
        int visitCallExpr(const CallExpr* expr) override {
            int count = 1 + visit(expr->getCallee());
            for (const auto& arg : expr->getArguments()) {
                count += visit(arg.get());
            }
            return count;
        }
        
        int visitMemberExpr(const MemberExpr* expr) override {
            return 1 + visit(expr->getObject());
        }
        
        int visitTableExpr(const TableExpr* expr) override {
            int count = 1;
            for (const auto& field : expr->getFields()) {
                if (field.key) {
                    count += visit(field.key.get());
                }
                count += visit(field.value.get());
            }
            return count;
        }
        
        int visitIndexExpr(const IndexExpr* expr) override {
            return 1 + visit(expr->getObject()) + visit(expr->getIndex());
        }
        
        int visitFunctionExpr(const FunctionExpr* expr) override {
            return 1 + visit(expr->getBody());
        }

        int visitVarargExpr(const VarargExpr* expr) override {
            return 1;
        }

        // Statement visitors
        int visitExprStmt(const ExprStmt* stmt) override {
            return 1 + visit(stmt->getExpression());
        }
        
        int visitBlockStmt(const BlockStmt* stmt) override {
            int count = 1;
            for (const auto& statement : stmt->getStatements()) {
                count += visit(statement.get());
            }
            return count;
        }
        
        int visitLocalStmt(const LocalStmt* stmt) override {
            int count = 1;
            if (stmt->getInitializer()) {
                count += visit(stmt->getInitializer());
            }
            return count;
        }

        int visitMultiLocalStmt(const MultiLocalStmt* stmt) override {
            int count = 1;
            for (const auto& initializer : stmt->getInitializers()) {
                count += visit(initializer.get());
            }
            return count;
        }
        
        int visitAssignStmt(const AssignStmt* stmt) override {
            return 1 + visit(stmt->getTarget()) + visit(stmt->getValue());
        }
        
        int visitIfStmt(const IfStmt* stmt) override {
            int count = 1 + visit(stmt->getCondition()) + visit(stmt->getThenBranch());
            if (stmt->getElseBranch()) {
                count += visit(stmt->getElseBranch());
            }
            return count;
        }
        
        int visitReturnStmt(const ReturnStmt* stmt) override {
            int count = 1;
            // Count all return values
            for (const auto& value : stmt->getValues()) {
                count += visit(value.get());
            }
            return count;
        }
    };

    // Utility visitor for finding variables
    class VariableFinder : public ASTTraverser {
    private:
        Str targetName;
        bool found = false;
        
    public:
        explicit VariableFinder(const Str& name) : targetName(name) {}
        
        void visitVariableExpr(const VariableExpr* expr) override {
            if (expr->getName() == targetName) {
                found = true;
            }
        }
        
        void visitLocalStmt(const LocalStmt* stmt) override {
            if (stmt->getName() == targetName) {
                found = true;
            }
            ASTTraverser::visitLocalStmt(stmt);
        }

        void visitMultiLocalStmt(const MultiLocalStmt* stmt) override {
            for (const Str& name : stmt->getNames()) {
                if (name == targetName) {
                    found = true;
                    break;
                }
            }
            ASTTraverser::visitMultiLocalStmt(stmt);
        }

        bool isFound() const { return found; }
        
        void reset() { found = false; }
    };

    // Utility visitor for collecting all variable names
    class VariableCollector : public ASTTraverser {
    private:
        HashSet<Str> variables;
        
    public:
        void visitVariableExpr(const VariableExpr* expr) override {
            variables.insert(expr->getName());
        }
        
        void visitLocalStmt(const LocalStmt* stmt) override {
            variables.insert(stmt->getName());
            ASTTraverser::visitLocalStmt(stmt);
        }
        
        const HashSet<Str>& getVariables() const { return variables; }
        
        void clear() { variables.clear(); }
    };

    // Utility functions
    namespace ASTUtils {
        // Count total nodes in AST
        int countNodes(const Vec<UPtr<Stmt>>& statements) {
            NodeCounter counter;
            int total = 0;
            for (const auto& stmt : statements) {
                total += counter.visit(stmt.get());
            }
            return total;
        }
        
        int countNodes(const Stmt* stmt) {
            if (!stmt) return 0;
            NodeCounter counter;
            return counter.visit(stmt);
        }
        
        int countNodes(const Expr* expr) {
            if (!expr) return 0;
            NodeCounter counter;
            return counter.visit(expr);
        }
        
        // Check if a variable is used in AST
        bool hasVariable(const Vec<UPtr<Stmt>>& statements, const Str& varName) {
            VariableFinder finder(varName);
            for (const auto& stmt : statements) {
                finder.visit(stmt.get());
                if (finder.isFound()) {
                    return true;
                }
            }
            return false;
        }
        
        bool hasVariable(const Stmt* stmt, const Str& varName) {
            if (!stmt) return false;
            VariableFinder finder(varName);
            finder.visit(stmt);
            return finder.isFound();
        }
        
        bool hasVariable(const Expr* expr, const Str& varName) {
            if (!expr) return false;
            VariableFinder finder(varName);
            finder.visit(expr);
            return finder.isFound();
        }
        
        // Collect all variable names in AST
        HashSet<Str> collectVariables(const Vec<UPtr<Stmt>>& statements) {
            VariableCollector collector;
            for (const auto& stmt : statements) {
                collector.visit(stmt.get());
            }
            return collector.getVariables();
        }
        
        HashSet<Str> collectVariables(const Stmt* stmt) {
            VariableCollector collector;
            if (stmt) {
                collector.visit(stmt);
            }
            return collector.getVariables();
        }
        
        HashSet<Str> collectVariables(const Expr* expr) {
            VariableCollector collector;
            if (expr) {
                collector.visit(expr);
            }
            return collector.getVariables();
        }
        
        // Print AST to string
        Str printAST(const Vec<UPtr<Stmt>>& statements) {
            ASTPrinter printer;
            return printer.print(statements);
        }
        
        Str printAST(const Stmt* stmt) {
            if (!stmt) return "null";
            ASTPrinter printer;
            return printer.visit(stmt);
        }
        
        Str printAST(const Expr* expr) {
            if (!expr) return "null";
            ASTPrinter printer;
            return printer.visit(expr);
        }
        
        // Print AST to console
        void printASTToConsole(const Vec<UPtr<Stmt>>& statements) {
            std::cout << printAST(statements) << std::endl;
        }
        
        void printASTToConsole(const Stmt* stmt) {
            std::cout << printAST(stmt) << std::endl;
        }
        
        void printASTToConsole(const Expr* expr) {
            std::cout << printAST(expr) << std::endl;
        }
    }
}