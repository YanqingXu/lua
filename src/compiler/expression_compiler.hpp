#pragma once

#include "../common/types.hpp"
#include "../vm/instruction.hpp"
#include "../vm/value.hpp"
#include "../parser/ast/expressions.hpp"

namespace Lua {
    class Compiler; // Forward declaration
    
    // Expression compiler class
    class ExpressionCompiler {
    private:
        Compiler* compiler;
        
    public:
        explicit ExpressionCompiler(Compiler* compiler);
        
        // Compile different types of expressions
        int compileExpr(const Expr* expr);
        int compileLiteral(const LiteralExpr* expr);
        int compileVariable(const VariableExpr* expr);
        int compileUnary(const UnaryExpr* expr);
        int compileBinary(const BinaryExpr* expr);
        int compileCall(const CallExpr* expr);
        int compileCallWithReturnCount(const CallExpr* expr, int expectedReturns);
        void compileCallWithMultiReturn(const CallExpr* expr, int startReg, int expectedReturns);
        int compileTableConstructor(const TableExpr* expr);
        int compileIndexAccess(const IndexExpr* expr);
        int compileMemberAccess(const MemberExpr* expr);
        int compileFunctionExpr(const FunctionExpr* expr);
        int compileVararg(const VarargExpr* expr);
        
    private:
        // Helper methods for different operation types
        void compileArithmeticOp(TokenType op, int resultReg, int leftReg, int rightReg);
        void compileComparisonOp(TokenType op, int resultReg, int leftReg, int rightReg);
        int compileLogicalOp(const BinaryExpr* expr);

        
        // Optimization methods
        bool tryConstantFolding(const BinaryExpr* expr);
        Value evaluateConstantBinary(const BinaryExpr* expr);
        
    public:
        // Public methods for testing
        bool isConstantExpression(const Expr* expr);
        Value getConstantValue(const Expr* expr);
    };
}