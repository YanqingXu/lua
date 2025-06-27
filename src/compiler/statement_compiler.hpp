#pragma once

#include "../common/types.hpp"
#include "../vm/instruction.hpp"
#include "../parser/ast/statements.hpp"

namespace Lua {
    class Compiler; // Forward declaration
    
    // Statement compiler class
    class StatementCompiler {
    private:
        Compiler* compiler;
        
    public:
        explicit StatementCompiler(Compiler* compiler);
        
        // Compile different types of statements
        void compileStmt(const Stmt* stmt);
        void compileExprStmt(const ExprStmt* stmt);
        void compileBlockStmt(const BlockStmt* stmt);
        void compileLocalStmt(const LocalStmt* stmt);
        void compileAssignmentStmt(const AssignStmt* stmt);
        void compileIfStmt(const IfStmt* stmt);
        void compileWhileStmt(const WhileStmt* stmt);
        void compileForStmt(const ForStmt* stmt);
        void compileForInStmt(const ForInStmt* stmt);
        void compileRepeatUntilStmt(const RepeatUntilStmt* stmt);
        void compileReturnStmt(const ReturnStmt* stmt);
        void compileBreakStmt(const BreakStmt* stmt);
        void compileFunctionStmt(const FunctionStmt* stmt);
        void compileDoStmt(const DoStmt* stmt);
        
    private:
        // Helper methods for control flow
        void compileConditionalJump(const Expr* condition, bool jumpIfTrue, int& jumpAddr);
        void compileLoopBody(const Stmt* body, int loopStart);
        void handleBreakStatements(int loopEnd);
        
        // Helper methods for nested conditions
        void compileNestedCondition(const Expr* condition, int& jumpToElse, int& jumpToEnd);
        void patchConditionalJumps(int jumpToElse, int jumpToEnd, bool hasElse);
    };
}