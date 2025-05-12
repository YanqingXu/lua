#pragma once

#include "../types.hpp"
#include "state.hpp"
#include "function.hpp"

namespace Lua {
    class VM {
    private:
        State* state;
        Ptr<Function> currentFunction;
        Vec<Instruction>* code;
        Vec<Value>* constants;
        size_t pc; // 程序计数器
        
    public:
        explicit VM(State* state);
        
        // 执行函数
        Value execute(Ptr<Function> function);
        
    private:
        // 运行一条指令
        bool runInstruction();
        
        // 获取常量
        Value getConstant(u32 idx) const;
        
        // 处理各种操作码
        void op_move(Instruction i);
        void op_loadk(Instruction i);
        void op_loadbool(Instruction i);
        void op_loadnil(Instruction i);
        void op_getglobal(Instruction i);
        void op_setglobal(Instruction i);
        void op_newtable(Instruction i);
        void op_add(Instruction i);
        void op_sub(Instruction i);
        void op_mul(Instruction i);
        void op_div(Instruction i);
        void op_not(Instruction i);
        void op_eq(Instruction i);
        void op_lt(Instruction i);
        void op_le(Instruction i);
        void op_jmp(Instruction i);
        void op_call(Instruction i);
        void op_return(Instruction i);
    };
}
