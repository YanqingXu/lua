#include "statement_compiler.hpp"
#include "compiler.hpp"
#include "expression_compiler.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>

namespace Lua {
    StatementCompiler::StatementCompiler(Compiler* compiler) : compiler(compiler) {}
    
    void StatementCompiler::compileStmt(const Stmt* stmt) {
        if (!stmt) {
            throw LuaException("Null statement in compilation");
        }
        
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
            case StmtType::Assign:
                compileAssignmentStmt(static_cast<const AssignStmt*>(stmt));
                break;
            case StmtType::If:
                compileIfStmt(static_cast<const IfStmt*>(stmt));
                break;
            case StmtType::While:
                compileWhileStmt(static_cast<const WhileStmt*>(stmt));
                break;
            case StmtType::For:
                compileForStmt(static_cast<const ForStmt*>(stmt));
                break;
            case StmtType::ForIn:
                compileForInStmt(static_cast<const ForInStmt*>(stmt));
                break;
            case StmtType::RepeatUntil:
                compileRepeatUntilStmt(static_cast<const RepeatUntilStmt*>(stmt));
                break;
            case StmtType::Return:
                compileReturnStmt(static_cast<const ReturnStmt*>(stmt));
                break;
            case StmtType::Break:
                compileBreakStmt(static_cast<const BreakStmt*>(stmt));
                break;
            case StmtType::Function:
                compileFunctionStmt(static_cast<const FunctionStmt*>(stmt));
                break;
            default:
                throw LuaException("Unknown statement type in compilation");
        }
    }
    
    void StatementCompiler::compileExprStmt(const ExprStmt* stmt) {
        // Compile expression and discard result
        int exprReg = compiler->getExpressionCompiler()->compileExpr(stmt->getExpression());
        compiler->freeReg(); // Free the expression result register
    }
    
    void StatementCompiler::compileBlockStmt(const BlockStmt* stmt) {
        compiler->beginScope();
        
        for (const auto& statement : stmt->getStatements()) {
            compileStmt(statement.get());
        }
        
        compiler->endScope();
    }
    
    void StatementCompiler::compileLocalStmt(const LocalStmt* stmt) {
        const Str& name = stmt->getName();
        const Expr* initializer = stmt->getInitializer();
        
        // Declare local variable
        int varSlot = compiler->allocReg();
        compiler->addLocal(name, varSlot);
        
        if (initializer != nullptr) {
            // Compile initializer expression
            int initReg = compiler->getExpressionCompiler()->compileExpr(initializer);
            // Move initializer value to variable slot
            compiler->emitInstruction(Instruction::createMOVE(varSlot, initReg));
            // Free initializer register
            compiler->freeReg();
        } else {
            // Initialize with nil
            compiler->emitInstruction(Instruction::createLOADNIL(varSlot));
        }
    }
    
    void StatementCompiler::compileAssignmentStmt(const AssignStmt* stmt) {
        const Expr* target = stmt->getTarget();
        const Expr* value = stmt->getValue();
        
        // Compile value expression
        int valueReg = compiler->getExpressionCompiler()->compileExpr(value);
        
        // Assign to target
        if (target->getType() == ExprType::Variable) {
                const auto* varExpr = static_cast<const VariableExpr*>(target);
                const Str& name = varExpr->getName();
                
            // Try to find local variable
            int localSlot = compiler->resolveLocal(name);
            if (localSlot != -1) {
                compiler->emitInstruction(Instruction::createMOVE(localSlot, valueReg));
            } else {
                // Global variable assignment
                int nameIdx = compiler->addConstant(Value(name));
                compiler->emitInstruction(Instruction::createSETGLOBAL(valueReg, nameIdx));
            }
        }
        // TODO: Handle table assignments (IndexAccessExpr, MemberAccessExpr)
        
        // Free value register
        compiler->freeReg();
    }
    
    void StatementCompiler::compileIfStmt(const IfStmt* stmt) {
        // Compile condition
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(stmt->getCondition());
        
        // Test condition and skip next instruction if false
        compiler->emitInstruction(Instruction::createTEST(conditionReg, 0));
        compiler->freeReg(); // Free condition register
        
        // Jump to else/end if condition is false
        int jumpToElse = compiler->emitJump();
        
        // Compile then branch
        compileStmt(stmt->getThenBranch());
        
        int jumpToEnd = -1;
        if (stmt->getElseBranch()) {
            // Jump over else branch
            jumpToEnd = compiler->emitJump();
        }
        
        // Patch jump to else
        compiler->patchJump(jumpToElse);
        
        // Compile else branch if present
        if (stmt->getElseBranch()) {
            compileStmt(stmt->getElseBranch());
            if (jumpToEnd != -1) {
                compiler->patchJump(jumpToEnd);
            }
        }
    }
    
    void StatementCompiler::compileWhileStmt(const WhileStmt* stmt) {
        int loopStart = static_cast<int>(compiler->getCodeSize());
        
        // Compile condition
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(stmt->getCondition());
        
        // Jump to end if condition is false
        int exitJump = compiler->emitJump();
        compiler->freeReg(); // Free condition register
        
        // Compile loop body
        compiler->beginScope();
        compileStmt(stmt->getBody());
        compiler->endScope();
        
        // Jump back to loop start
        int backJump = static_cast<int>(compiler->getCodeSize()) - loopStart;
        compiler->emitInstruction(Instruction::createJMP(-backJump));
        
        // Patch exit jump
        compiler->patchJump(exitJump);
    }
    
    void StatementCompiler::compileForStmt(const ForStmt* stmt) {
        compiler->beginScope();
        
        // Initialize loop variable
        const Str& varName = stmt->getVariable();
        int varSlot = compiler->allocReg();
        compiler->addLocal(varName, varSlot);
        
        // Compile initial value
        int initReg = compiler->getExpressionCompiler()->compileExpr(stmt->getStart());
        compiler->emitInstruction(Instruction::createMOVE(varSlot, initReg));
        compiler->freeReg();
        
        // Compile limit and step
        int limitReg = compiler->getExpressionCompiler()->compileExpr(stmt->getEnd());
        int stepReg = -1;
        if (stmt->getStep()) {
            stepReg = compiler->getExpressionCompiler()->compileExpr(stmt->getStep());
        } else {
            // Default step is 1
            stepReg = compiler->allocReg();
            int oneIdx = compiler->addConstant(Value(1.0));
            compiler->emitInstruction(Instruction::createLOADK(stepReg, oneIdx));
        }
        
        int loopStart = static_cast<int>(compiler->getCodeSize());
        
        // Check loop condition (var <= limit for positive step, var >= limit for negative step)
        int condReg = compiler->allocReg();
        compiler->emitInstruction(Instruction::createLE(condReg, varSlot, limitReg));
        
        // Jump to end if condition is false
        int exitJump = compiler->emitJump();
        compiler->freeReg(); // Free condition register
        
        // Compile loop body
        compileStmt(stmt->getBody());
        
        // Increment loop variable
        compiler->emitInstruction(Instruction::createADD(varSlot, varSlot, stepReg));
        
        // Jump back to loop start
        int backJump = static_cast<int>(compiler->getCodeSize()) - loopStart;
        compiler->emitInstruction(Instruction::createJMP(-backJump));
        
        // Patch exit jump
        compiler->patchJump(exitJump);
        
        // Free limit and step registers
        compiler->freeReg(); // step
        compiler->freeReg(); // limit
        
        compiler->endScope();
    }
    
    void StatementCompiler::compileForInStmt(const ForInStmt* stmt) {
        compiler->beginScope();
        
        // Compile iterator expressions
        const auto& iterators = stmt->getIterators();
        Vec<int> iterRegs;
        for (const auto& iter : iterators) {
            int reg = compiler->getExpressionCompiler()->compileExpr(iter.get());
            iterRegs.push_back(reg);
        }
        
        // Set up control variables
        int controlReg = compiler->allocReg();
        if (!iterRegs.empty()) {
            compiler->emitInstruction(Instruction::createMOVE(controlReg, iterRegs[0]));
        }
        
        int loopStart = static_cast<int>(compiler->getCodeSize());
        
        // Call iterator function
        int exprReg = compiler->allocReg();
        compiler->emitInstruction(Instruction::createCALL(controlReg, 0, 1));
        
        // Check if iteration is done (result is nil)
        int exitJump = compiler->emitJump();
        
        // Declare loop variables
        const auto& variables = stmt->getVariables();
        for (size_t i = 0; i < variables.size(); ++i) {
            const Str& varName = variables[i];
            int varSlot = compiler->allocReg();
            compiler->addLocal(varName, varSlot);
            
            if (i == 0) {
                compiler->emitInstruction(Instruction::createMOVE(varSlot, exprReg));
                compiler->emitInstruction(Instruction::createMOVE(controlReg, exprReg));
            } else {
                compiler->emitInstruction(Instruction::createMOVE(varSlot, exprReg + static_cast<int>(i)));
            }
        }
        
        // Compile loop body
        compileStmt(stmt->getBody());
        
        // Jump back to loop start
        int backJump = static_cast<int>(compiler->getCodeSize()) - loopStart;
        compiler->emitInstruction(Instruction::createJMP(-backJump));
        
        // Patch exit jump
        compiler->patchJump(exitJump);
        
        // Free registers
        for (size_t i = 0; i < iterRegs.size(); ++i) {
            compiler->freeReg();
        }
        
        compiler->endScope();
    }
    
    void StatementCompiler::compileRepeatUntilStmt(const RepeatUntilStmt* stmt) {
        compiler->beginScope();
        
        int loopStart = static_cast<int>(compiler->getCodeSize());
        
        // Compile loop body
        compileStmt(stmt->getBody());
        
        // Evaluate until condition
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(stmt->getCondition());
        
        // Jump to exit if condition is true
        int exitJump = compiler->emitJump();
        
        // Jump back to loop start (when condition is false)
        int backJump = static_cast<int>(compiler->getCodeSize()) - loopStart + 1;
        compiler->emitInstruction(Instruction::createJMP(-backJump));
        
        // Patch the exit jump
        compiler->patchJump(exitJump);
        
        // Free condition register
        compiler->freeReg();
        
        compiler->endScope();
    }
    
    void StatementCompiler::compileReturnStmt(const ReturnStmt* stmt) {
        const auto* value = stmt->getValue();
        
        if (value == nullptr) {
            // Return nil
            compiler->emitInstruction(Instruction::createRETURN(0, 0));
        } else {
            // Compile return value
            int reg = compiler->getExpressionCompiler()->compileExpr(value);
            
            // Emit return instruction
            compiler->emitInstruction(Instruction::createRETURN(reg, 1));
            
            // Free value register
            compiler->freeReg();
        }
    }
    
    void StatementCompiler::compileBreakStmt(const BreakStmt* stmt) {
        // Add break jump to be patched later
        int breakJump = compiler->emitJump();
        compiler->addBreakJump(breakJump);
    }
    
    void StatementCompiler::compileFunctionStmt(const FunctionStmt* stmt) {
        compiler->beginScope();
        
        // TODO: Implement proper function compilation
        // For now, compile the body as a block
        compileStmt(stmt->getBody());
        
        compiler->endScope();
    }
    
    void StatementCompiler::compileConditionalJump(const Expr* condition, bool jumpIfTrue, int& jumpAddr) {
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(condition);
        
        if (jumpIfTrue) {
            jumpAddr = compiler->emitJump();
        } else {
            jumpAddr = compiler->emitJump();
        }
        
        compiler->freeReg(); // Free condition register
    }
    
    void StatementCompiler::compileLoopBody(const Stmt* body, int loopStart) {
        compiler->beginScope();
        compileStmt(body);
        compiler->endScope();
        
        // Jump back to loop start
        int backJump = static_cast<int>(compiler->getCodeSize()) - loopStart;
        compiler->emitInstruction(Instruction::createJMP(-backJump));
    }
    
    void StatementCompiler::handleBreakStatements(int loopEnd) {
        // Patch all break jumps to point to loop end
        compiler->patchBreakJumps(loopEnd);
    }
}