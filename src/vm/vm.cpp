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
        
        // Save current context
        auto oldFunction = currentFunction;
        auto oldCode = code;
        auto oldConstants = constants;
        auto oldPC = pc;
        
        // Set new context
        currentFunction = function;
        
        // Get function bytecode and constant table
        const auto& functionCode = function->getCode();
        code = const_cast<Vec<Instruction>*>(&functionCode);
        
        const auto& functionConstants = function->getConstants();
        constants = const_cast<Vec<Value>*>(&functionConstants);
        
        pc = 0;
        
        // Execute function
        Value result;
        
        try {
            // Execute bytecode
            while (pc < code->size()) {
                if (!runInstruction()) {
                    break;  // Hit return instruction
                }
            }
            
            // Default return nil
            result = Value(nullptr);
        } catch (const std::exception& e) {
            std::cerr << "VM execution error: " << e.what() << std::endl;

            // Restore old context
            currentFunction = oldFunction;
            code = oldCode;
            constants = oldConstants;
            pc = oldPC;
            
            // Re-throw exception
            throw;
        }
        
        // Restore old context
        currentFunction = oldFunction;
        code = oldCode;
        constants = oldConstants;
        pc = oldPC;
        
        return result;
    }
    
    bool VM::runInstruction() {
        // Get current instruction
        Instruction i = (*code)[pc++];
        
        // Dispatch based on opcode
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
                return false;  // Stop execution
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
    
    // Instruction implementations
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
            pc++;  // Skip next instruction
        }
    }
    
    void VM::op_loadnil(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        
        for (u32 j = a; j <= static_cast<u32>(a + b); j++) {
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
            pc++;  // Skip next instruction
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
            pc++;  // Skip next instruction
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
            pc++;  // Skip next instruction
        }
    }
    
    void VM::op_jmp(Instruction i) {
        int sbx = i.getSBx();
        pc += sbx;
    }
    
    void VM::op_call(Instruction i) {
        u8 a = i.getA();  // Function register
        u8 b = i.getB();  // Number of arguments + 1, if 0 means use all values from a+1 to top
        u8 c = i.getC();  // Expected number of return values + 1, if 0 means all return values needed
        
        // Get function object
        Value func = state->get(a + 1);
        
        // Check if it's a function
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }
        
        // Get number of arguments
        int nargs = (b == 0) ? (state->getTop() - a) : (b - 1);
        
        // Prepare argument list
        Vec<Value> args;
        for (int i = 1; i <= nargs; ++i) {
            args.push_back(state->get(a + 1 + i));
        }
        
        // Call function
        try {
            Value result = state->call(func, args);
            
            // Put result in register a
            state->set(a + 1, result);
            
            // Clean up extra stack space (simplified handling)
            if (state->getTop() > a + 1) {
                // Keep one return value
                for (int i = state->getTop(); i > a + 1; --i) {
                    state->pop();
                }
            }
        } catch (const LuaException& e) {
            std::cerr << "Error during function call: " << e.what() << std::endl;
            throw; // Re-throw exception
        }
    }
    
    void VM::op_return(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        
        // b-1 is the number of values to return, if b=0, return all values from a to top
        // Simplified handling: we currently only support returning at most one value
        if (b == 0 || b > 1) {
            // Return value in register a
            // Should implement multi-return value handling in practice
        }
    }
}
