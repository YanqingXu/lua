#include "compiler.hpp"

namespace Lua {
    Compiler::Compiler() : scopeDepth(0) {
        code = make_ptr<std::vector<Instruction>>();
    }
    
    int Compiler::addConstant(const Value& value) {
        constants.push_back(value);
        return constants.size() - 1;
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
        for (int i = locals.size() - 1; i >= 0; i--) {
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
        // 查找变量名
        int slot = resolveLocal(expr->getName());
        
        if (slot != -1) {
            // 局部变量直接返回其寄存器编号
            return slot;
        } else {
            int dst = allocReg();
            int constant = addConstant(expr->getName());
            emitInstruction(Instruction::createGETGLOBAL(dst, static_cast<u16>(constant)));
            return dst;
        }
    }
    
    int Compiler::compileUnary(const UnaryExpr* expr) {
        // 先编译操作数
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
        // 先编译左侧和右侧表达式
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
        // 编译被调用的函数或变量
        int base = compileExpr(expr->getCallee());
        
        // 编译参数
        int argCount = 0;
        for (const auto& arg : expr->getArguments()) {
            int reg = compileExpr(arg.get());
            // ensure arguments are contiguous; simplistic approach assumes they already are
            argCount++;
        }
        
        // 发出调用指令，指定参数数量
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
        // 结果不再需要
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
            default:
                throw LuaException("Unsupported statement type.");
        }
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
            return nullptr;
        }
    }
}
