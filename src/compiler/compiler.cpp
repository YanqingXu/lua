#include "compiler.hpp"
#include "expression_compiler.hpp"
#include "statement_compiler.hpp"
#include "../vm/instruction.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>
#include <iostream>

namespace Lua {
    Compiler::Compiler() :
        scopeManager_(),
        functionNestingDepth(0),
        code(std::make_shared<Vec<Instruction>>()),
        parentContext_(nullptr),
        utils(),
        registerManager_() {  // Lua 5.1官方寄存器管理器
        initializeModules();
    }

    Compiler::Compiler(Ptr<CompilationContext> parentContext) :
        scopeManager_(),
        functionNestingDepth(0),
        code(std::make_shared<Vec<Instruction>>()),
        parentContext_(parentContext),
        utils(),
        registerManager_() {
        initializeModules();

        // If we have a parent context, inherit the parent scope chain
        if (parentContext_ && parentContext_->parentScope) {
            // Set the parent scope in our scope manager
            // This allows isFreeVariable to work correctly
            scopeManager_.setParentScope(parentContext_->parentScope);
        }
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
    
    int Compiler::defineLocal(const Str& name, int stackIndex) {
        if (stackIndex == -1) {
            stackIndex = registerManager_.allocateLocal(name);
        }

        // std::cout << "[DEBUG] DEFINE LOCAL: name='" << name << "', stackIndex=" << stackIndex << std::endl;

        bool success = scopeManager_.defineLocal(name, stackIndex);
        if (!success) {
            // std::cout << "[DEBUG] DEFINE LOCAL FAILED: name='" << name << "'" << std::endl;
            throw LuaException("Failed to define local variable: " + name);
        }

        // std::cout << "[DEBUG] DEFINE LOCAL SUCCESS: name='" << name << "', stackIndex=" << stackIndex << std::endl;
        return stackIndex;
    }

    int Compiler::addUpvalue(const Str& name, bool isLocal, int index) {
        // Check if upvalue already exists
        for (size_t i = 0; i < currentUpvalues_.size(); ++i) {
            if (currentUpvalues_[i].name == name) {
                return static_cast<int>(i);
            }
        }

        // Add new upvalue
        int upvalueIndex = static_cast<int>(currentUpvalues_.size());
        currentUpvalues_.emplace_back(name, upvalueIndex, isLocal, index);

        return upvalueIndex;
    }

    Compiler::VariableInfo Compiler::resolveVariable(const Str& name) {
        // 1. Check if it's a local variable in current function
        Variable* localVar = scopeManager_.findVariable(name);

        if (localVar) {
            // We found the variable, now check if it's in current function or parent function
            if (parentContext_) {
                // We're in a nested function
                // Check if this variable was defined in the current function
                // by checking if it's NOT in the parent scope
                Variable* parentVar = parentContext_->parentScope->findVariable(name);
                bool isInParent = (parentVar != nullptr);

                if (!isInParent) {
                    // Variable is not in parent scope, so it's local to current function
                    return VariableInfo(VariableType::Local, localVar->stackIndex);
                }
                // If it's in parent scope, it will be handled as upvalue below
            } else {
                // We're in the main function - all found variables are local
                return VariableInfo(VariableType::Local, localVar->stackIndex);
            }
        }

        // 2. Check if it's an upvalue (variable from parent function)
        // Only check if we have a parent context (i.e., we're in a nested function)
        if (parentContext_ && parentContext_->parentScope) {
            // Check if variable exists in parent function's scope
            Variable* parentVar = parentContext_->parentScope->findVariable(name);
            if (parentVar) {
                // Found in parent function - create upvalue
                bool isLocal = parentContext_->parentScope->isInCurrentScope(name);
                int sourceIndex = parentVar->stackIndex;

                if (!isLocal) {
                    // It's an upvalue in the parent, need to find it in parent's upvalues
                    if (parentContext_->parentUpvalues) {
                        for (size_t i = 0; i < parentContext_->parentUpvalues->size(); ++i) {
                            if ((*parentContext_->parentUpvalues)[i].name == name) {
                                sourceIndex = static_cast<int>(i);
                                break;
                            }
                        }
                    }
                }

                // Add this as an upvalue in current function
                int upvalueIndex = addUpvalue(name, isLocal, sourceIndex);
                return VariableInfo(VariableType::Upvalue, upvalueIndex);
            }
        }

        // 3. It's a global variable
        int constantIndex = addConstant(Value(name));
        return VariableInfo(VariableType::Global, constantIndex);
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
            // Initialize global scope for main chunk
            beginScope();

            // Compile each statement
            for (const auto& stmt : statements) {
                stmtCompiler->compileStmt(stmt.get());
            }

            // Add return instruction for main chunk
            // Main chunk should return nil without affecting global variables
            emitInstruction(Instruction::createRETURN(0, 1));

            // End global scope
            endScope();

            // Create function object with proper parameters
            // Main function has 0 parameters and includes all nested function prototypes
            return Function::createLua(code, constants, prototypes, 0,
                                     static_cast<u8>(scopeManager_.getLocalCount()),
                                     static_cast<u8>(currentUpvalues_.size()));
        } catch (const LuaException& e) {
            // Compilation error, return nullptr
            CompilerUtils::reportCompilerError(e.what());
            return nullptr;
        }
    }
}
