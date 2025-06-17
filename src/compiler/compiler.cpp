#include "compiler.hpp"
#include "expression_compiler.hpp"
#include "statement_compiler.hpp"
#include "../vm/instruction.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>

namespace Lua {
    Compiler::Compiler() : 
        scopeDepth(0),
        nextRegister(0),
        code(std::make_shared<Vec<Instruction>>()),
        utils() {
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
    }
    
    void Compiler::endScope() {
        utils.exitScope(scopeDepth, locals);
    }
    
    int Compiler::resolveLocal(const Str& name) {
        return utils.resolveLocal(locals, name, scopeDepth);
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
            
            // Add return instruction
            emitInstruction(Instruction::createRETURN(0, 0));
            
            // Create function object
            return Function::createLua(code, constants, {});
        } catch (const LuaException& e) {
            // Compilation error, return nullptr
            CompilerUtils::reportCompilerError(e.what());
            return nullptr;
        }
    }
}
