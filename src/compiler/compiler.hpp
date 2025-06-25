#pragma once

#include "../common/types.hpp"
#include "../parser/parser.hpp"
#include "../vm/function.hpp"
#include "../common/opcodes.hpp"
#include "compiler_utils.hpp"
#include <iostream>

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
        
        // Function prototypes for nested functions
        Vec<GCRef<Function>> prototypes;
        
        // Jump instruction positions for current block
        Vec<int> breaks;
        
        // Next available register
        int nextRegister;
        
        // Function nesting depth tracking
        int functionNestingDepth;
        
        // Utility helper
        CompilerUtils utils;
        
        // Compiler modules
        UPtr<ExpressionCompiler> exprCompiler;
        UPtr<StatementCompiler> stmtCompiler;
        
    public:
        explicit Compiler();
        ~Compiler();
        
        // Main compilation interface
        GCRef<Function> compile(const Vec<UPtr<Stmt>>& statements);
        
        // Compilation methods
        int compileExpr(const Expr* expr);
        void compileStmt(const Stmt* stmt);
        
        // Access to compiler modules
        ExpressionCompiler* getExpressionCompiler() const { return exprCompiler.get(); }
        StatementCompiler* getStatementCompiler() const { return stmtCompiler.get(); }
        
        // Register management (简化版本)
        int allocReg() {
            int reg = nextRegister++;
            return reg;
        }
        void freeReg() {
            if (nextRegister > 1) {
                nextRegister--;
            }
        }
        int getNextReg() const { return nextRegister; }
        
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
        
        // Function prototype management
        int addPrototype(GCRef<Function> prototype) {
            prototypes.push_back(prototype);
            return static_cast<int>(prototypes.size() - 1);
        }
        
        const Vec<GCRef<Function>>& getPrototypes() const { return prototypes; }
        
        // Code access
        size_t getCodeSize() const { return code->size(); }
        size_t getConstantCount() const { return constants.size(); }
        
        // Access to internal data for function compilation
        const Ptr<Vec<Instruction>>& getCode() const { return code; }
        const Vec<Value>& getConstants() const { return constants; }
        const Vec<Local>& getLocals() const { return locals; }
        
        // Function nesting depth management
        void enterFunctionScope();
        void exitFunctionScope();
        int getFunctionNestingDepth() const { return functionNestingDepth; }
        
    private:
        // Initialize compiler modules
        void initializeModules();
        
        // Check function nesting depth limit
        void checkFunctionNestingDepth() const;
        
    };
}
