#pragma once

#include "../common/types.hpp"
#include "../parser/parser.hpp"
#include "../vm/function.hpp"
#include "../common/opcodes.hpp"
#include "compiler_utils.hpp"
#include "register_manager.hpp"
#include "symbol_table.hpp"
#include <iostream>

namespace Lua {
    // Forward declarations
    class ExpressionCompiler;
    class StatementCompiler;
    class Expr;
    class Stmt;

    // Forward declaration
    class Compiler;

    // Compilation context for nested function support
    struct CompilationContext {
        ScopeManager* parentScope;              // Parent scope manager
        Vec<UpvalueDescriptor>* parentUpvalues; // Parent function's upvalues
        Compiler* parentCompiler;               // Parent compiler instance

        CompilationContext(ScopeManager* scope, Vec<UpvalueDescriptor>* upvalues, Compiler* compiler)
            : parentScope(scope), parentUpvalues(upvalues), parentCompiler(compiler) {}
    };

    // Compiler class
    class Compiler {
    private:
        // Unified scope management
        ScopeManager scopeManager_;

        // Current upvalues for this function
        Vec<UpvalueDescriptor> currentUpvalues_;

        // Constant table
        Vec<Value> constants;

        // Bytecode
        Ptr<Vec<Instruction>> code;

        // Function prototypes for nested functions
        Vec<GCRef<Function>> prototypes;

        // Jump instruction positions for current block
        Vec<int> breaks;

        // Lua 5.1官方寄存器管理器
        RegisterManager registerManager_;

        // Function nesting depth tracking
        int functionNestingDepth;

        // Compilation context for nested functions
        Ptr<CompilationContext> parentContext_;

        // Utility helper
        CompilerUtils utils;

        // Compiler modules
        UPtr<ExpressionCompiler> exprCompiler;
        UPtr<StatementCompiler> stmtCompiler;
        
    public:
        explicit Compiler();
        explicit Compiler(Ptr<CompilationContext> parentContext);
        ~Compiler();

        // Main compilation interface
        GCRef<Function> compile(const Vec<UPtr<Stmt>>& statements);

        // Compilation methods
        int compileExpr(const Expr* expr);
        void compileStmt(const Stmt* stmt);

        // Access to compiler modules
        ExpressionCompiler* getExpressionCompiler() const { return exprCompiler.get(); }
        StatementCompiler* getStatementCompiler() const { return stmtCompiler.get(); }
        
        // Register management (Lua 5.1官方设计)
        int allocReg() {
            return registerManager_.allocateTemp("expr");
        }
        int allocTempReg(const std::string& name = "temp") {
            return registerManager_.allocateTemp(name);
        }
        int allocLocalReg(const std::string& name = "") {
            return registerManager_.allocateLocal(name);
        }
        void freeTempReg() {
            registerManager_.freeTemp();
        }
        void freeReg() {  // 保持兼容性
            registerManager_.freeTemp();
        }
        int getNextReg() const {
            return registerManager_.getStackTop();
        }
        int getLocalCount() const {
            return registerManager_.getLocalCount();
        }



        // 寄存器管理器访问
        RegisterManager& getRegisterManager() { return registerManager_; }
        
        // Constant management
        int addConstant(const Value& value);

        // Variable resolution (unified local/upvalue/global)
        enum class VariableType { Local, Upvalue, Global };
        struct VariableInfo {
            VariableType type;
            int index;  // Register index for local, upvalue index for upvalue, constant index for global

            VariableInfo(VariableType t, int i) : type(t), index(i) {}
        };
        VariableInfo resolveVariable(const Str& name);

        // Scope management
        void beginScope() { scopeManager_.enterScope(); }
        void endScope() { scopeManager_.exitScope(); }

        // Local variable management
        int defineLocal(const Str& name, int stackIndex = -1);

        // Upvalue management
        int addUpvalue(const Str& name, bool isLocal, int index);
        const Vec<UpvalueDescriptor>& getCurrentUpvalues() const { return currentUpvalues_; }

        // Scope manager access
        ScopeManager& getScopeManager() { return scopeManager_; }
        
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

        // Context access for nested compilation
        Ptr<CompilationContext> createChildContext() {
            return std::make_shared<CompilationContext>(&scopeManager_, &currentUpvalues_, this);
        }
        
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
