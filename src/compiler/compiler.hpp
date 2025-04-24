#pragma once

#include "../types.hpp"
#include "../parser/parser.hpp"
#include "../vm/function.hpp"
#include "../common/opcodes.hpp"

namespace Lua {
    // 本地变量
    struct Local {
        Str name;
        int depth;
        bool isCaptured;
        
        Local(const Str& name, int depth)
            : name(name), depth(depth), isCaptured(false) {}
    };

    class Expr;
    class LiteralExpr;
    class VariableExpr;
    class UnaryExpr;
    class BinaryExpr;
    class CallExpr;
    class ExprStmt;
    class BlockStmt;
    class LocalStmt;
    class Stmt;

    
    // 编译器类
    class Compiler {
    private:
        // 当前作用域深度
        int scopeDepth;
        
        // 局部变量
        Vec<Local> locals;
        
        // 常量表
        Vec<Value> constants;
        
        // 字节码
        Ptr<Vec<Instruction>> code;
        
        // 当前块的跳转指令位置
        std::vector<int> breaks;
        
        // 编译表达式
        void compileExpr(const Expr* expr);
        void compileLiteral(const LiteralExpr* expr);
        void compileVariable(const VariableExpr* expr);
        void compileUnary(const UnaryExpr* expr);
        void compileBinary(const BinaryExpr* expr);
        void compileCall(const CallExpr* expr);
        
        // 编译语句
        void compileStmt(const Stmt* stmt);
        void compileExprStmt(const ExprStmt* stmt);
        void compileBlockStmt(const BlockStmt* stmt);
        void compileLocalStmt(const LocalStmt* stmt);
        
        // 辅助方法
        int addConstant(const Value& value);
        void emitByte(u8 byte);
        void emitBytes(u8 byte1, u8 byte2);
        void emitInstruction(Instruction instr);
        int emitJump(u8 instruction);
        void patchJump(int offset);
        void beginScope();
        void endScope();
        int resolveLocal(const Str& name);
        
    public:
        Compiler();
        
        // 编译AST，生成函数对象
        Ptr<Function> compile(const Vec<UPtr<Stmt>>& statements);
    };
}
