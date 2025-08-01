﻿#pragma once

#include "../common/types.hpp"
#include "../parser/ast/ast_base.hpp"
#include "../parser/ast/expressions.hpp"
#include "../parser/ast/statements.hpp"
#include "../compiler/symbol_table.hpp"
#include <iostream>

namespace Lua {
    /**
     * RAII Scope Guard for automatic scope management
     * Ensures proper scope entry and exit even in case of exceptions
     */
    class ScopeGuard {
    private:
        ScopeManager& scopeManager_;
        
    public:
        explicit ScopeGuard(ScopeManager& sm) : scopeManager_(sm) {
            scopeManager_.enterScope();
        }
        
        ~ScopeGuard() {
            scopeManager_.exitScope();
        }
        
        // Non-copyable and non-movable
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard(ScopeGuard&&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) = delete;
    };

    /**
     * UpvalueAnalyzer - Analyzes function AST nodes to identify free variables for closure creation
     *
     * This class is responsible for:
     * 1. Traversing function body AST to identify all variable references
     * 2. Distinguishing between local variables and free variables
     * 3. Creating upvalue descriptors for free variables
     * 4. Supporting upvalue analysis for nested functions
     *
     * Design principles:
     * - Uses dependency injection: ScopeManager is provided by caller
     * - Caller is responsible for ScopeManager lifetime management
     * - Simple and reliable: only one construction method
     * - RAII scope management for exception safety
     */
    class UpvalueAnalyzer {
    private:
        ScopeManager& scopeManager_;      // Reference to external ScopeManager
        Vec<UpvalueDescriptor> upvalues_; // Current function's upvalue list
        HashSet<Str> freeVars_;          // Free variables set

    public:
        // Constructor: accepts ScopeManager reference
        // Caller is responsible for ScopeManager lifetime
        explicit UpvalueAnalyzer(ScopeManager& scopeManager)
            : scopeManager_(scopeManager) {}
        
        // Non-copyable and non-movable to ensure reference validity
        UpvalueAnalyzer(const UpvalueAnalyzer&) = delete;
        UpvalueAnalyzer& operator=(const UpvalueAnalyzer&) = delete;
        UpvalueAnalyzer(UpvalueAnalyzer&&) = delete;
        UpvalueAnalyzer& operator=(UpvalueAnalyzer&&) = delete;
        
        // Direct access to ScopeManager
        ScopeManager& getScopeManager() {
            return scopeManager_;
        }
        
        const ScopeManager& getScopeManager() const {
            return scopeManager_;
        }

        // Analyze function expression and return upvalue descriptor list
        Vec<UpvalueDescriptor> analyzeFunction(const FunctionExpr* funcExpr);

        // Analyze function statement and return upvalue descriptor list
        Vec<UpvalueDescriptor> analyzeFunction(const FunctionStmt* funcStmt);
        
        // Get current upvalues
        const Vec<UpvalueDescriptor>& getUpvalues() const { return upvalues_; }

        // Check if variable is a local variable
        bool isLocalVariable(const Str& name) const;

        // Check if variable is a free variable
        bool isFreeVariable(const Str& name) const;

    private:
        // Analyze variable references in expressions
        void analyzeExpression(const Expr* expr);

        // Analyze variable references in statements
        void analyzeStatement(const Stmt* stmt);

        // Analyze variable expressions
        void analyzeVariableExpr(const VariableExpr* varExpr);

        // Analyze binary expressions
        void analyzeBinaryExpr(const BinaryExpr* binExpr);

        // Analyze unary expressions
        void analyzeUnaryExpr(const UnaryExpr* unaryExpr);

        // Analyze function call expressions
        void analyzeCallExpr(const CallExpr* callExpr);

        // Analyze member access expressions
        void analyzeMemberExpr(const MemberExpr* memberExpr);

        // Analyze index access expressions
        void analyzeIndexExpr(const IndexExpr* indexExpr);

        // Analyze table construction expressions
        void analyzeTableExpr(const TableExpr* tableExpr);

        // Analyze block statements
        void analyzeBlockStmt(const BlockStmt* blockStmt);

        // Analyze local variable declaration statements
        void analyzeLocalStmt(const LocalStmt* localStmt);

        // Analyze multi-local variable declaration statements
        void analyzeMultiLocalStmt(const MultiLocalStmt* multiLocalStmt);

        // Analyze assignment statements
        void analyzeAssignStmt(const AssignStmt* assignStmt);

        // Analyze if statements
        void analyzeIfStmt(const IfStmt* ifStmt);

        // Analyze while statements
        void analyzeWhileStmt(const WhileStmt* whileStmt);

        // Analyze for statements
        void analyzeForStmt(const ForStmt* forStmt);

        // Analyze for-in statements
        void analyzeForInStmt(const ForInStmt* forInStmt);

        // Analyze return statements
        void analyzeReturnStmt(const ReturnStmt* returnStmt);

        // Analyze repeat-until statements
        void analyzeRepeatUntilStmt(const RepeatUntilStmt* repeatStmt);

        // Analyze function definition statements
        void analyzeFunctionStmt(const FunctionStmt* funcStmt);

        // Create upvalue descriptor for free variables
        UpvalueDescriptor createUpvalueDescriptor(const Str& name);

        // Clean up analysis state
        void reset();
    };
}