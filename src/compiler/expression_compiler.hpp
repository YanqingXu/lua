#pragma once

#include "../types.hpp"
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
        int compileTableConstructor(const TableExpr* expr);
        int compileIndexAccess(const IndexExpr* expr);
        int compileMemberAccess(const MemberExpr* expr);
        
    private:
        // Helper methods for expression compilation
        int compileArithmeticOp(const BinaryExpr* expr);
        int compileComparisonOp(const BinaryExpr* expr);
        int compileLogicalOp(const BinaryExpr* expr);
        int compileConcatenationOp(const BinaryExpr* expr);
    };
}