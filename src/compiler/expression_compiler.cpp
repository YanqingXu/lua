#include "expression_compiler.hpp"
#include "compiler.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>
#include <cmath>

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
                int constIdx = compiler->addConstant(expr->getValue());
                if (constIdx < 0 || constIdx > 65535) {
                    throw LuaException("Constant index out of range for LOADK instruction");
                }
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
        
        // Try to find local variable
        int localSlot = compiler->resolveLocal(name);
        if (localSlot != -1) {
            return localSlot; // Local variables are already in registers
        }
        
        // Global variable access
        int reg = compiler->allocReg();
        int nameIdx = compiler->addConstant(Value(name));
        compiler->emitInstruction(Instruction::createGETGLOBAL(reg, nameIdx));
        
        return reg;
    }
    
    int ExpressionCompiler::compileUnary(const UnaryExpr* expr) {
        int operandReg = compileExpr(expr->getRight());
        int resultReg = compiler->allocReg();
        
        switch (expr->getOperator()) {
            case TokenType::Minus:
                compiler->emitInstruction(Instruction::createUNM(resultReg, operandReg));
                break;
            case TokenType::Not:
                compiler->emitInstruction(Instruction::createNOT(resultReg, operandReg));
                break;
            case TokenType::Hash:
                compiler->emitInstruction(Instruction::createLEN(resultReg, operandReg));
                break;
            default:
                throw LuaException("Unknown unary operator");
        }
        
        compiler->freeReg(); // Free operand register
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
                // Generate CONCAT instruction: result = left .. right
                compiler->emitInstruction(Instruction::createCONCAT(resultReg, leftReg, rightReg));
                break;
            default:
                throw LuaException("Unsupported binary operator");
        }
        
        compiler->freeReg(); // Free right operand register
        compiler->freeReg(); // Free left operand register
        
        return resultReg;
    }
    
    int ExpressionCompiler::compileCall(const CallExpr* expr) {
        // Compile function expression
        int funcReg = compileExpr(expr->getCallee());
        
        // Compile arguments
        const auto& args = expr->getArguments();
        Vec<int> argRegs;
        
        for (const auto& arg : args) {
            int argReg = compileExpr(arg.get());
            argRegs.push_back(argReg);
        }
        
        // Emit call instruction
        int resultReg = compiler->allocReg();
        compiler->emitInstruction(Instruction::createCALL(funcReg, 
            static_cast<int>(args.size()), 1));
        
        // Move result to result register
        compiler->emitInstruction(Instruction::createMOVE(resultReg, funcReg));
        
        // Free argument registers
        for (size_t i = 0; i < argRegs.size(); ++i) {
            compiler->freeReg();
        }
        compiler->freeReg(); // Free function register
        
        return resultReg;
    }
    
    int ExpressionCompiler::compileTableConstructor(const TableExpr* expr) {
        int tableReg = compiler->allocReg();
        const auto& fields = expr->getFields();
        
        // Count array and hash parts for table pre-sizing
        int arraySize = 0;
        int hashSize = 0;
        
        for (const auto& field : fields) {
            if (field.key == nullptr) {
                arraySize++; // Array-style field: {value}
            } else {
                hashSize++; // Hash-style field: {key = value} or {[expr] = value}
            }
        }
        
        // Create new table with pre-sizing hints
        compiler->emitInstruction(Instruction::createNEWTABLE(tableReg, 
            static_cast<u8>(std::min(arraySize, 255)), 
            static_cast<u8>(std::min(hashSize, 255))));
        
        // Initialize table fields
        int arrayIndex = 1; // Lua arrays start at index 1
        
        for (const auto& field : fields) {
            if (field.key == nullptr) {
                // Array-style field: table[arrayIndex] = value
                int valueReg = compileExpr(field.value.get());
                int indexReg = compiler->allocReg();
                
                // Load array index as constant
                int indexConstant = compiler->addConstant(Value(static_cast<double>(arrayIndex)));
                compiler->emitInstruction(Instruction::createLOADK(indexReg, indexConstant));
                
                // Set table field: SETTABLE table[index] = value
                compiler->emitInstruction(Instruction::createSETTABLE(tableReg, indexReg, valueReg));
                
                compiler->freeReg(); // Free value register
                compiler->freeReg(); // Free index register
                arrayIndex++;
            } else {
                // Hash-style field: table[key] = value
                int keyReg = compileExpr(field.key.get());
                int valueReg = compileExpr(field.value.get());
                
                // Set table field: SETTABLE table[key] = value
                compiler->emitInstruction(Instruction::createSETTABLE(tableReg, keyReg, valueReg));
                
                compiler->freeReg(); // Free value register
                compiler->freeReg(); // Free key register
            }
        }
        
        return tableReg;
    }
    
    int ExpressionCompiler::compileIndexAccess(const IndexExpr* expr) {
        int tableReg = compileExpr(expr->getObject());
        int indexReg = compileExpr(expr->getIndex());
        int resultReg = compiler->allocReg();
        
        compiler->emitInstruction(Instruction::createGETTABLE(resultReg, tableReg, indexReg));
        
        compiler->freeReg(); // Free index register
        compiler->freeReg(); // Free table register
        
        return resultReg;
    }
    
    int ExpressionCompiler::compileMemberAccess(const MemberExpr* expr) {
        int tableReg = compileExpr(expr->getObject());
        int resultReg = compiler->allocReg();
        
        // Convert member name to string constant
        int nameIdx = compiler->addConstant(Value(expr->getName()));
        
        compiler->emitInstruction(Instruction::createGETTABLE(resultReg, tableReg, nameIdx));
        
        compiler->freeReg(); // Free table register
        
        return resultReg;
    }
    
    void ExpressionCompiler::compileArithmeticOp(TokenType op, int resultReg, int leftReg, int rightReg) {
        switch (op) {
            case TokenType::Plus:
                compiler->emitInstruction(Instruction::createADD(resultReg, leftReg, rightReg));
                break;
            case TokenType::Minus:
                compiler->emitInstruction(Instruction::createSUB(resultReg, leftReg, rightReg));
                break;
            case TokenType::Star:
                compiler->emitInstruction(Instruction::createMUL(resultReg, leftReg, rightReg));
                break;
            case TokenType::Slash:
                compiler->emitInstruction(Instruction::createDIV(resultReg, leftReg, rightReg));
                break;
            case TokenType::Percent:
                compiler->emitInstruction(Instruction::createMOD(resultReg, leftReg, rightReg));
                break;
            case TokenType::Caret:
                compiler->emitInstruction(Instruction::createPOW(resultReg, leftReg, rightReg));
                break;
            default:
                throw LuaException("Unknown arithmetic operator");
        }
    }
    
    void ExpressionCompiler::compileComparisonOp(TokenType op, int resultReg, int leftReg, int rightReg) {
        switch (op) {
            case TokenType::Equal:
                compiler->emitInstruction(Instruction::createEQ(resultReg, leftReg, rightReg));
                break;
            case TokenType::NotEqual:
                compiler->emitInstruction(Instruction::createEQ(resultReg, leftReg, rightReg));
                compiler->emitInstruction(Instruction::createNOT(resultReg, resultReg));
                break;
            case TokenType::Less:
                compiler->emitInstruction(Instruction::createLT(resultReg, leftReg, rightReg));
                break;
            case TokenType::LessEqual:
                compiler->emitInstruction(Instruction::createLE(resultReg, leftReg, rightReg));
                break;
            case TokenType::Greater:
                compiler->emitInstruction(Instruction::createLT(resultReg, rightReg, leftReg));
                break;
            case TokenType::GreaterEqual:
                compiler->emitInstruction(Instruction::createLE(resultReg, rightReg, leftReg));
                break;
            default:
                throw LuaException("Unknown comparison operator");
        }
    }
    
    int ExpressionCompiler::compileLogicalOp(const BinaryExpr* expr) {
        int leftReg = compileExpr(expr->getLeft());
        
        if (expr->getOperator() == TokenType::And) {
            // Short-circuit AND: if left is false, result is left; otherwise result is right
            int jumpIfFalse = compiler->emitJump();
            compiler->freeReg(); // Free left register
            
            int rightReg = compileExpr(expr->getRight());
            compiler->patchJump(jumpIfFalse);
            
            return rightReg;
        } else { // TokenType::Or
            // Short-circuit OR: if left is true, result is left; otherwise result is right
            int jumpIfTrue = compiler->emitJump();
            compiler->freeReg(); // Free left register
            
            int rightReg = compileExpr(expr->getRight());
            compiler->patchJump(jumpIfTrue);
            
            return rightReg;
        }
    }
    

    
    // Optimization methods implementation
    bool ExpressionCompiler::tryConstantFolding(const BinaryExpr* expr) {
        return isConstantExpression(expr->getLeft()) && isConstantExpression(expr->getRight());
    }
    
    Value ExpressionCompiler::evaluateConstantBinary(const BinaryExpr* expr) {
        Value left = getConstantValue(expr->getLeft());
        Value right = getConstantValue(expr->getRight());
        TokenType op = expr->getOperator();
        
        // Only handle numeric operations for now
        if (left.type() != ValueType::Number || right.type() != ValueType::Number) {
            throw LuaException("Constant folding only supports numeric operations");
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
}