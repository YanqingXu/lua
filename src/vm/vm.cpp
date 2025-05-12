#include "vm.hpp"
#include "table.hpp"
#include "instruction.hpp"
#include "../common/opcodes.hpp"
#include <stdexcept>

namespace Lua {
    VM::VM(State* state) : 
        state(state),
        currentFunction(nullptr),
        code(nullptr),
        constants(nullptr),
        pc(0) {
    }
    
    Value VM::execute(Ptr<Function> function) {
        if (!function || function->getType() != Function::Type::Lua) {
            throw LuaException("Cannot execute non-Lua function");
        }
        
        // 保存当前上下文
        auto oldFunction = currentFunction;
        auto oldCode = code;
        auto oldConstants = constants;
        auto oldPC = pc;
        
        // 设置新上下文
        currentFunction = function;
        
        // 获取函数的字节码和常量表
        const auto& functionCode = function->getCode();
        code = const_cast<Vec<Instruction>*>(&functionCode);
        
        const auto& functionConstants = function->getConstants();
        constants = const_cast<Vec<Value>*>(&functionConstants);
        
        pc = 0;
        
        // 执行函数
        Value result;
        
        try {
            // 执行字节码
            while (pc < code->size()) {
                if (!runInstruction()) {
                    break;  // 碰到return指令
                }
            }
            
            // 默认返回nil
            result = Value(nullptr);
        } catch (const std::exception& e) {
            // 恢复旧上下文
            currentFunction = oldFunction;
            code = oldCode;
            constants = oldConstants;
            pc = oldPC;
            
            // 重新抛出异常
            throw;
        }
        
        // 恢复旧上下文
        currentFunction = oldFunction;
        code = oldCode;
        constants = oldConstants;
        pc = oldPC;
        
        return result;
    }
    
    bool VM::runInstruction() {
        // 获取当前指令
        Instruction i = (*code)[pc++];
        
        // 根据操作码分派
        OpCode op = i.getOpCode();
        
        switch (op) {
            case OpCode::MOVE:
                op_move(i);
                break;
            case OpCode::LOADK:
                op_loadk(i);
                break;
            case OpCode::LOADBOOL:
                op_loadbool(i);
                break;
            case OpCode::LOADNIL:
                op_loadnil(i);
                break;
            case OpCode::GETGLOBAL:
                op_getglobal(i);
                break;
            case OpCode::SETGLOBAL:
                op_setglobal(i);
                break;
            case OpCode::NEWTABLE:
                op_newtable(i);
                break;
            case OpCode::CALL:
                op_call(i);
                break;
            case OpCode::ADD:
                op_add(i);
                break;
            case OpCode::SUB:
                op_sub(i);
                break;
            case OpCode::MUL:
                op_mul(i);
                break;
            case OpCode::DIV:
                op_div(i);
                break;
            case OpCode::NOT:
                op_not(i);
                break;
            case OpCode::EQ:
                op_eq(i);
                break;
            case OpCode::LT:
                op_lt(i);
                break;
            case OpCode::LE:
                op_le(i);
                break;
            case OpCode::JMP:
                op_jmp(i);
                break;
            case OpCode::RETURN:
                op_return(i);
                return false;  // 停止执行
            default:
                throw LuaException("Unimplemented opcode: " + std::to_string(static_cast<int>(op)));
        }
        
        return true;
    }
    
    Value VM::getConstant(u32 idx) const {
        if (idx >= constants->size()) {
            throw LuaException("Invalid constant index");
        }
        return (*constants)[idx];
    }
    
    // 指令实现
    void VM::op_move(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        
        state->set(a + 1, state->get(b + 1));
    }
    
    void VM::op_loadk(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();
        
        state->set(a + 1, getConstant(bx));
    }
    
    void VM::op_loadbool(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        state->set(a + 1, Value(b != 0));
        
        if (c != 0) {
            pc++;  // 跳过下一条指令
        }
    }
    
    void VM::op_loadnil(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        
        for (u32 j = a; j <= a + b; j++) {
            state->set(j + 1, Value(nullptr));
        }
    }
    
    void VM::op_getglobal(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();
        
        Value key = getConstant(bx);
        if (!key.isString()) {
            throw LuaException("Global name must be a string");
        }
        
        state->set(a + 1, state->getGlobal(key.asString()));
    }
    
    void VM::op_setglobal(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();
        
        Value key = getConstant(bx);
        if (!key.isString()) {
            throw LuaException("Global name must be a string");
        }
        
        state->setGlobal(key.asString(), state->get(a + 1));
    }
    
    void VM::op_newtable(Instruction i) {
        u8 a = i.getA();
        state->set(a + 1, make_ptr<Table>());
    }
    
    void VM::op_add(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            state->set(a + 1, Value(bn + cn));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_sub(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            state->set(a + 1, Value(bn - cn));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_mul(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            state->set(a + 1, Value(bn * cn));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_div(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        u32 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            
            if (cn == 0) {
                throw LuaException("attempt to divide by zero");
            }
            
            state->set(a + 1, Value(bn / cn));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_not(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        
        Value bval = state->get(b + 1);
        state->set(a + 1, Value(!bval.asBoolean()));
    }
    
    void VM::op_eq(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        u32 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        bool equal = (bval == cval);
        
        if (equal == (a != 0)) {
            pc++;  // 跳过下一条指令
        }
    }
    
    void VM::op_lt(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        u32 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        bool result;
        
        if (bval.isNumber() && cval.isNumber()) {
            result = bval.asNumber() < cval.asNumber();
        } else if (bval.isString() && cval.isString()) {
            result = bval.asString() < cval.asString();
        } else {
            throw LuaException("attempt to compare incompatible values");
        }
        
        if (result == (a != 0)) {
            pc++;  // 跳过下一条指令
        }
    }
    
    void VM::op_le(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        u32 c = i.getC();
        
        Value bval = state->get(b + 1);
        Value cval = state->get(c + 1);
        
        bool result;
        
        if (bval.isNumber() && cval.isNumber()) {
            result = bval.asNumber() <= cval.asNumber();
        } else if (bval.isString() && cval.isString()) {
            result = bval.asString() <= cval.asString();
        } else {
            throw LuaException("attempt to compare incompatible values");
        }
        
        if (result == (a != 0)) {
            pc++;  // 跳过下一条指令
        }
    }
    
    void VM::op_jmp(Instruction i) {
        int sbx = i.getSBx();
        pc += sbx;
    }
    
    void VM::op_call(Instruction i) {
        u8 a = i.getA();  // 函数寄存器
        u8 b = i.getB();  // 参数数量+1，如果为0则表示使用从a+1到栈顶的所有值
        u8 c = i.getC();  // 期望的返回值数量+1，如果为0表示所有返回值都需要
        
        // 获取函数对象
        Value func = state->get(a + 1);
        
        // 检查是否为函数
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }
        
        // 获取参数数量
        int nargs = (b == 0) ? (state->getTop() - a) : (b - 1);
        
        // 准备参数列表
        Vec<Value> args;
        for (int i = 1; i <= nargs; ++i) {
            args.push_back(state->get(a + 1 + i));
        }
        
        // 调用函数
        try {
            Value result = state->call(func, args);
            
            // 将结果放入a寄存器
            state->set(a + 1, result);
            
            // 清理额外的栈空间(简化处理)
            if (state->getTop() > a + 1) {
                // 保留一个返回值
                for (int i = state->getTop(); i > a + 1; --i) {
                    state->pop();
                }
            }
        } catch (const LuaException& e) {
            throw; // 重新抛出异常
        }
    }
    
    void VM::op_return(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        
        // b-1是要返回的值的数量，如果b=0，表示返回从a开始到栈顶的所有值
        // 简化处理：我们目前只支持最多返回一个值
        if (b == 0 || b > 1) {
            // 返回寄存器a中的值
            // 实际应实现多返回值处理
        }
    }
}
