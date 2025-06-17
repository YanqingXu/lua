#pragma once

#include "../common/types.hpp"
#include "../parser/ast/ast_base.hpp"
#include "../parser/ast/expressions.hpp"
#include "../parser/ast/statements.hpp"
#include "../compiler/symbol_table.hpp"
#include <iostream>

namespace Lua {
    /**
     * UpvalueAnalyzer - Analyzes function AST nodes to identify free variables for closure creation
     *
     * This class is responsible for:
     * 1. Traversing function body AST to identify all variable references
     * 2. Distinguishing between local variables and free variables
     * 3. Creating upvalue descriptors for free variables
     * 4. Supporting upvalue analysis for nested functions
     */
    class UpvalueAnalyzer {
    private:
        ScopeManager& scopeManager_;           // Scope manager
        Vec<UpvalueDescriptor> upvalues_; // Current function's upvalue list
        HashSet<Str> freeVars_; // Free variables set

    public:
        explicit UpvalueAnalyzer(ScopeManager& scopeManager);

        // Analyze function expression and return upvalue descriptor list
        Vec<UpvalueDescriptor> analyzeFunction(const FunctionExpr* funcExpr);

        // Analyze function statement and return upvalue descriptor list
        Vec<UpvalueDescriptor> analyzeFunction(const FunctionStmt* funcStmt);

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