#pragma once

#include "../types.hpp"
#include "../parser/parser.hpp"
#include "../vm/function.hpp"
#include "../common/opcodes.hpp"

namespace Lua {
    // Local variable
    struct Local {
        Str name;
        int depth;
        bool isCaptured;
        int slot;    // Register index
        
        Local(const Str& name, int depth, int slot)
            : name(name), depth(depth), isCaptured(false), slot(slot) {}
    };

    class Expr;
    class LiteralExpr;
    class VariableExpr;
    class UnaryExpr;
    class BinaryExpr;
    class CallExpr;
    class ExprStmt;
    class BlockStmt;
    class LocalStmt;
    class ForInStmt;
class RepeatUntilStmt;
    class Stmt;

    
    // Compiler class
    class Compiler {
    private:
        // Current scope depth
        int scopeDepth;
        
        // Local variables
        Vec<Local> locals;
        
        // Constant table
        Vec<Value> constants;
        
        // Bytecode
        Ptr<Vec<Instruction>> code;
        
        // Jump instruction positions for current block
        Vec<int> breaks;
        
        // Compile expressions
        int compileExpr(const Expr* expr);
        int compileLiteral(const LiteralExpr* expr);
        int compileVariable(const VariableExpr* expr);
        int compileUnary(const UnaryExpr* expr);
        int compileBinary(const BinaryExpr* expr);
        int compileCall(const CallExpr* expr);
        
        // Compile statements
        void compileStmt(const Stmt* stmt);
        void compileExprStmt(const ExprStmt* stmt);
        void compileBlockStmt(const BlockStmt* stmt);
        void compileLocalStmt(const LocalStmt* stmt);
        void compileForInStmt(const ForInStmt* stmt);
        void compileRepeatUntilStmt(const RepeatUntilStmt* stmt);
        
        // Helper methods
        int addConstant(const Value& value);
        void emitInstruction(const Instruction& instr);
        int emitJump();
        void patchJump(int from);
        
        // Register management
        int nextReg = 0;
        int allocReg() { return nextReg++; }
        void freeReg() { if (nextReg>0) --nextReg; }
        
        void beginScope();
        void endScope();
        int resolveLocal(const Str& name);
        
    public:
        Compiler();
        
        // Compile AST, generate function object
        Ptr<Function> compile(const Vec<UPtr<Stmt>>& statements);
    };
}
