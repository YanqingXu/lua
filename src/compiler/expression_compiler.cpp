#include "expression_compiler.hpp"
#include "compiler.hpp"
#include "symbol_table.hpp"
#include "upvalue_analyzer.hpp"
#include "../common/opcodes.hpp"
#include "../common/defines.hpp"
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace Lua {
    ExpressionCompiler::ExpressionCompiler(Compiler* compiler) : compiler(compiler) {}
    
    int ExpressionCompiler::compileExpr(const Expr* expr) {
        if (!expr) {
            throw LuaException("Null expression in compilation");
        }
        
        // std::cout << "compileExpr: Expression type = " << static_cast<int>(expr->getType()) << std::endl;
        
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
            case ExprType::Table:
                return compileTableConstructor(static_cast<const TableExpr*>(expr));
            case ExprType::Index:
                return compileIndexAccess(static_cast<const IndexExpr*>(expr));
            case ExprType::Member:
                return compileMemberAccess(static_cast<const MemberExpr*>(expr));
            case ExprType::Function:
                return compileFunctionExpr(static_cast<const FunctionExpr*>(expr));
            case ExprType::Vararg:
                return compileVararg(static_cast<const VarargExpr*>(expr));
            default:
                throw LuaException("Unknown expression type in compilation");
        }
    }
    
    int ExpressionCompiler::compileLiteral(const LiteralExpr* expr) {
        if (!expr) {
            throw LuaException("Null literal expression in compilation");
        }
        
        int reg = compiler->allocReg();
        
        switch (expr->getValue().type()) {
            case ValueType::Nil:
                compiler->emitInstruction(Instruction::createLOADNIL(reg));
                break;
                
            case ValueType::Boolean:
                compiler->emitInstruction(Instruction::createLOADBOOL(reg, 
                    expr->getValue().asBoolean()));
                break;
                
            case ValueType::Number:
            case ValueType::String: {
                Value val = expr->getValue();
                int constIdx = compiler->addConstant(val);
                if (constIdx < 0 || constIdx > 65535) {
                    throw LuaException("Constant index out of range for LOADK instruction");
                }
                // Lua官方设计：使用0基索引，直接使用寄存器编号
                compiler->emitInstruction(Instruction::createLOADK(reg, static_cast<u16>(constIdx)));
                break;
            }
            
            case ValueType::Table:
            case ValueType::Function: {
                // For complex literals like tables and functions, add to constant table
                int constIdx = compiler->addConstant(expr->getValue());
                if (constIdx < 0 || constIdx > 65535) {
                    throw LuaException("Constant index out of range for LOADK instruction");
                }
                // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
                compiler->emitInstruction(Instruction::createLOADK(reg, static_cast<u16>(constIdx)));
                break;
            }
            
            default:
                throw LuaException("Unsupported literal type in compilation");
        }
        
        return reg;
    }
    
    int ExpressionCompiler::compileVariable(const VariableExpr* expr) {
        if (!expr) {
            throw LuaException("Null variable expression in compilation");
        }

        const Str& name = expr->getName();

        // Use unified variable resolution
        auto varInfo = compiler->resolveVariable(name);

        switch (varInfo.type) {
            case Compiler::VariableType::Local: {
                // Local variable - return register directly
                return varInfo.index;
            }

            case Compiler::VariableType::Upvalue: {
                // Upvalue - generate GETUPVAL instruction
                int reg = compiler->allocReg();

                compiler->emitInstruction(Instruction::createGETUPVAL(reg, varInfo.index));
                return reg;
            }

            case Compiler::VariableType::Global: {
                // Global variable - generate GETGLOBAL instruction
                int reg = compiler->allocReg();
                compiler->emitInstruction(Instruction::createGETGLOBAL(reg, varInfo.index));
                return reg;
            }

            default:
                throw LuaException("Unknown variable type for: " + name);
        }
    }
    
    int ExpressionCompiler::compileUnary(const UnaryExpr* expr) {
        int operandReg = compileExpr(expr->getRight());
        int resultReg = compiler->allocReg();
        
        switch (expr->getOperator()) {
            case TokenType::Minus:
                // Use metamethod-aware unary minus for full Lua 5.1 compatibility
                compiler->emitInstruction(Instruction::createUNM_MM(resultReg, operandReg));
                break;
            case TokenType::Not:
                compiler->emitInstruction(Instruction::createNOT(resultReg, operandReg));
                break;
            case TokenType::Hash:
                // Lua官方设计：使用0基索引，直接使用寄存器编号
                compiler->emitInstruction(Instruction::createLEN(resultReg, operandReg));
                break;
            default:
                throw LuaException("Unknown unary operator");
        }
        
        // 暂时不释放寄存器
        return resultReg;
    }
    
    int ExpressionCompiler::compileBinary(const BinaryExpr* expr) {
        // std::cout << "compileBinary: Starting compilation" << std::endl;
        TokenType op = expr->getOperator();
        // std::cout << "compileBinary: Operator = " << static_cast<int>(op) << std::endl;
        
        // Handle logical operators with short-circuit evaluation
        if (op == TokenType::And || op == TokenType::Or) {
            return compileLogicalOp(expr);
        }
        
        // Try constant folding optimization
        if (tryConstantFolding(expr)) {
            // If both operands are constants, compute at compile time
            Value result = evaluateConstantBinary(expr);
            int constIdx = compiler->addConstant(result);
            int resultReg = compiler->allocReg();
            compiler->emitInstruction(Instruction::createLOADK(resultReg, constIdx));
            return resultReg;
        }
        
        // Compile operands
        int leftReg = compileExpr(expr->getLeft());
        int rightReg = compileExpr(expr->getRight());
        int resultReg = compiler->allocReg();

        // Debug output disabled for production
        
        // Generate appropriate instruction based on operator
        switch (op) {
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Star:
            case TokenType::Slash:
            case TokenType::Percent:
            case TokenType::Caret:
                compileArithmeticOp(op, resultReg, leftReg, rightReg);
                break;
            case TokenType::Equal:
            case TokenType::NotEqual:
            case TokenType::Less:
            case TokenType::LessEqual:
            case TokenType::Greater:
            case TokenType::GreaterEqual:
                compileComparisonOp(op, resultReg, leftReg, rightReg);
                break;
            case TokenType::DotDot:
                // Use basic concatenation, metamethods will be handled at runtime if needed
                compiler->emitInstruction(Instruction::createCONCAT(resultReg, leftReg, rightReg));

                // Special handling for concatenation: don't free operand registers
                // to avoid register conflicts in complex concatenation expressions
                return resultReg;
            default:
                throw LuaException("Unsupported binary operator");
        }

        // 释放操作数寄存器（Lua 5.1官方设计）
        // 注意：只有当操作数是临时值时才释放，局部变量不释放
        compiler->freeReg(); // 释放rightReg
        compiler->freeReg(); // 释放leftReg

        return resultReg;
    }
    
    int ExpressionCompiler::compileCall(const CallExpr* expr) {
        // 完全重写：最简单、最直接的实现
        const auto& args = expr->getArguments();
        int nargs = static_cast<int>(args.size());

        // 检查最后一个参数是否是可变参数表达式
        bool hasVarargExpansion = false;
        if (nargs > 0) {
            const auto* lastArg = args[nargs - 1].get();
            if (dynamic_cast<const VarargExpr*>(lastArg)) {
                hasVarargExpansion = true;
            }
        }

        // 新策略：分配连续寄存器，然后直接编译到目标位置
        // 这样避免寄存器冲突

        // 步骤1：分配连续的寄存器块（Lua 5.1官方设计）
        int callFrameSize = 1 + nargs;  // 函数 + 参数
        int base = compiler->getRegisterManager().allocateCallFrame(callFrameSize, "call");

        // 参数寄存器是连续的
        Vec<int> argTargets;
        for (int i = 0; i < nargs; i++) {
            int argTarget = base + 1 + i;  // 参数从base+1开始
            argTargets.push_back(argTarget);
        }

        // 步骤2：编译函数到临时寄存器，然后移动
        int funcReg = compileExpr(expr->getCallee());

        if (funcReg != base) {
            compiler->emitInstruction(Instruction::createMOVE(base, funcReg));
        }

        // 步骤3：编译每个参数到临时寄存器，然后移动
        for (int i = 0; i < nargs; i++) {
            const auto* arg = args[i].get();

            // 特殊处理可变参数表达式
            if (dynamic_cast<const VarargExpr*>(arg)) {
                // 如果是最后一个参数且是可变参数，展开所有可变参数
                if (i == nargs - 1 && hasVarargExpansion) {
                    // 生成VARARG指令，B=0表示获取所有可变参数
                    compiler->emitInstruction(Instruction::createVARARG(argTargets[i], 0));
                } else {
                    // 否则只获取第一个可变参数
                    compiler->emitInstruction(Instruction::createVARARG(argTargets[i], 2));
                }
            } else {
                // 普通参数
                int argReg = compileExpr(arg);
                if (argReg != argTargets[i]) {
                    compiler->emitInstruction(Instruction::createMOVE(argTargets[i], argReg));
                }
            }
        }

        // 步骤5：发出CALL_MM指令（支持__call元方法）
        // CALL_MM a b c: 函数在a，参数在a+1..a+nargs，结果在a
        int callA = base;
        int callB = hasVarargExpansion ? 0 : (nargs + 1);  // 如果有可变参数展开，B=0表示使用栈顶
        int callC = 2;          // 期望1个返回值（默认）

        compiler->emitInstruction(Instruction::createCALL_MM(callA, callB, callC));

        // 步骤6：返回结果寄存器（函数调用后结果在base）
        return base;
    }

    int ExpressionCompiler::compileCallWithReturnCount(const CallExpr* expr, int expectedReturns) {
        // 与compileCall相同的逻辑，但使用指定的返回值数量
        const auto& args = expr->getArguments();
        int nargs = static_cast<int>(args.size());

        // 检查最后一个参数是否是可变参数表达式
        bool hasVarargExpansion = false;
        if (nargs > 0) {
            const auto* lastArg = args[nargs - 1].get();
            if (dynamic_cast<const VarargExpr*>(lastArg)) {
                hasVarargExpansion = true;
            }
        }

        // 步骤1：分配连续的寄存器块（Lua 5.1官方设计）
        int callFrameSize = 1 + nargs;  // 函数 + 参数
        int base = compiler->getRegisterManager().allocateCallFrame(callFrameSize, "call");

        // 参数寄存器是连续的
        Vec<int> argTargets;
        for (int i = 0; i < nargs; i++) {
            int argTarget = base + 1 + i;  // 参数从base+1开始
            argTargets.push_back(argTarget);
        }

        // 步骤2：编译函数到临时寄存器，然后移动
        int funcReg = compileExpr(expr->getCallee());

        if (funcReg != base) {
            compiler->emitInstruction(Instruction::createMOVE(base, funcReg));
        }

        // 步骤3：编译每个参数到临时寄存器，然后移动
        for (int i = 0; i < nargs; i++) {
            const auto* arg = args[i].get();

            // 特殊处理可变参数表达式
            if (dynamic_cast<const VarargExpr*>(arg)) {
                // 如果是最后一个参数且是可变参数，展开所有可变参数
                if (i == nargs - 1 && hasVarargExpansion) {
                    // 生成VARARG指令，B=0表示获取所有可变参数
                    compiler->emitInstruction(Instruction::createVARARG(argTargets[i], 0));
                } else {
                    // 否则只获取第一个可变参数
                    compiler->emitInstruction(Instruction::createVARARG(argTargets[i], 2));
                }
            } else {
                // 普通参数
                int argReg = compileExpr(arg);
                if (argReg != argTargets[i]) {
                    compiler->emitInstruction(Instruction::createMOVE(argTargets[i], argReg));
                }
            }
        }

        // 步骤5：发出CALL_MM指令（支持__call元方法）
        // CALL_MM a b c: 函数在a，参数在a+1..a+nargs，结果在a
        int callA = base;
        int callB = hasVarargExpansion ? 0 : (nargs + 1);  // 如果有可变参数展开，B=0表示使用栈顶
        int callC = (expectedReturns == -1) ? 0 : (expectedReturns + 1);  // 使用指定的返回值数量

        std::cout << "=== DEBUG: Compiler generating CALL_MM with expectedReturns=" << expectedReturns
                  << " callC=" << callC << " ===" << std::endl;

        compiler->emitInstruction(Instruction::createCALL_MM(callA, callB, callC));

        // 步骤6：返回结果寄存器（函数调用后结果在base）
        return base;
    }
    
    int ExpressionCompiler::compileTableConstructor(const TableExpr* expr) {
        int tableReg = compiler->allocReg();
        const auto& fields = expr->getFields();

        // DEBUG: Print table compilation info
        std::cout << "=== DEBUG: Table Constructor Compilation ===" << std::endl;
        std::cout << "Table register: " << tableReg << std::endl;
        std::cout << "Total fields: " << fields.size() << std::endl;

        // Count array and hash parts for table pre-sizing
        int arraySize = 0;
        int hashSize = 0;
        
        for (const auto& field : fields) {
            if (field.key == nullptr) {
                arraySize++; // Array-style field: {value}
                std::cout << "  Array field found" << std::endl;
            } else {
                hashSize++; // Hash-style field: {key = value} or {[expr] = value}
                std::cout << "  Hash field found" << std::endl;
            }
        }
        
        std::cout << "Array size: " << arraySize << ", Hash size: " << hashSize << std::endl;
        
        // Create new table with pre-sizing hints
        // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
        std::cout << "Emitting NEWTABLE instruction" << std::endl;
        compiler->emitInstruction(Instruction::createNEWTABLE(tableReg,
            static_cast<u8>(std::min(arraySize, 255)),
            static_cast<u8>(std::min(hashSize, 255))));
        
        // Initialize table fields
        int arrayIndex = 1; // Lua arrays start at index 1
        int fieldIndex = 0;
        
        for (const auto& field : fields) {
            std::cout << "Processing field " << fieldIndex++ << std::endl;
            
            if (field.key == nullptr) {
                // Array-style field: table[arrayIndex] = value
                std::cout << "  Array field: index=" << arrayIndex << std::endl;
                int valueReg = compileExpr(field.value.get());
                std::cout << "  Value compiled to register: " << valueReg << std::endl;
                int indexReg = compiler->allocReg();
                std::cout << "  Index register allocated: " << indexReg << std::endl;

                // Load array index as constant
                int indexConstant = compiler->addConstant(Value(static_cast<double>(arrayIndex)));
                std::cout << "  Index constant added: " << indexConstant << std::endl;
                compiler->emitInstruction(Instruction::createLOADK(indexReg, indexConstant));
                std::cout << "  LOADK instruction emitted" << std::endl;

                // Set table field: SETTABLE table[index] = value
                std::cout << "  Emitting SETTABLE: table=" << tableReg << ", index=" << indexReg << ", value=" << valueReg << std::endl;
                compiler->emitInstruction(Instruction::createSETTABLE(tableReg, indexReg, valueReg));

                // Free registers in reverse order
                compiler->freeReg(); // Free value register
                compiler->freeReg(); // Free index register
                std::cout << "  Registers freed" << std::endl;
                arrayIndex++;
            } else {
                // Hash-style field: table[key] = value
                std::cout << "  Hash field processing" << std::endl;
                int keyReg = compileExpr(field.key.get());
                std::cout << "  Key compiled to register: " << keyReg << std::endl;
                int valueReg = compileExpr(field.value.get());
                std::cout << "  Value compiled to register: " << valueReg << std::endl;

                // Set table field: SETTABLE table[key] = value
                // Note: SETTABLE instruction format is SETTABLE A B C where:
                // A = table register, B = key register, C = value register
                std::cout << "  Emitting SETTABLE: table=" << tableReg << ", key=" << keyReg << ", value=" << valueReg << std::endl;
                compiler->emitInstruction(Instruction::createSETTABLE(tableReg, keyReg, valueReg));

                // DON'T free registers here - let them be reused by the next field
                // The registers will be automatically managed by the compiler's scope
                std::cout << "  Registers NOT freed (fixed)" << std::endl;
            }
        }
        
        std::cout << "Table constructor compilation completed, returning register: " << tableReg << std::endl;
        std::cout << "===================================================" << std::endl;
        return tableReg;
    }
    
    int ExpressionCompiler::compileIndexAccess(const IndexExpr* expr) {
        int tableReg = compileExpr(expr->getObject());
        int indexReg = compileExpr(expr->getIndex());
        int resultReg = compiler->allocReg();

        // Use basic table access first, metamethods will be handled at runtime if needed
        compiler->emitInstruction(Instruction::createGETTABLE(resultReg, tableReg, indexReg));

        // 暂时不释放寄存器

        return resultReg;
    }
    
    int ExpressionCompiler::compileMemberAccess(const MemberExpr* expr) {
        int tableReg = compileExpr(expr->getObject());
        int resultReg = compiler->allocReg();

        // Convert member name to string constant
        int nameIdx = compiler->addConstant(Value(expr->getName()));

        // Use RK encoding for constant index (Lua 5.1 standard with 8-bit operands)
        if (nameIdx <= MAXINDEXRK_8) {
            u8 keyParam = RK(static_cast<u8>(nameIdx));
            // Use RK encoding for constant index access
            compiler->emitInstruction(Instruction::createGETTABLE(resultReg, tableReg, keyParam));
        } else {
            // Fallback to register approach for large constant indices
            // Fallback to register approach for large constant indices
            int keyReg = compiler->allocReg();
            compiler->emitInstruction(Instruction::createLOADK(keyReg, nameIdx));
            compiler->emitInstruction(Instruction::createGETTABLE(resultReg, tableReg, keyReg));
            compiler->freeReg(); // Free key register
        }

        // Note: Don't free table register here to avoid register allocation conflicts
        // The caller is responsible for register management

        return resultReg;
    }
    
    void ExpressionCompiler::compileArithmeticOp(TokenType op, int resultReg, int leftReg, int rightReg) {
        // Use metamethod-aware arithmetic instructions for full Lua 5.1 compatibility
        switch (op) {
            case TokenType::Plus:
                compiler->emitInstruction(Instruction::createADD_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::Minus:
                compiler->emitInstruction(Instruction::createSUB_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::Star:
                compiler->emitInstruction(Instruction::createMUL_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::Slash:
                compiler->emitInstruction(Instruction::createDIV_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::Percent:
                compiler->emitInstruction(Instruction::createMOD_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::Caret:
                compiler->emitInstruction(Instruction::createPOW(resultReg, leftReg, rightReg));
                break;
            default:
                throw LuaException("Unknown arithmetic operator");
        }
    }
    
    void ExpressionCompiler::compileComparisonOp(TokenType op, int resultReg, int leftReg, int rightReg) {
        // Use metamethod-aware comparison instructions for full Lua 5.1 compatibility
        switch (op) {
            case TokenType::Equal:
                compiler->emitInstruction(Instruction::createEQ_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::NotEqual:
                compiler->emitInstruction(Instruction::createEQ_MM(resultReg, leftReg, rightReg));
                compiler->emitInstruction(Instruction::createNOT(resultReg, resultReg));
                break;
            case TokenType::Less:
                compiler->emitInstruction(Instruction::createLT_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::LessEqual:
                compiler->emitInstruction(Instruction::createLE_MM(resultReg, leftReg, rightReg));
                break;
            case TokenType::Greater:
                compiler->emitInstruction(Instruction::createLT_MM(resultReg, rightReg, leftReg));
                break;
            case TokenType::GreaterEqual:
                compiler->emitInstruction(Instruction::createLE_MM(resultReg, rightReg, leftReg));
                break;
            default:
                throw LuaException("Unknown comparison operator");
        }
    }
    
    int ExpressionCompiler::compileLogicalOp(const BinaryExpr* expr) {
        int leftReg = compileExpr(expr->getLeft());
        int resultReg = compiler->allocReg();
        
        // Move left value to result register first
        compiler->emitInstruction(Instruction::createMOVE(resultReg, leftReg));
        
        if (expr->getOperator() == TokenType::And) {
            // Short-circuit AND: if left is false/nil, result is left; otherwise evaluate right
            // TEST leftReg, 0 - test if leftReg is false/nil
            compiler->emitInstruction(Instruction::createTEST(leftReg, 0));
            
            // If left is false/nil, skip right evaluation (jump to end)
            int jumpToEnd = compiler->emitJump();
            
            // Left is true, evaluate right operand
            int rightReg = compileExpr(expr->getRight());
            
            // Move right result to result register
            compiler->emitInstruction(Instruction::createMOVE(resultReg, rightReg));
            compiler->freeReg(); // Free right register
            
            // Patch jump to end
            compiler->patchJump(jumpToEnd);
            
        } else { // TokenType::Or
            // Short-circuit OR: if left is true, result is left; otherwise evaluate right
            // TEST leftReg, 1 - test if leftReg is true (invert test)
            compiler->emitInstruction(Instruction::createTEST(leftReg, 1));
            
            // If left is true, skip right evaluation (jump to end)
            int jumpToEnd = compiler->emitJump();
            
            // Left is false/nil, evaluate right operand
            int rightReg = compileExpr(expr->getRight());
            
            // Move right result to result register
            compiler->emitInstruction(Instruction::createMOVE(resultReg, rightReg));
            compiler->freeReg(); // Free right register
            
            // Patch jump to end
            compiler->patchJump(jumpToEnd);
        }
        
        compiler->freeReg(); // Free left register
        return resultReg;
    }
    

    
    // Optimization methods implementation
    bool ExpressionCompiler::tryConstantFolding(const BinaryExpr* expr) {
        return isConstantExpression(expr->getLeft()) && isConstantExpression(expr->getRight());
    }
    
    Value ExpressionCompiler::evaluateConstantBinary(const BinaryExpr* expr) {
        Value left = getConstantValue(expr->getLeft());
        Value right = getConstantValue(expr->getRight());
        TokenType op = expr->getOperator();

        // Handle string concatenation
        if (op == TokenType::DotDot) {
            // Convert operands to strings
            std::string leftStr, rightStr;

            if (left.isString()) {
                leftStr = left.asString();
            } else if (left.isNumber()) {
                LuaNumber num = left.asNumber();
                if (num == std::floor(num)) {
                    leftStr = std::to_string(static_cast<long long>(num));
                } else {
                    leftStr = std::to_string(num);
                }
            } else {
                throw LuaException("attempt to concatenate non-string/number value (left operand)");
            }

            if (right.isString()) {
                rightStr = right.asString();
            } else if (right.isNumber()) {
                LuaNumber num = right.asNumber();
                if (num == std::floor(num)) {
                    rightStr = std::to_string(static_cast<long long>(num));
                } else {
                    rightStr = std::to_string(num);
                }
            } else {
                throw LuaException("attempt to concatenate non-string/number value (right operand)");
            }

            return Value(leftStr + rightStr);
        }

        // Handle numeric operations
        if (left.type() != ValueType::Number || right.type() != ValueType::Number) {
            throw LuaException("Constant folding only supports numeric and string concatenation operations");
        }

        double leftVal = left.asNumber();
        double rightVal = right.asNumber();
        
        switch (op) {
            case TokenType::Plus:
                return Value(leftVal + rightVal);
            case TokenType::Minus:
                return Value(leftVal - rightVal);
            case TokenType::Star:
                return Value(leftVal * rightVal);
            case TokenType::Slash:
                if (rightVal == 0.0) {
                    throw LuaException("Division by zero in constant expression");
                }
                return Value(leftVal / rightVal);
            case TokenType::Percent:
                if (rightVal == 0.0) {
                    throw LuaException("Modulo by zero in constant expression");
                }
                return Value(fmod(leftVal, rightVal));
            case TokenType::Caret:
                return Value(pow(leftVal, rightVal));
            case TokenType::Equal:
                return Value(leftVal == rightVal);
            case TokenType::NotEqual:
                return Value(leftVal != rightVal);
            case TokenType::Less:
                return Value(leftVal < rightVal);
            case TokenType::LessEqual:
                return Value(leftVal <= rightVal);
            case TokenType::Greater:
                return Value(leftVal > rightVal);
            case TokenType::GreaterEqual:
                return Value(leftVal >= rightVal);
            default:
                throw LuaException("Unsupported operator for constant folding");
        }
    }
    
    bool ExpressionCompiler::isConstantExpression(const Expr* expr) {
        if (!expr) return false;
        
        switch (expr->getType()) {
            case ExprType::Literal:
                return true;
            case ExprType::Binary: {
                const BinaryExpr* binExpr = static_cast<const BinaryExpr*>(expr);
                return isConstantExpression(binExpr->getLeft()) && 
                       isConstantExpression(binExpr->getRight());
            }
            case ExprType::Unary: {
                const UnaryExpr* unExpr = static_cast<const UnaryExpr*>(expr);
                return isConstantExpression(unExpr->getRight());
            }
            default:
                return false;
        }
    }
    
    Value ExpressionCompiler::getConstantValue(const Expr* expr) {
        if (!expr || !isConstantExpression(expr)) {
            throw LuaException("Expression is not a constant");
        }
        
        switch (expr->getType()) {
            case ExprType::Literal: {
                const LiteralExpr* litExpr = static_cast<const LiteralExpr*>(expr);
                return litExpr->getValue();
            }
            case ExprType::Binary: {
                const BinaryExpr* binExpr = static_cast<const BinaryExpr*>(expr);
                return evaluateConstantBinary(binExpr);
            }
            case ExprType::Unary: {
                const UnaryExpr* unExpr = static_cast<const UnaryExpr*>(expr);
                Value operand = getConstantValue(unExpr->getRight());
                
                switch (unExpr->getOperator()) {
                    case TokenType::Minus:
                        if (operand.type() == ValueType::Number) {
                            return Value(-operand.asNumber());
                        }
                        break;
                    case TokenType::Not:
                        return Value(!operand.asBoolean());
                    case TokenType::Hash:
                        if (operand.type() == ValueType::String) {
                            return Value(static_cast<double>(operand.asString().length()));
                        }
                        break;
                }
                throw LuaException("Unsupported unary operator for constant folding");
            }
            default:
                throw LuaException("Unsupported expression type for constant value");
        }
    }
    
    int ExpressionCompiler::compileFunctionExpr(const FunctionExpr* expr) {
        if (!expr) {
            throw LuaException("Null function expression in compilation");
        }

        // Check function nesting depth before proceeding
        compiler->enterFunctionScope();

        // Create compilation context for the nested function
        auto childContext = compiler->createChildContext();

        // Create a new compiler instance for the function body with parent context
        Compiler functionCompiler(childContext);

        // Inherit the nesting depth from parent compiler
        for (int i = 0; i < compiler->getFunctionNestingDepth(); ++i) {
            functionCompiler.enterFunctionScope();
        }

        // Analyze upvalues using the parent compiler's scope manager
        // This allows the analyzer to see variables from outer scopes
        UpvalueAnalyzer analyzer(compiler->getScopeManager());

        // Analyze the function body to find upvalues
        analyzer.analyzeFunction(expr);
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
        for (const auto& param : expr->getParameters()) {
            functionCompiler.defineLocal(param);  // 寄存器1, 2, 3...
        }
        
        // Handle variadic functions - reserve space for vararg table if needed
        if (expr->getIsVariadic()) {
            // In Lua, variadic functions can access extra arguments via ...
            // This is typically handled at runtime by the VM
        }
        
        // Compile function body
        functionCompiler.compileStmt(expr->getBody());
        
        // End function scope
        functionCompiler.endScope();
        
        // Create function object and add to prototypes
        auto function = Function::createLua(
            functionCompiler.getCode(),
            functionCompiler.getConstants(),
            functionCompiler.getPrototypes(),
            static_cast<u8>(expr->getParameters().size()),
            static_cast<u8>(functionCompiler.getScopeManager().getLocalCount()),
            static_cast<u8>(upvalues.size()),
            expr->getIsVariadic()
        );
        int prototypeIndex = compiler->addPrototype(function);
        
        // Check prototype index bounds for CLOSURE instruction
        if (prototypeIndex > 65535) { // u16 max value
            throw LuaException("Too many function prototypes in compilation unit");
        }
        
        // Allocate register for the function
        int reg = compiler->allocReg();
        
        // Emit CLOSURE instruction to create closure
        compiler->emitInstruction(Instruction::createCLOSURE(reg, static_cast<u16>(prototypeIndex)));
        
        // Handle upvalues if any
        // VM expects upvalue binding instructions immediately after CLOSURE
        // Check if we have too many upvalues to avoid register overflow
        if (upvalues.size() > MAX_UPVALUES_PER_CLOSURE) {
            throw LuaException("Too many upvalues in closure: " + std::to_string(upvalues.size()) + 
                             " (max: " + std::to_string(MAX_UPVALUES_PER_CLOSURE) + ")");
        }
        
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
            Instruction upvalBinding;
            upvalBinding.setA(isLocal);
            upvalBinding.setB(sourceIndex);
            compiler->emitInstruction(upvalBinding);
        }
        
        // Exit function scope
        compiler->exitFunctionScope();

        return reg;
    }

    int ExpressionCompiler::compileVararg(const VarargExpr* expr) {
        if (!expr) {
            throw LuaException("Null vararg expression in compilation");
        }

        // Generate VARARG instruction
        // In Lua 5.1, VARARG A B means:
        // - A: starting register to store varargs
        // - B: number of varargs to retrieve (0 means all, 1 means none, 2 means 1, etc.)
        int reg = compiler->allocReg();

        // For a simple vararg expression like "local x = ...", we want the first vararg
        // So we use B=2 (which means 1 vararg)
        compiler->emitInstruction(Instruction::createVARARG(reg, 2));

        return reg;
    }
}