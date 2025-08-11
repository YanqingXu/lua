#include "statement_compiler.hpp"
#include "compiler.hpp"
#include "expression_compiler.hpp"
#include "upvalue_analyzer.hpp"
#include "symbol_table.hpp"
#include "../common/opcodes.hpp"
#include "../vm/value.hpp"
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
            case StmtType::MultiLocal:
                compileMultiLocalStmt(static_cast<const MultiLocalStmt*>(stmt));
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
            case StmtType::Do:
                compileDoStmt(static_cast<const DoStmt*>(stmt));
                break;
            default:
                throw LuaException("Unknown statement type in compilation");
        }
    }
    
    // Helper methods implementation
    void StatementCompiler::compileConditionalJump(const Expr* condition, bool jumpIfTrue, int& jumpAddr) {
        // Compile condition expression
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(condition);
        
        // Create TEST instruction based on jump direction
        if (jumpIfTrue) {
            // Jump if condition is true (invert test)
            compiler->emitInstruction(Instruction::createTEST(conditionReg, 1));
        } else {
            // Jump if condition is false/nil
            compiler->emitInstruction(Instruction::createTEST(conditionReg, 0));
        }
        
        compiler->freeReg(); // Free condition register
        
        // Create jump placeholder
        jumpAddr = compiler->emitJump();
    }
    
    void StatementCompiler::compileNestedCondition(const Expr* condition, int& jumpToElse, int& jumpToEnd) {
        // Handle complex nested conditions with proper jump management
        if (condition->getType() == ExprType::Binary) {
            const auto* binExpr = static_cast<const BinaryExpr*>(condition);
            TokenType op = binExpr->getOperator();
            
            if (op == TokenType::And || op == TokenType::Or) {
                // For logical operators, let expression compiler handle short-circuiting
                int conditionReg = compiler->getExpressionCompiler()->compileExpr(condition);
                
                // Test final result
                compiler->emitInstruction(Instruction::createTEST(conditionReg, 0));
                compiler->freeReg();
                
                jumpToElse = compiler->emitJump();
                jumpToEnd = -1; // Will be set later if needed
                return;
            }
        }
        
        // For simple conditions, use standard compilation
        compileConditionalJump(condition, false, jumpToElse);
        jumpToEnd = -1;
    }
    
    void StatementCompiler::patchConditionalJumps(int jumpToElse, int jumpToEnd, bool hasElse) {
        // Patch the jump to else/end
        if (jumpToElse != -1) {
            compiler->patchJump(jumpToElse);
        }
        
        // Patch the jump to end if there was an else branch
        if (hasElse && jumpToEnd != -1) {
            compiler->patchJump(jumpToEnd);
        }
    }
    
    void StatementCompiler::compileLoopBody(const Stmt* body, int loopStart) {
        // Begin new scope for loop body
        compiler->beginScope();
        
        // Compile loop body
        compileStmt(body);
        
        // End loop scope
        compiler->endScope();
    }
    
    void StatementCompiler::handleBreakStatements(int loopEnd) {
        // Patch all break statements to jump to loop end
        compiler->patchBreakJumps(loopEnd);
    }
    
    void StatementCompiler::compileExprStmt(const ExprStmt* stmt) {
        // Save current register state
        int oldStackTop = compiler->getRegisterManager().getStackTop();

        // Compile expression and discard result
        int exprReg = compiler->getExpressionCompiler()->compileExpr(stmt->getExpression());

        // Reset register stack to previous state to free all temporary registers
        // This ensures function calls don't leave dangling register allocations
        while (compiler->getRegisterManager().getStackTop() > oldStackTop) {
            compiler->freeReg();
        }
    }
    
    void StatementCompiler::compileBlockStmt(const BlockStmt* stmt) {
        // 检测是否是多重局部变量声明的BlockStmt
        bool isMultiLocalBlock = true;
        for (const auto& statement : stmt->getStatements()) {
            if (statement->getType() != StmtType::Local) {
                isMultiLocalBlock = false;
                break;
            }
        }

        // 如果是多重局部变量声明，不创建新的作用域
        if (!isMultiLocalBlock) {
            compiler->beginScope();
        }

        for (const auto& statement : stmt->getStatements()) {
            compileStmt(statement.get());
        }

        if (!isMultiLocalBlock) {
            compiler->endScope();
        }
    }
    
    void StatementCompiler::compileLocalStmt(const LocalStmt* stmt) {
        const Str& name = stmt->getName();
        const Expr* initializer = stmt->getInitializer();

        // Declare local variable using unified scope management
        int varSlot = compiler->defineLocal(name);

        if (initializer != nullptr) {
            // Compile initializer expression

            // TODO: 优化 - 直接将初始化表达式编译到局部变量槽位
            // 当前实现：先编译到临时寄存器，再移动（保持兼容性）
            int initReg = compiler->getExpressionCompiler()->compileExpr(initializer);

            if (initReg != varSlot) {
                // 只有当初始化寄存器与变量槽位不同时才需要移动
                compiler->emitInstruction(Instruction::createMOVE(varSlot, initReg));
                // Free initializer register
                compiler->freeReg();
            }
        } else {
            // Initialize with nil
            compiler->emitInstruction(Instruction::createLOADNIL(varSlot));
        }
    }

    void StatementCompiler::compileMultiLocalStmt(const MultiLocalStmt* stmt) {
        const Vec<Str>& names = stmt->getNames();
        const Vec<UPtr<Expr>>& initializers = stmt->getInitializers();

        // Declare all local variables first
        Vec<int> varSlots;
        for (const Str& name : names) {
            int varSlot = compiler->defineLocal(name);
            varSlots.push_back(varSlot);
        }

        if (initializers.empty()) {
            // No initializers - initialize all variables with nil
            for (int varSlot : varSlots) {
                compiler->emitInstruction(Instruction::createLOADNIL(varSlot));
            }
        } else if (initializers.size() == 1) {
            // Single initializer - could be a function call returning multiple values
            const Expr* init = initializers[0].get();

            if (init->getType() == ExprType::Call) {
                // Function call - check if it's a method call
                const CallExpr* callExpr = static_cast<const CallExpr*>(init);

                // COLON SYNTAX DECISIVE FIX: For method calls, use compileCall instead of compileCallWithMultiReturn
                if (callExpr->getIsMethodCall()) {
                    // For method calls, use the already working compileCall
                    int resultReg = compiler->getExpressionCompiler()->compileCall(callExpr);

                    // Move result to first variable slot
                    if (resultReg != varSlots[0]) {
                        compiler->emitInstruction(Instruction::createMOVE(varSlots[0], resultReg));
                    }

                    // Initialize remaining variables to nil
                    for (size_t i = 1; i < varSlots.size(); ++i) {
                        compiler->emitInstruction(Instruction::createLOADNIL(varSlots[i]));
                    }
                } else {
                    // Regular function call - use multi-return support
                    int expectedReturns = static_cast<int>(names.size());
                    int startReg = varSlots[0]; // Start from first variable slot

                    // Compile function call expression with multi-return support
                    compiler->getExpressionCompiler()->compileCallWithMultiReturn(callExpr, startReg, expectedReturns);
                }

                // The call instruction will place results in consecutive registers starting from startReg
                // If we get fewer returns than expected, remaining slots will be nil
                // If we get more returns than expected, excess returns are discarded
            } else {
                // Single non-call expression - assign to first variable, rest get nil
                int initReg = compiler->getExpressionCompiler()->compileExpr(init);

                // Move to first variable slot if needed
                if (initReg != varSlots[0]) {
                    compiler->emitInstruction(Instruction::createMOVE(varSlots[0], initReg));
                    compiler->freeReg();
                }

                // Initialize remaining variables with nil
                for (size_t i = 1; i < varSlots.size(); ++i) {
                    compiler->emitInstruction(Instruction::createLOADNIL(varSlots[i]));
                }
            }
        } else {
            // Multiple initializers - assign one-to-one, remaining variables get nil
            size_t minSize = std::min(names.size(), initializers.size());

            // Compile and assign each initializer
            for (size_t i = 0; i < minSize; ++i) {
                int initReg = compiler->getExpressionCompiler()->compileExpr(initializers[i].get());

                if (initReg != varSlots[i]) {
                    compiler->emitInstruction(Instruction::createMOVE(varSlots[i], initReg));
                    compiler->freeReg();
                }
            }

            // Initialize remaining variables with nil
            for (size_t i = minSize; i < varSlots.size(); ++i) {
                compiler->emitInstruction(Instruction::createLOADNIL(varSlots[i]));
            }
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

            // Use unified variable resolution
            auto varInfo = compiler->resolveVariable(name);

            switch (varInfo.type) {
                case Compiler::VariableType::Local:
                    compiler->emitInstruction(Instruction::createMOVE(varInfo.index, valueReg));
                    break;

                case Compiler::VariableType::Upvalue:
                    compiler->emitInstruction(Instruction::createSETUPVAL(valueReg, varInfo.index));
                    break;

                case Compiler::VariableType::Global:
                    compiler->emitInstruction(Instruction::createSETGLOBAL(valueReg, varInfo.index));
                    break;

                default:
                    throw LuaException("Unknown variable type for assignment: " + name);
            }
        }
        else if (target->getType() == ExprType::Index) {
            // Table index assignment: table[key] = value
            const auto* indexExpr = static_cast<const IndexExpr*>(target);
            int tableReg = compiler->getExpressionCompiler()->compileExpr(indexExpr->getObject());
            int keyReg = compiler->getExpressionCompiler()->compileExpr(indexExpr->getIndex());

            // Use basic table assignment first, metamethods will be handled at runtime if needed
            compiler->emitInstruction(Instruction::createSETTABLE(tableReg, keyReg, valueReg));

            // Free registers
            compiler->freeReg(); // Free key register
            compiler->freeReg(); // Free table register
        }
        else if (target->getType() == ExprType::Member) {
            // Table member assignment: table.member = value
            const auto* memberExpr = static_cast<const MemberExpr*>(target);
            int tableReg = compiler->getExpressionCompiler()->compileExpr(memberExpr->getObject());

            // Convert member name to string constant
            int nameIdx = compiler->addConstant(Value(memberExpr->getName()));

            // Use basic table assignment first, metamethods will be handled at runtime if needed
            // Use RK encoding for constant index (Lua 5.1 standard with 8-bit operands)
            if (nameIdx <= MAXINDEXRK_8) {
                u8 keyParam = RK(static_cast<u8>(nameIdx));
                //std::cout << "Member assignment: nameIdx=" << nameIdx << ", keyParam=" << (int)keyParam << " (RK encoded)" << std::endl;
                compiler->emitInstruction(Instruction::createSETTABLE(tableReg, keyParam, valueReg));
            } else {
                // Fallback to register approach for large constant indices
                //std::cout << "Member assignment: nameIdx=" << nameIdx << " too large, using register" << std::endl;
                int keyReg = compiler->allocReg();
                compiler->emitInstruction(Instruction::createLOADK(keyReg, nameIdx));
                compiler->emitInstruction(Instruction::createSETTABLE(tableReg, keyReg, valueReg));
                compiler->freeReg(); // Free key register
            }

            // Free table register
            compiler->freeReg();
        }
        else {
            throw LuaException("Invalid assignment target");
        }
        
        // Free value register
        compiler->freeReg();
    }
    
    void StatementCompiler::compileIfStmt(const IfStmt* stmt) {
        // Compile condition expression
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(stmt->getCondition());

        // Test condition: if true, skip the jump (continue to then branch)
        // if false, execute the jump (go to else/end)
        compiler->emitInstruction(Instruction::createTEST(conditionReg, 1));
        compiler->freeReg(); // Free condition register

        // Create jump placeholder for false condition (jump to else/end)
        int jumpToElse = compiler->emitJump();
        
        // Compile then branch (without scope management to avoid register conflicts)
        compileStmt(stmt->getThenBranch());
        
        int jumpToEnd = -1;
        if (stmt->getElseBranch()) {
            // If there's an else branch, we need to jump over it after then branch
            jumpToEnd = compiler->emitJump();
        }
        
        // Patch the jump to else/end - this is where false condition lands
        compiler->patchJump(jumpToElse);
        
        // Compile else branch if present (without scope management to avoid register conflicts)
        if (stmt->getElseBranch()) {
            compileStmt(stmt->getElseBranch());
            
            // Patch the jump to end after then branch
            if (jumpToEnd != -1) {
                compiler->patchJump(jumpToEnd);
            }
        }
    }
    
    void StatementCompiler::compileWhileStmt(const WhileStmt* stmt) {
        int loopStart = static_cast<int>(compiler->getCodeSize());

        // Compile condition (same pattern as for loop)
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(stmt->getCondition());

        // Test condition: if true, skip the jump to end (continue with loop body)
        compiler->emitInstruction(Instruction::createTEST(conditionReg, 1));
        int exitJump = compiler->emitJump();
        compiler->freeReg(); // Free condition register

        // Compile loop body
        compiler->beginScope();
        compileStmt(stmt->getBody());
        compiler->endScope();

        // Jump back to loop start
        int currentPos = static_cast<int>(compiler->getCodeSize());
        int backJump = currentPos - loopStart + 1;  // +1 because JMP instruction itself advances PC
        compiler->emitInstruction(Instruction::createJMP(-backJump));

        // Patch exit jump
        compiler->patchJump(exitJump);

        // Handle break statements - patch them to jump to loop end
        handleBreakStatements(static_cast<int>(compiler->getCodeSize()));
    }
    
    void StatementCompiler::compileForStmt(const ForStmt* stmt) {
        compiler->beginScope();

        // Initialize loop variable
        const Str& varName = stmt->getVariable();
        int varSlot = compiler->defineLocal(varName);



        // Compile initial value
        int initReg = compiler->getExpressionCompiler()->compileExpr(stmt->getStart());
        compiler->emitInstruction(Instruction::createMOVE(varSlot, initReg));
        compiler->freeReg();

        // Compile limit and step as local variables to avoid register conflicts
        int limitReg = compiler->defineLocal("__limit");
        int limitExprReg = compiler->getExpressionCompiler()->compileExpr(stmt->getEnd());
        compiler->emitInstruction(Instruction::createMOVE(limitReg, limitExprReg));
        compiler->freeReg();

        int stepReg = compiler->defineLocal("__step");
        if (stmt->getStep()) {
            int stepExprReg = compiler->getExpressionCompiler()->compileExpr(stmt->getStep());
            compiler->emitInstruction(Instruction::createMOVE(stepReg, stepExprReg));
            compiler->freeReg();
        } else {
            // Default step is 1
            int oneIdx = compiler->addConstant(Value(1.0));
            compiler->emitInstruction(Instruction::createLOADK(stepReg, oneIdx));
        }

        int loopStart = static_cast<int>(compiler->getCodeSize());

        // Check loop condition based on step value
        int condReg = compiler->allocReg();

        // Try to determine step sign at compile time
        bool isStepNegative = false;
        if (stmt->getStep() && compiler->getExpressionCompiler()->isConstantExpression(stmt->getStep())) {
            Value stepValue = compiler->getExpressionCompiler()->getConstantValue(stmt->getStep());
            if (stepValue.isNumber() && stepValue.asNumber() < 0) {
                isStepNegative = true;
            }
        }

        if (isStepNegative) {
            // For negative step: var >= limit (equivalent to limit <= var)
            compiler->emitInstruction(Instruction::createLE(condReg, limitReg, varSlot));
        } else {
            // For positive step: var <= limit
            compiler->emitInstruction(Instruction::createLE(condReg, varSlot, limitReg));
        }

        // Jump to end if condition is false
        compiler->emitInstruction(Instruction::createTEST(condReg, 1));
        int exitJump = compiler->emitJump();

        compiler->freeReg(); // condReg

        // Compile loop body with independent scope for each iteration
        // This ensures that local variables in the loop body don't conflict across iterations
        compiler->beginScope();
        compileStmt(stmt->getBody());
        compiler->endScope();

        // Increment loop variable
        compiler->emitInstruction(Instruction::createADD(varSlot, varSlot, stepReg));

        // Jump back to loop start
        int currentPos = static_cast<int>(compiler->getCodeSize());
        int backJump = currentPos - loopStart + 1;  // +1 because JMP instruction itself advances PC
        compiler->emitInstruction(Instruction::createJMP(-backJump));

        // Patch exit jump
        compiler->patchJump(exitJump);

        // Handle break statements - patch them to jump to loop end
        handleBreakStatements(static_cast<int>(compiler->getCodeSize()));

        // limit and step are now local variables, no need to free them manually

        compiler->endScope();
    }
    
    void StatementCompiler::compileForInStmt(const ForInStmt* stmt) {
        compiler->beginScope();

        // Simplified for-in implementation that works with our current pairs() function
        const auto& iterators = stmt->getIterators();
        if (iterators.empty()) {
            throw LuaException("for-in statement requires at least one iterator expression");
        }

        // Compile the iterator expression and use compileCallWithMultiReturn for proper handling
        const auto* callExpr = dynamic_cast<const CallExpr*>(iterators[0].get());
        if (!callExpr) {
            throw LuaException("for-in statement requires a function call as iterator");
        }

        // Allocate individual registers to avoid conflicts with method parameters
        int iteratorReg = compiler->allocReg();  // Iterator function (permanent)
        int stateReg = compiler->allocReg();     // State table (permanent)
        int keyReg = compiler->allocReg();       // Current key (updated each iteration)

        // Allocate a block of 3 consecutive registers for the call
        // This ensures callReg+1 and callReg+2 are available for arguments
        // and callReg and callReg+1 will receive return values
        int callReg = compiler->allocReg();      // Call function + return values
        compiler->allocReg();                    // callReg + 1 (arg1/return2)
        compiler->allocReg();                    // callReg + 2 (arg2)

        // Registers allocated for for-in loop

        // Compile the iterator call (e.g., pairs(table)) to get iterator, state, initial_key
        compiler->getExpressionCompiler()->compileCallWithMultiReturn(callExpr, iteratorReg, 3);

        // The iterator call returns: [iteratorReg] = iterator_func, [iteratorReg+1] = state, [iteratorReg+2] = initial_key
        // Move to dedicated registers
        compiler->emitInstruction(Instruction::createMOVE(stateReg, iteratorReg + 1));
        compiler->emitInstruction(Instruction::createMOVE(keyReg, iteratorReg + 2));

        int loopStart = static_cast<int>(compiler->getCodeSize());

        // Set up iterator function call: iterator(state, key)
        compiler->emitInstruction(Instruction::createMOVE(callReg, iteratorReg));        // function
        compiler->emitInstruction(Instruction::createMOVE(callReg + 1, stateReg));      // arg1: state
        compiler->emitInstruction(Instruction::createMOVE(callReg + 2, keyReg));        // arg2: key

        // Call iterator function with 2 arguments, expect 2 returns
        compiler->emitInstruction(Instruction::createCALL_MM(callReg, 3, 3));

        // Return values are now in callReg (key) and callReg+1 (value)
        // No need to move them since we'll use them directly

        // Check if iteration is done (first return value is nil)
        compiler->emitInstruction(Instruction::createTEST(callReg, 1));  // Test if key is truthy
        int exitJump = compiler->emitJump();  // Jump to exit if nil

        // CRITICAL: Save all critical values BEFORE declaring loop variables and executing loop body
        // The loop body may use registers that conflict with our for-in state

        // Allocate safe backup registers for critical state
        int backupIteratorReg = compiler->allocReg();
        int backupStateReg = compiler->allocReg();

        // Save critical state to backup registers
        compiler->emitInstruction(Instruction::createMOVE(backupIteratorReg, iteratorReg));
        compiler->emitInstruction(Instruction::createMOVE(backupStateReg, stateReg));

        // Update key register for next iteration
        compiler->emitInstruction(Instruction::createMOVE(keyReg, callReg));

        // Declare loop variables and assign return values
        const auto& variables = stmt->getVariables();
        for (size_t i = 0; i < variables.size() && i < 2; ++i) {  // Max 2 variables (key, value)
            const Str& varName = variables[i];
            int varSlot = compiler->defineLocal(varName);

            // Move return value to variable slot
            if (i == 0) {
                compiler->emitInstruction(Instruction::createMOVE(varSlot, callReg));      // key
            } else {
                compiler->emitInstruction(Instruction::createMOVE(varSlot, callReg + 1));  // value
            }
        }

        // Compile loop body
        compileStmt(stmt->getBody());

        // CRITICAL: Restore critical state from backup registers after loop body
        compiler->emitInstruction(Instruction::createMOVE(iteratorReg, backupIteratorReg));
        compiler->emitInstruction(Instruction::createMOVE(stateReg, backupStateReg));

        // Free backup registers
        compiler->freeReg();  // backupStateReg
        compiler->freeReg();  // backupIteratorReg

        // Jump back to loop start
        int currentPos = static_cast<int>(compiler->getCodeSize());
        int backJump = currentPos - loopStart + 1;
        compiler->emitInstruction(Instruction::createJMP(-backJump));

        // Patch exit jump
        compiler->patchJump(exitJump);

        // Free all allocated registers in reverse order
        compiler->freeReg();  // callReg + 2
        compiler->freeReg();  // callReg + 1
        compiler->freeReg();  // callReg
        compiler->freeReg();  // keyReg
        compiler->freeReg();  // stateReg
        compiler->freeReg();  // iteratorReg

        compiler->endScope();
    }
    
    void StatementCompiler::compileRepeatUntilStmt(const RepeatUntilStmt* stmt) {
        compiler->beginScope();

        int loopStart = static_cast<int>(compiler->getCodeSize());

        // Compile loop body
        compileStmt(stmt->getBody());

        // Compile until condition
        int conditionReg = compiler->getExpressionCompiler()->compileExpr(stmt->getCondition());

        // Test condition: if false, skip the jump to exit (continue looping)
        compiler->emitInstruction(Instruction::createTEST(conditionReg, 0));
        int exitJump = compiler->emitJump();
        compiler->freeReg(); // Free condition register

        // Jump back to loop start (when condition is false)
        int currentPos = static_cast<int>(compiler->getCodeSize());
        int backJump = currentPos - loopStart + 1;  // +1 because JMP instruction itself advances PC
        compiler->emitInstruction(Instruction::createJMP(-backJump));

        // Patch the exit jump
        compiler->patchJump(exitJump);

        compiler->endScope();
    }
    
    void StatementCompiler::compileReturnStmt(const ReturnStmt* stmt) {
        const auto& values = stmt->getValues();
        
        if (values.empty()) {
            // Return nil (no values)
            compiler->emitInstruction(Instruction::createRETURN(0, 0));
        } else if (values.size() == 1) {
            // Check if the single return value is a vararg expression
            const auto* varargExpr = dynamic_cast<const VarargExpr*>(values[0].get());
            if (varargExpr) {
                // Special case: return ... should return all varargs
#if DEBUG_COMPILER
                DEBUG_PRINT("compileReturnStmt: detected return ... - generating VARARG and RETURN with B=0");
#endif
                // Generate VARARG instruction to get all varargs starting from register 0
                compiler->emitInstruction(Instruction::createVARARG(0, 0));

                // Emit return instruction with B=0 to return all values from register 0 to top
                compiler->emitInstruction(Instruction::createRETURN(0, 0));
            } else {
                // Single return value (backward compatibility)
                int reg = compiler->getExpressionCompiler()->compileExpr(values[0].get());

                // Emit return instruction (B=2 means return 1 value)
                // A parameter should be reg because VM will read from register a+1
                // If reg is compiler register index, VM register is reg+1, so a should be reg
                compiler->emitInstruction(Instruction::createRETURN(reg, 2));

                // Don't free value register - return statement terminates function execution
                // compiler->freeReg();
            }
        } else {
            // Multiple return values
            Vec<int> valueRegs;
            
            // Compile all return expressions
            for (const auto& value : values) {
                int reg = compiler->getExpressionCompiler()->compileExpr(value.get());
                valueRegs.push_back(reg);
            }
            
            // For multiple return values, we need to ensure they are in consecutive registers
            // starting from the first allocated register
            int startReg = valueRegs.empty() ? 0 : valueRegs[0];
            
            // Move values to consecutive registers if needed
            for (size_t i = 1; i < valueRegs.size(); ++i) {
                if (valueRegs[i] != startReg + static_cast<int>(i)) {
                    compiler->emitInstruction(Instruction::createMOVE(startReg + static_cast<int>(i), valueRegs[i]));
                }
            }
            
            // Emit return instruction (B = number of values + 1)
            // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
            compiler->emitInstruction(Instruction::createRETURN(startReg, static_cast<u8>(values.size() + 1)));
            
            // Free all value registers
            for (size_t i = 0; i < valueRegs.size(); ++i) {
                compiler->freeReg();
            }
        }
    }
    
    void StatementCompiler::compileBreakStmt(const BreakStmt* stmt) {
        // Add break jump to be patched later
        int breakJump = compiler->emitJump();
        compiler->addBreakJump(breakJump);
    }
    
    void StatementCompiler::compileFunctionStmt(const FunctionStmt* stmt) {


        // Check function nesting depth before proceeding
        compiler->enterFunctionScope();

        // Create compilation context for the nested function
        auto childContext = compiler->createChildContext();

        // Create a new compiler instance for the function body with parent context
        Compiler functionCompiler(childContext);

        // Analyze upvalues using the parent compiler's scope manager
        // This allows the analyzer to see variables from outer scopes
        UpvalueAnalyzer analyzer(compiler->getScopeManager());

        // Analyze the function body to find upvalues
        analyzer.analyzeFunction(stmt);
        const auto& upvalues = analyzer.getUpvalues();

        // Store upvalues in the function compiler
        for (const auto& upvalue : upvalues) {
            functionCompiler.addUpvalue(upvalue.name, upvalue.isLocal, upvalue.stackIndex);
        }
        
        // Enter function scope and define parameters
        functionCompiler.beginScope();

        // Lua 5.1官方调用约定：寄存器0是函数，参数从寄存器1开始
        // 先分配寄存器0给函数本身（虽然不会直接使用）
        functionCompiler.defineLocal("function");  // 寄存器0保留给函数

        // 然后分配参数寄存器（从寄存器1开始）
        for (const auto& param : stmt->getParameters()) {
            functionCompiler.defineLocal(param);  // 寄存器1, 2, 3...
        }
        
        // Compile function body
        functionCompiler.getStatementCompiler()->compileStmt(stmt->getBody());
        
        // Add implicit return if needed
        if (functionCompiler.getCodeSize() == 0 || 
            functionCompiler.getCode()->back().getOpCode() != OpCode::RETURN) {
            functionCompiler.emitInstruction(Instruction::createRETURN(0, 0));
        }
        
        // Get local count BEFORE ending scope (which resets the count)
        int paramCount = static_cast<int>(stmt->getParameters().size());
        int localCount = functionCompiler.getRegisterManager().getLocalCount();
        int upvalueCount = static_cast<int>(upvalues.size());

        functionCompiler.endScope();

        // Create function prototype
        auto functionCode = std::make_shared<Vec<Instruction>>(*functionCompiler.getCode());



        auto functionProto = Function::createLua(
            functionCode,
            functionCompiler.getConstants(),
            functionCompiler.getPrototypes(),
            static_cast<u8>(paramCount),
            static_cast<u8>(localCount),
            static_cast<u8>(upvalueCount),
            stmt->getIsVariadic()  // 修复：传递可变参数标志
        );
        
        // Add prototype to current compiler
        int prototypeIndex = compiler->addPrototype(functionProto);
        
        // Allocate register for the closure
        int closureReg = compiler->allocReg();
        
        // Generate CLOSURE instruction (Lua 5.1官方设计：使用0基索引)

        compiler->emitInstruction(Instruction::createCLOSURE(closureReg, prototypeIndex));

        // Generate upvalue binding instructions
        for (const auto& upvalue : upvalues) {
            // Create upvalue binding instruction for VM
            // A = isLocal flag (1 for local, 0 for upvalue)
            // B = source index (local variable register index or parent upvalue index)
            u8 isLocal = upvalue.isLocal ? 1 : 0;
            u8 sourceIndex = static_cast<u8>(upvalue.stackIndex);  // Use stackIndex, not index



            // Validate source index bounds
            if (sourceIndex > 255) {
                throw LuaException("Upvalue source index out of bounds: " + std::to_string(sourceIndex));
            }

            // Use a generic instruction format for upvalue binding
            // This will be interpreted by VM's op_closure method
            // Note: These are not real instructions, just data for upvalue binding
            Instruction upvalBinding;
            upvalBinding.setOpCode(OpCode::MOVE);  // Use MOVE as a placeholder opcode
            upvalBinding.setA(isLocal);
            upvalBinding.setB(sourceIndex);



            compiler->emitInstruction(upvalBinding);
        }

        // Store function in global or local variable
        auto varInfo = compiler->resolveVariable(stmt->getName());
        switch (varInfo.type) {
            case Compiler::VariableType::Local:
                compiler->emitInstruction(Instruction::createMOVE(varInfo.index, closureReg));
                break;
            case Compiler::VariableType::Upvalue:
                compiler->emitInstruction(Instruction::createSETUPVAL(closureReg, varInfo.index));
                break;
            case Compiler::VariableType::Global:
                compiler->emitInstruction(Instruction::createSETGLOBAL(closureReg, varInfo.index));
                break;
        }
        
        // Free closure register
        compiler->freeReg();
        
        // Exit function scope
        compiler->exitFunctionScope();
    }

    void StatementCompiler::compileDoStmt(const DoStmt* stmt) {
        // Do statement is just a block with its own scope
        // The body is already a block statement, so we just compile it
        compileStmt(stmt->getBody());
    }
}