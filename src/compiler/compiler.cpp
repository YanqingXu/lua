#include "compiler.hpp"
#include "expression_compiler.hpp"
#include "statement_compiler.hpp"
#include "../vm/instruction.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>
#include <iostream>

namespace Lua {
    Compiler::Compiler() :
        scopeDepth(0),
        functionNestingDepth(0),
        code(std::make_shared<Vec<Instruction>>()),
        utils(),
        registerManager_() {  // Lua 5.1官方寄存器管理器
        initializeModules();
    }
    
    Compiler::~Compiler() = default;
    
    void Compiler::initializeModules() {
        exprCompiler = std::make_unique<ExpressionCompiler>(this);
        stmtCompiler = std::make_unique<StatementCompiler>(this);
    }
    
    // Utility methods delegated to CompilerUtils
    int Compiler::addConstant(const Value& value) {
        return utils.addConstant(constants, value);
    }
    
    void Compiler::emitInstruction(const Instruction& instr) {
        utils.emitInstruction(*code, instr);
    }
    
    int Compiler::emitJump() {
        return utils.createJumpPlaceholder(*code);
    }
    
    void Compiler::patchJump(int from) {
        utils.patchJump(*code, from, static_cast<int>(code->size()));
    }
    
    void Compiler::beginScope() {
        utils.enterScope(scopeDepth);
        registerManager_.enterScope();
    }

    void Compiler::endScope() {
        utils.exitScope(scopeDepth, locals);
        registerManager_.exitScope();
    }
    
    int Compiler::resolveLocal(const Str& name) {
        return utils.resolveLocal(locals, name, scopeDepth);
    }
    
    void Compiler::enterFunctionScope() {
        functionNestingDepth++;
        checkFunctionNestingDepth();
    }
    
    void Compiler::exitFunctionScope() {
        if (functionNestingDepth > 0) {
            functionNestingDepth--;
        }
    }
    
    void Compiler::checkFunctionNestingDepth() const {
        if (functionNestingDepth > MAX_FUNCTION_NESTING_DEPTH) {
            throw std::runtime_error("Function nesting depth exceeded: " + 
                std::to_string(MAX_FUNCTION_NESTING_DEPTH) + 
                " (current depth: " + std::to_string(functionNestingDepth) + ")");
        }
    }
    
    // All compilation methods have been moved to specialized compilers
    // Expression compilation delegated to ExpressionCompiler
    int Compiler::compileExpr(const Expr* expr) {
        return exprCompiler->compileExpr(expr);
    }
    
    // Statement compilation delegated to StatementCompiler
    void Compiler::compileStmt(const Stmt* stmt) {
        stmtCompiler->compileStmt(stmt);
    }
    
    GCRef<Function> Compiler::compile(const Vec<UPtr<Stmt>>& statements) {
        try {
            // Compile each statement
            for (const auto& stmt : statements) {
                stmtCompiler->compileStmt(stmt.get());
            }
            
            // Add return instruction for main chunk
            // Main chunk should return nil without affecting global variables
            emitInstruction(Instruction::createRETURN(0, 1));
            
            // Create function object with proper parameters
            // Main function has 0 parameters and includes all nested function prototypes
            return Function::createLua(code, constants, prototypes, 0);
        } catch (const LuaException& e) {
            // Compilation error, return nullptr
            CompilerUtils::reportCompilerError(e.what());
            return nullptr;
        }
    }
}
