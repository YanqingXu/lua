#include "expression_compiler.hpp"
#include "compiler.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>

namespace Lua {
    ExpressionCompiler::ExpressionCompiler(Compiler* compiler) : compiler(compiler) {}
    
    int ExpressionCompiler::compileExpr(const Expr* expr) {
        if (!expr) {
            throw LuaException("Null expression in compilation");
        }
        
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
                compiler->emitInstruction(Instruction::createLOADK(reg, constIdx));
                break;
            }
            default:
                throw LuaException("Unsupported literal type");
        }
        
        return reg;
    }
    
    int ExpressionCompiler::compileVariable(const VariableExpr* expr) {
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
        TokenType op = expr->getOperator();
        
        // Handle different types of binary operations
        switch (op) {
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Star:
            case TokenType::Slash:
            case TokenType::Percent:
            case TokenType::Caret:
                return compileArithmeticOp(expr);
                
            case TokenType::Equal:
            case TokenType::NotEqual:
            case TokenType::Less:
            case TokenType::LessEqual:
            case TokenType::Greater:
            case TokenType::GreaterEqual:
                return compileComparisonOp(expr);
                
            case TokenType::And:
            case TokenType::Or:
                return compileLogicalOp(expr);
                
            case TokenType::DotDot:
                return compileConcatenationOp(expr);
                
            default:
                throw LuaException("Unknown binary operator");
        }
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
        
        // Create new table
        compiler->emitInstruction(Instruction::createNEWTABLE(tableReg, 0, 0));
        
        // TODO: Implement table field initialization
        // This would involve compiling field expressions and using SETTABLE instructions
        
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
    
    int ExpressionCompiler::compileArithmeticOp(const BinaryExpr* expr) {
        int leftReg = compileExpr(expr->getLeft());
        int rightReg = compileExpr(expr->getRight());
        int resultReg = compiler->allocReg();
        
        switch (expr->getOperator()) {
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
        
        compiler->freeReg(); // Free right register
        compiler->freeReg(); // Free left register
        
        return resultReg;
    }
    
    int ExpressionCompiler::compileComparisonOp(const BinaryExpr* expr) {
        int leftReg = compileExpr(expr->getLeft());
        int rightReg = compileExpr(expr->getRight());
        int resultReg = compiler->allocReg();
        
        switch (expr->getOperator()) {
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
        
        compiler->freeReg(); // Free right register
        compiler->freeReg(); // Free left register
        
        return resultReg;
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
    
    int ExpressionCompiler::compileConcatenationOp(const BinaryExpr* expr) {
        int leftReg = compileExpr(expr->getLeft());
        int rightReg = compileExpr(expr->getRight());
        int resultReg = compiler->allocReg();
        
        // TODO: Implement string concatenation instruction
        // For now, use a placeholder
        compiler->emitInstruction(Instruction::createADD(resultReg, leftReg, rightReg));
        
        compiler->freeReg(); // Free right register
        compiler->freeReg(); // Free left register
        
        return resultReg;
    }
}