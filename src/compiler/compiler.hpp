#pragma once

#include "../types.hpp"
#include "../parser/parser.hpp"
#include "../vm/function.hpp"
#include "../common/opcodes.hpp"
#include "compiler_utils.hpp"

namespace Lua {
    // Forward declarations
    class ExpressionCompiler;
    class StatementCompiler;
    class Expr;
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
        
        // Next available register
        int nextRegister;
        
        // Utility helper
        CompilerUtils utils;
        
        // Compiler modules
        UPtr<ExpressionCompiler> exprCompiler;
        UPtr<StatementCompiler> stmtCompiler;
        
    public:
        explicit Compiler();
        ~Compiler();
        
        // Main compilation interface
        Ptr<Function> compile(const Vec<UPtr<Stmt>>& statements);
        
        // Compilation methods
        int compileExpr(const Expr* expr);
        void compileStmt(const Stmt* stmt);
        
        // Access to compiler modules
        ExpressionCompiler* getExpressionCompiler() const { return exprCompiler.get(); }
        StatementCompiler* getStatementCompiler() const { return stmtCompiler.get(); }
        
        // Register management
        int allocReg() { return utils.allocateRegister(nextRegister); }
        void freeReg() { utils.freeRegister(nextRegister); }
        
        // Constant management
        int addConstant(const Value& value);
        
        // Local variable management
        int resolveLocal(const Str& name);
        void addLocal(const Str& name, int slot) { utils.addLocal(locals, name, scopeDepth, slot); }
        
        // Scope management
        void beginScope();
        void endScope();
        
        // Instruction emission
        void emitInstruction(const Instruction& instr);
        int emitJump();
        void patchJump(int jumpAddr);
        
        // Break statement support
        void addBreakJump(int jumpAddr) { breaks.push_back(jumpAddr); }
        void patchBreakJumps(int targetAddr) {
            for (int jumpAddr : breaks) {
                utils.patchJump(*code, jumpAddr, targetAddr);
            }
            breaks.clear();
        }
        
        // Code access
        size_t getCodeSize() const { return code->size(); }
        size_t getConstantCount() const { return constants.size(); }
        
    private:
        // Initialize compiler modules
        void initializeModules();
        
    };
}
