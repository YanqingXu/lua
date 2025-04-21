#include "compiler.hpp"

namespace Lua {
    Compiler::Compiler() : scopeDepth(0) {
        code = make_ptr<std::vector<Instruction>>();
    }
    
    int Compiler::addConstant(const Value& value) {
        constants.push_back(value);
        return constants.size() - 1;
    }
    
    void Compiler::emitByte(u8 byte) {
        code->push_back(byte);
    }
    
    void Compiler::emitBytes(u8 byte1, u8 byte2) {
        emitByte(byte1);
        emitByte(byte2);
    }
    
    void Compiler::emitInstruction(Instruction instr) {
        code->push_back(instr);
    }
    
    int Compiler::emitJump(u8 instruction) {
        emitByte(instruction);
        emitByte(0xff);
        emitByte(0xff);
        return code->size() - 2;
    }
    
    void Compiler::patchJump(int offset) {
        // 计算跳转距离
        int jump = code->size() - offset - 2;
        
        if (jump > 0xffff) {
            // 跳转过远，无法编码
            throw LuaException("Too much code to jump over.");
        }
        
        // 更新跳转指令的操作数
        (*code)[offset] = (jump >> 8) & 0xff;
        (*code)[offset + 1] = jump & 0xff;
    }
    
    void Compiler::beginScope() {
        scopeDepth++;
    }
    
    void Compiler::endScope() {
        scopeDepth--;
        
        // 移除当前作用域的局部变量
        while (!locals.empty() && locals.back().depth > scopeDepth) {
            // 如果变量被捕获，则发出关闭上值的指令
            if (locals.back().isCaptured) {
                emitByte(OP_CLOSE_UPVALUE);
            } else {
                emitByte(OP_POP);
            }
            locals.pop_back();
        }
    }
    
    int Compiler::resolveLocal(const Str& name) {
        // 从后向前搜索，查找变量名称
        for (int i = locals.size() - 1; i >= 0; i--) {
            if (locals[i].name == name) {
                return i;
            }
        }
        
        return -1; // 未找到
    }
    
    void Compiler::compileLiteral(const LiteralExpr* expr) {
        const Value& value = expr->getValue();
        
        // 针对不同类型的字面量生成不同的指令
        if (value.isNil()) {
            emitByte(OP_NIL);
        } else if (value.isBoolean()) {
            emitByte(value.asBoolean() ? OP_TRUE : OP_FALSE);
        } else if (value.isNumber()) {
            // 为数字创建常量
            int constant = addConstant(value);
            emitBytes(OP_CONSTANT, constant);
        } else if (value.isString()) {
            // 为字符串创建常量
            int constant = addConstant(value);
            emitBytes(OP_CONSTANT, constant);
        } else {
            // 不支持的字面量类型
            throw LuaException("Unsupported literal type.");
        }
    }
    
    void Compiler::compileVariable(const VariableExpr* expr) {
        // 查找变量名
        int slot = resolveLocal(expr->getName());
        
        if (slot != -1) {
            // 局部变量
            emitBytes(OP_GET_LOCAL, slot);
        } else {
            // 全局变量
            int constant = addConstant(expr->getName());
            emitBytes(OP_GET_GLOBAL, constant);
        }
    }
    
    void Compiler::compileUnary(const UnaryExpr* expr) {
        // 先编译操作数
        compileExpr(expr->getRight());
        
        // 生成一元操作指令
        switch (expr->getOperator()) {
            case TokenType::Minus:
                emitByte(OP_NEGATE);
                break;
            case TokenType::Not:
                emitByte(OP_NOT);
                break;
            default:
                // 不支持的一元操作符
                throw LuaException("Unsupported unary operator.");
        }
    }
    
    void Compiler::compileBinary(const BinaryExpr* expr) {
        // 先编译左侧和右侧表达式
        compileExpr(expr->getLeft());
        compileExpr(expr->getRight());
        
        // 生成二元操作指令
        switch (expr->getOperator()) {
            case TokenType::Plus:
                emitByte(OP_ADD);
                break;
            case TokenType::Minus:
                emitByte(OP_SUBTRACT);
                break;
            case TokenType::Star:
                emitByte(OP_MULTIPLY);
                break;
            case TokenType::Slash:
                emitByte(OP_DIVIDE);
                break;
            case TokenType::Percent:
                emitByte(OP_MODULO);
                break;
            case TokenType::Equal:
                emitByte(OP_EQUAL);
                break;
            case TokenType::NotEqual:
                emitBytes(OP_EQUAL, OP_NOT);
                break;
            case TokenType::Less:
                emitByte(OP_LESS);
                break;
            case TokenType::LessEqual:
                emitBytes(OP_GREATER, OP_NOT);
                break;
            case TokenType::Greater:
                emitByte(OP_GREATER);
                break;
            case TokenType::GreaterEqual:
                emitBytes(OP_LESS, OP_NOT);
                break;
            default:
                // 不支持的二元操作符
                throw LuaException("Unsupported binary operator.");
        }
    }
    
    void Compiler::compileCall(const CallExpr* expr) {
        // 编译被调用的函数或变量
        compileExpr(expr->getCallee());
        
        // 编译参数
        int argCount = 0;
        for (const auto& arg : expr->getArguments()) {
            compileExpr(arg.get());
            argCount++;
        }
        
        // 发出调用指令，指定参数数量
        emitBytes(OP_CALL, argCount);
    }
    
    void Compiler::compileExpr(const Expr* expr) {
        if (expr == nullptr) return;
        
        switch (expr->getType()) {
            case ExprType::Literal:
                compileLiteral(static_cast<const LiteralExpr*>(expr));
                break;
            case ExprType::Variable:
                compileVariable(static_cast<const VariableExpr*>(expr));
                break;
            case ExprType::Unary:
                compileUnary(static_cast<const UnaryExpr*>(expr));
                break;
            case ExprType::Binary:
                compileBinary(static_cast<const BinaryExpr*>(expr));
                break;
            case ExprType::Call:
                compileCall(static_cast<const CallExpr*>(expr));
                break;
            default:
                throw LuaException("Unsupported expression type.");
        }
    }
    
    void Compiler::compileExprStmt(const ExprStmt* stmt) {
        compileExpr(stmt->getExpression());
        // 表达式语句执行后，结果值被丢弃
        emitByte(OP_POP);
    }
    
    void Compiler::compileBlockStmt(const BlockStmt* stmt) {
        beginScope();
        
        // 编译块中的每个语句
        for (const auto& statement : stmt->getStatements()) {
            compileStmt(statement.get());
        }
        
        endScope();
    }
    
    void Compiler::compileLocalStmt(const LocalStmt* stmt) {
        // 编译初始化器
        if (stmt->getInitializer() != nullptr) {
            compileExpr(stmt->getInitializer());
        } else {
            // 默认为nil
            emitByte(OP_NIL);
        }
        
        // 添加局部变量到作用域
        locals.push_back(Local(stmt->getName(), scopeDepth));
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
    
    Ptr<Function> Compiler::compile(const Vec<UniquePtr<Stmt>>& statements) {
        try {
            // 编译每个语句
            for (const auto& stmt : statements) {
                compileStmt(stmt.get());
            }
            
            // 添加返回指令
            emitByte(OP_RETURN);
            
            // 创建函数对象
            return Function::createLua(code, constants);
        } catch (const LuaException& e) {
            // 编译错误，返回nullptr
            return nullptr;
        }
    }
}
