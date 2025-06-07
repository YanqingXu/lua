#include "compiler.hpp"

namespace Lua {
    Compiler::Compiler() : scopeDepth(0) {
        code = make_ptr<std::vector<Instruction>>();
    }
    
    int Compiler::addConstant(const Value& value) {
        constants.push_back(value);
        return static_cast<int>(constants.size()) - 1;
    }
    
    void Compiler::emitInstruction(const Instruction& instr) {
        code->push_back(instr);
    }
    
    int Compiler::emitJump() {
        // Emit placeholder jump with sBx = 0, will patch later.
        emitInstruction(Instruction::createJMP(0));
        return static_cast<int>(code->size()) - 1;
    }
    
    void Compiler::patchJump(int from) {
        int to = static_cast<int>(code->size());
        int sbx = to - from - 1; // distance to jump over next instructions
        if (sbx < -32768 || sbx > 32767) {
            throw LuaException("Jump offset out of range");
        }
        (*code)[from].setSBx(static_cast<i16>(sbx));
    }
    
    void Compiler::beginScope() {
        scopeDepth++;
    }
    
    void Compiler::endScope() {
        scopeDepth--;
        
        // Remove local variables from current scope
        while (!locals.empty() && locals.back().depth > scopeDepth) {
            // If variable is captured, emit close upvalue instruction
            if (locals.back().isCaptured) {
                emitInstruction(Instruction::createCLOSE(locals.back().slot));
            } else {
                emitInstruction(Instruction::createPOP(locals.back().slot));
            }
            locals.pop_back();
        }
    }
    
    int Compiler::resolveLocal(const Str& name) {
        // Search backwards to find variable name
        for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
            if (locals[i].name == name) {
                return i;
            }
        }
        
        return -1; // Not found
    }
    
    int Compiler::compileLiteral(const LiteralExpr* expr) {
        const Value& value = expr->getValue();
        
        int dst = allocReg();
        
        if (value.isNil()) {
            emitInstruction(Instruction::createLOADNIL(dst));
        } else if (value.isBoolean()) {
            emitInstruction(Instruction::createLOADBOOL(dst, value.asBoolean()));
        } else if (value.isNumber() || value.isString()) {
            int constant = addConstant(value);
            emitInstruction(Instruction::createLOADK(dst, static_cast<u16>(constant)));
        } else {
            throw LuaException("Unsupported literal type.");
        }
        return dst;
    }
    
    int Compiler::compileVariable(const VariableExpr* expr) {
        // Find variable name
        int slot = resolveLocal(expr->getName());
        
        if (slot != -1) {
            // Local variable directly returns its register number
            return slot;
        } else {
            int dst = allocReg();
            int constant = addConstant(expr->getName());
            emitInstruction(Instruction::createGETGLOBAL(dst, static_cast<u16>(constant)));
            return dst;
        }
    }
    
    int Compiler::compileUnary(const UnaryExpr* expr) {
        // Compile operand first
        int rhs = compileExpr(expr->getRight());
        int dst = allocReg();
        
        switch (expr->getOperator()) {
            case TokenType::Minus:
                emitInstruction(Instruction::createUNM(dst, rhs));
                break;
            case TokenType::Not:
                emitInstruction(Instruction::createNOT(dst, rhs));
                break;
            default:
                throw LuaException("Unsupported unary operator.");
        }
        return dst;
    }
    
    int Compiler::compileBinary(const BinaryExpr* expr) {
        // Compile left and right expressions first
        int ra = compileExpr(expr->getLeft());
        int rb = compileExpr(expr->getRight());
        int dst = allocReg();
        
        switch (expr->getOperator()) {
            case TokenType::Plus:
                emitInstruction(Instruction::createADD(dst, ra, rb));
                break;
            case TokenType::Minus:
                emitInstruction(Instruction::createSUB(dst, ra, rb));
                break;
            case TokenType::Star:
                emitInstruction(Instruction::createMUL(dst, ra, rb));
                break;
            case TokenType::Slash:
                emitInstruction(Instruction::createDIV(dst, ra, rb));
                break;
            case TokenType::Percent:
                emitInstruction(Instruction::createMOD(dst, ra, rb));
                break;
            case TokenType::Equal:
                emitInstruction(Instruction::createEQ(dst, ra, rb));
                break;
            case TokenType::Less:
                emitInstruction(Instruction::createLT(dst, ra, rb));
                break;
            case TokenType::LessEqual:
                emitInstruction(Instruction::createLE(dst, ra, rb));
                break;
            default:
                throw LuaException("Unsupported binary operator.");
        }
        return dst;
    }
    
    int Compiler::compileCall(const CallExpr* expr) {
        // Compile the called function or variable
        int base = compileExpr(expr->getCallee());
        
        // Compile arguments
        int argCount = 0;
        for (const auto& arg : expr->getArguments()) {
            int reg = compileExpr(arg.get());
            // ensure arguments are contiguous; simplistic approach assumes they already are
            argCount++;
        }
        
        // Emit call instruction, specifying argument count
        emitInstruction(Instruction::createCALL(base, argCount + 1, 1));
        return base; // result in base
    }
    
    int Compiler::compileExpr(const Expr* expr) {
        if (expr == nullptr) return 0;
        
        switch (expr->getType()) {
            case ExprType::Literal:
                return compileLiteral(static_cast<const LiteralExpr*>(expr));
            case ExprType::Variable:
                return compileVariable(static_cast<const VariableExpr*>(expr));
            case ExprType::Unary:
                return compileUnary(static_cast<const UnaryExpr*>(expr));
            case ExprType::Binary:
                return compileBinary(static_cast<const BinaryExpr*>(expr));
            case ExprType::Call:
                return compileCall(static_cast<const CallExpr*>(expr));
            default:
                throw LuaException("Unsupported expression type.");
        }
        return 0; // unreachable
    }
    
    void Compiler::compileExprStmt(const ExprStmt* stmt) {
        int r = compileExpr(stmt->getExpression());
        // Result no longer needed
        nextReg = r; // reset reg pointer to allow reuse (simplistic)
    }
    
    void Compiler::compileLocalStmt(const LocalStmt* stmt) {
        // Allocate register slot for local variable
        int slot = allocReg();
        // Record variable to locals first for later endScope cleanup
        locals.emplace_back(stmt->getName(), scopeDepth, slot);

        if (stmt->getInitializer() != nullptr) {
            int r = compileExpr(stmt->getInitializer());
            emitInstruction(Instruction::createMOVE(slot, r));
            freeReg(); // Release temporary register r
        } else {
            emitInstruction(Instruction::createLOADNIL(slot));
        }
    }
    
    void Compiler::compileBlockStmt(const BlockStmt* stmt) {
        beginScope();
        
        // Compile each statement in the block
        for (const auto& statement : stmt->getStatements()) {
            compileStmt(statement.get());
        }
        
        endScope();
    }
    
    void Compiler::compileStmt(const Stmt* stmt) {
        if (stmt == nullptr) return;
        
        switch (stmt->getType()) {
            case StmtType::Expression:
                compileExprStmt(static_cast<const ExprStmt*>(stmt));
                break;
            case StmtType::Block:
                compileBlockStmt(static_cast<const BlockStmt*>(stmt));
                break;
            case StmtType::Local:
                compileLocalStmt(static_cast<const LocalStmt*>(stmt));
                break;
            case StmtType::ForIn:
                compileForInStmt(static_cast<const ForInStmt*>(stmt));
                break;
            case StmtType::RepeatUntil:
                compileRepeatUntilStmt(static_cast<const RepeatUntilStmt*>(stmt));
                break;
            default:
                throw LuaException("Unsupported statement type.");
        }
    }
    
    void Compiler::compileForInStmt(const ForInStmt* stmt) {
        // For-in loop compilation:
        // 1. Evaluate iterator expression list
        // 2. Call iterator function to get values
        // 3. Assign values to loop variables
        // 4. Compile loop body
        // 5. Jump back to iterator call
        
        beginScope();
        
        // Allocate registers for iterator function, state, and control variable
        int iterReg = allocReg();     // Iterator function
        int stateReg = allocReg();    // Iterator state
        int controlReg = allocReg();  // Control variable
        
        // Compile iterator expression list (e.g., next, table)
        const auto& iterators = stmt->getIterators();
        int exprReg = allocReg();
        
        if (iterators.size() == 1) {
            // Single expression case (e.g., pairs(table))
            exprReg = compileExpr(iterators[0].get());
            emitInstruction(Instruction::createCALL(exprReg, 1, 4)); // Call with 0 args, expect 3 results
        } else {
            // Multiple expressions case (e.g., next, table)
            for (size_t i = 0; i < iterators.size() && i < 3; ++i) {
                int tempReg = compileExpr(iterators[i].get());
                emitInstruction(Instruction::createMOVE(exprReg + static_cast<int>(i), tempReg));
                freeReg();
            }
        }
        
        // Move results to our allocated registers
        emitInstruction(Instruction::createMOVE(iterReg, exprReg));
        emitInstruction(Instruction::createMOVE(stateReg, exprReg + 1));
        emitInstruction(Instruction::createMOVE(controlReg, exprReg + 2));
        
        // Free the temporary expression register
        freeReg();
        
        // Loop start position
        int loopStart = static_cast<int>(code->size());
        
        // Call iterator function with state and control variable
        emitInstruction(Instruction::createMOVE(exprReg, iterReg));
        emitInstruction(Instruction::createMOVE(exprReg + 1, stateReg));
        emitInstruction(Instruction::createMOVE(exprReg + 2, controlReg));
        emitInstruction(Instruction::createCALL(exprReg, 3, static_cast<int>(stmt->getVariables().size()) + 1));
        
        // Check if first result is nil (end of iteration)
        int exitJump = emitJump(); // Will be patched to jump to loop end
        
        // Declare loop variables and assign values from iterator call
        for (size_t i = 0; i < stmt->getVariables().size(); ++i) {
            const auto& varName = stmt->getVariables()[i];
            int varSlot = allocReg();
            locals.emplace_back(varName, scopeDepth, varSlot);
            
            // Move result to variable slot
            if (i == 0) {
                // First variable gets the control value for next iteration
                emitInstruction(Instruction::createMOVE(varSlot, exprReg));
                emitInstruction(Instruction::createMOVE(controlReg, exprReg));
            } else {
                emitInstruction(Instruction::createMOVE(varSlot, exprReg + static_cast<int>(i)));
            }
        }
        
        // Compile loop body
        compileStmt(stmt->getBody());
        
        // Jump back to loop start
        int backJump = static_cast<int>(code->size()) - loopStart;
        emitInstruction(Instruction::createJMP(-backJump));
        
        // Patch exit jump
        patchJump(exitJump);
        
        endScope();
    }
    
    void Compiler::compileRepeatUntilStmt(const RepeatUntilStmt* stmt) {
        // Repeat-until loop compilation:
        // 1. Mark loop start position
        // 2. Compile loop body
        // 3. Evaluate until condition
        // 4. If condition is false, jump back to loop start
        
        beginScope();
        
        // Mark loop start position
        int loopStart = static_cast<int>(code->size());
        
        // Compile loop body
        compileStmt(stmt->getBody());
        
        // Evaluate until condition
        int conditionReg = compileExpr(stmt->getCondition());
        
        // In repeat-until, we continue looping while condition is false
        // So we need to jump back if condition is false (nil or false)
        // We can use a conditional jump that jumps when condition is true (to exit)
        // and fall through to jump back when condition is false
        
        // Emit a conditional jump that will be patched to exit the loop
        int exitJump = emitJump();
        
        // Jump back to loop start (this executes when condition is false)
        int backJump = static_cast<int>(code->size()) - loopStart + 1;
        emitInstruction(Instruction::createJMP(-backJump));
        
        // Patch the exit jump to point here (after the loop)
        patchJump(exitJump);
        
        // Free the condition register
        freeReg();
        
        endScope();
    }
    
    Ptr<Function> Compiler::compile(const Vec<UPtr<Stmt>>& statements) {
        try {
            // Compile each statement
            for (const auto& stmt : statements) {
                compileStmt(stmt.get());
            }
            
            // Add return instruction
            emitInstruction(Instruction::createRETURN(0, 0));
            
            // Create function object
            return Function::createLua(code, constants);
        } catch (const LuaException& e) {
            // Compilation error, return nullptr
            std::cerr << "Compilation error: " << e.what() << std::endl;
            return nullptr;
        }
    }
}
