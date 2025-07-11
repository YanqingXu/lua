﻿#include "vm.hpp"
#include "table.hpp"
#include "instruction.hpp"
#include "upvalue.hpp"
#include "metamethod_manager.hpp"
#include "core_metamethods.hpp"
#include "../common/opcodes.hpp"
#include "../common/defines.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/core/garbage_collector.hpp"
#include <iostream>
#include <stdexcept>
#include <iostream>
#include <cmath>

namespace Lua {
    VM::VM(State* state) : 
        state(state),
        currentFunction(nullptr),
        code(nullptr),
        constants(nullptr),
        pc(0),
        registerBase(0),
        callDepth(0),
        openUpvalues(nullptr) {

    }
    
    Value VM::execute(GCRef<Function> function) {
        if (!function || function->getType() != Function::Type::Lua) {
            throw LuaException("Cannot execute non-Lua function");
        }
        
        // Set current function
        currentFunction = function;
        
        // Get code and constants
        code = const_cast<Vec<Instruction>*>(&function->getCode());
        constants = const_cast<Vec<Value>*>(&function->getConstants());
        
        // Initialize program counter
        pc = 0;
        
        // Create new stack frame for this function
        int expectedArgs = function->getParamCount();
        int stackSize = state->getTop();

        // Debug output disabled

        // Lua 5.1 calling convention:
        // Stack layout: [function] [arg1] [arg2] [arg3] ...
        // Register 0 = function, Register 1 = arg1, Register 2 = arg2, etc.
        //
        // 对于主函数：stackSize=0, expectedArgs=0, registerBase应该=0
        // 对于被调用函数：State::call压入了函数和参数
        int oldRegisterBase = this->registerBase;
        if (stackSize == 0) {
            // 主函数，从栈位置0开始
            this->registerBase = 0;
        } else {
            // 被调用函数，函数在stackSize-1-expectedArgs位置
            this->registerBase = stackSize - 1 - expectedArgs;
        }

        // Debug output disabled

        // 为函数的局部变量扩展栈空间
        int localCount = function->getLocalCount();
        // 简单的栈扩展策略：为函数分配足够的空间
        int minRequiredSize = this->registerBase + localCount + 20; // 保守估计

        // 扩展栈到所需大小
        while (state->getTop() < minRequiredSize) {
            state->push(Value(nullptr)); // 用nil填充
        }



        // Stack initialization complete
        
        Value result = Value(nullptr);  // Default return value is nil
        
        // Execute bytecode
        while (pc < code->size()) {
            if (!runInstruction()) {
                // Hit return instruction, get return value from stack top
                if (state->getTop() > 0) {
                    result = state->get(-1);  // Get top value from stack
                    state->pop();  // Remove return value from stack
                }
                break;
            }
        }

        // Restore previous register base
        this->registerBase = oldRegisterBase;

        return result;
    }
    
    bool VM::runInstruction() {
        // Get current instruction
        Instruction i = (*code)[pc];

        pc++;

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
            case OpCode::GETTABLE:
                op_gettable(i);
                break;
            case OpCode::SETTABLE:
                op_settable(i);
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
            case OpCode::MOD:
                op_mod(i);
                break;
            case OpCode::POW:
                op_pow(i);
                break;
            case OpCode::UNM:
                op_unm(i);
                break;
            case OpCode::NOT:
                op_not(i);
                break;
            case OpCode::LEN:
                op_len(i);
                break;
            case OpCode::CONCAT:
                op_concat(i);
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
            case OpCode::TEST:
                op_test(i);
                break;
            case OpCode::RETURN:
                op_return(i);
                return false;  // Stop execution
            case OpCode::CLOSURE:
                op_closure(i);
                break;
            case OpCode::GETUPVAL:
                op_getupval(i);
                break;
            case OpCode::SETUPVAL:
                op_setupval(i);
                break;

            // Metamethod-aware operations
            case OpCode::GETTABLE_MM:
                op_gettable_mm(i);
                break;
            case OpCode::SETTABLE_MM:
                op_settable_mm(i);
                break;
            case OpCode::CALL_MM:
                op_call_mm(i);
                break;
            case OpCode::ADD_MM:
                op_add_mm(i);
                break;
            case OpCode::SUB_MM:
                op_sub_mm(i);
                break;
            case OpCode::MUL_MM:
                op_mul_mm(i);
                break;
            case OpCode::DIV_MM:
                op_div_mm(i);
                break;
            case OpCode::MOD_MM:
                op_mod_mm(i);
                break;
            case OpCode::POW_MM:
                op_pow_mm(i);
                break;
            case OpCode::UNM_MM:
                op_unm_mm(i);
                break;
            case OpCode::CONCAT_MM:
                op_concat_mm(i);
                break;
            case OpCode::EQ_MM:
                op_eq_mm(i);
                break;
            case OpCode::LT_MM:
                op_lt_mm(i);
                break;
            case OpCode::LE_MM:
                op_le_mm(i);
                break;
            default:
                throw LuaException("Unimplemented opcode: " + std::to_string(static_cast<int>(op)));
        }

        // Debug output disabled for production

        return true;
    }
    
    Value VM::getConstant(u32 idx) const {
        if (idx >= constants->size()) {
            throw LuaException("Invalid constant index");
        }
        return (*constants)[idx];
    }

    Value VM::getReg(int reg) const {
        // Convert VM register (0-based) to stack position using register base
        // Lua官方设计：每个函数有独立的寄存器空间，从0开始
        int stackPos = registerBase + reg;

        Value val = state->get(stackPos);

        // Get register value
        return val;
    }

    void VM::setReg(int reg, const Value& value) {
        // Convert VM register (0-based) to stack position using register base
        int stackPos = registerBase + reg;

        // Set register value

        state->set(stackPos, value);
    }

    Value* VM::getRegPtr(int reg) {
        // Convert VM register (0-based) to stack position using register base
        // State::getPtr expects 1-based index, so we add 1
        return state->getPtr(registerBase + reg + 1);
    }
    
    // Instruction implementations
    void VM::op_move(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();

        // Lua官方设计：指令中的寄存器编号就是0基的
        Value val = getReg(b);

        // Move value between registers

        setReg(a, val);
    }
    
    void VM::op_loadk(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();

        Value constant = getConstant(bx);

        // Load constant to register

        setReg(a, constant);
    }
    
    void VM::op_loadbool(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        setReg(a, Value(b != 0));

        if (c != 0) {
            pc++;  // Skip next instruction
        }
    }
    
    void VM::op_loadnil(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();

        for (u32 j = a; j <= static_cast<u32>(a + b); j++) {
            setReg(j, Value(nullptr));
        }
    }
    
    void VM::op_getglobal(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();

        Value key = getConstant(bx);
        if (!key.isString()) {
            throw LuaException("Global name must be a string");
        }

        Value globalValue = state->getGlobal(key.asString());

        // Get global variable

        setReg(a, globalValue);
    }
    
    void VM::op_setglobal(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();
        
        Value key = getConstant(bx);
        if (!key.isString()) {
            throw LuaException("Global name must be a string");
        }
        
        Value val = getReg(a);
        state->setGlobal(key.asString(), val);
        

    }
    
    void VM::op_gettable(Instruction i) {
        u8 a = i.getA();  // Target register
        u8 b = i.getB();  // Table register
        u8 c = i.getC();  // Key register or constant index

        Value table = getReg(b);

        // Handle key: use ISK to check if it's a constant index
        Value key;
        if (ISK(c)) {
            key = getConstant(INDEXK(c));
        } else {
            key = getReg(c);
        }

        // Get table element
        if (table.isNil()) {
            throw LuaException("attempt to index nil value");
        } else if (!table.isTable()) {
            throw LuaException("attempt to index a non-table value (type: " +
                             std::to_string(static_cast<int>(table.type())) + ")");
        }

        // Try direct table access first
        Value result = table.asTable()->get(key);

        // If result is nil and table has metatable, try __index metamethod
        if (result.isNil()) {
            auto tablePtr = table.asTable();
            auto metatable = tablePtr->getMetatable();
            if (metatable) {
                // Try __index metamethod directly since we already did raw access
                Value indexHandler = MetaMethodManager::getMetaMethod(table, MetaMethod::Index);
                if (!indexHandler.isNil()) {
                    if (indexHandler.isFunction()) {
                        // __index is a function: call it with (table, key)
                        Vec<Value> args = {table, key};
                        result = CoreMetaMethods::handleMetaMethodCall(state, indexHandler, args);
                    } else if (indexHandler.isTable()) {
                        // __index is a table: recursively look up key in that table
                        result = CoreMetaMethods::handleIndex(state, indexHandler, key);
                    }
                }
            }
        }

        setReg(a, result);
    }

    void VM::op_settable(Instruction i) {
        u8 a = i.getA();  // Table register
        u8 b = i.getB();  // Key register or constant index
        u8 c = i.getC();  // Value register or constant index

        // DEBUG: Print SETTABLE instruction details
        std::cout << "=== DEBUG: SETTABLE Instruction ===" << std::endl;
        std::cout << "A (table reg): " << static_cast<int>(a) << std::endl;
        std::cout << "B (key): " << static_cast<int>(b) << std::endl;
        std::cout << "C (value): " << static_cast<int>(c) << std::endl;

        Value table = getReg(a);
        std::cout << "Table value type: " << static_cast<int>(table.type()) << std::endl;

        // Handle key: use ISK to check if it's a constant
        Value key;
        if (ISK(b)) {
            key = getConstant(INDEXK(b));
            std::cout << "Key from constant[" << static_cast<int>(INDEXK(b)) << "] (ISK encoded)" << std::endl;
        } else {
            key = getReg(b);
            std::cout << "Key from register[" << static_cast<int>(b) << "]" << std::endl;
        }
        std::cout << "Key value type: " << static_cast<int>(key.type()) << std::endl;

        // Handle value: use ISK to check if it's a constant
        Value value;
        if (ISK(c)) {
            value = getConstant(INDEXK(c));
            std::cout << "Value from constant[" << static_cast<int>(INDEXK(c)) << "] (ISK encoded)" << std::endl;
        } else {
            value = getReg(c);
            std::cout << "Value from register[" << static_cast<int>(c) << "]" << std::endl;
        }
        std::cout << "Value type: " << static_cast<int>(value.type()) << std::endl;

        // Set table element
        std::cout << "Setting table element..." << std::endl;
        if (table.isNil()) {
            std::cout << "ERROR: Table is nil!" << std::endl;
            throw LuaException("attempt to index nil value");
        } else if (!table.isTable()) {
            std::cout << "ERROR: Not a table, type: " << static_cast<int>(table.type()) << std::endl;
            throw LuaException("attempt to index a non-table value (type: " +
                             std::to_string(static_cast<int>(table.type())) + ")");
        }

        // Check if key already exists in table
        auto tablePtr = table.asTable();
        Value existingValue = tablePtr->get(key);
        std::cout << "Existing value type: " << static_cast<int>(existingValue.type()) << std::endl;

        if (!existingValue.isNil()) {
            // Key exists, perform direct assignment
            std::cout << "Key exists, performing direct assignment" << std::endl;
            tablePtr->set(key, value);
        } else {
            std::cout << "Key doesn't exist, checking for __newindex metamethod" << std::endl;
            // Key doesn't exist, check for __newindex metamethod
            auto metatable = tablePtr->getMetatable();
            if (metatable) {
                std::cout << "Metatable exists, checking __newindex" << std::endl;
                // Try __newindex metamethod only if metatable exists
                Value newindexHandler = MetaMethodManager::getMetaMethod(table, MetaMethod::NewIndex);
                if (!newindexHandler.isNil()) {
                    std::cout << "__newindex metamethod found" << std::endl;
                    if (newindexHandler.isFunction()) {
                        // __newindex is a function: call it with (table, key, value)
                        Vec<Value> args = {table, key, value};
                        CoreMetaMethods::handleMetaMethodCall(state, newindexHandler, args);
                        std::cout << "Called __newindex function" << std::endl;
                        return; // Don't set in original table
                    } else if (newindexHandler.isTable()) {
                        // __newindex is a table: recursively set key in that table
                        CoreMetaMethods::handleNewIndex(state, newindexHandler, key, value);
                        std::cout << "Called __newindex table" << std::endl;
                        return; // Don't set in original table
                    }
                } else {
                    std::cout << "No __newindex metamethod found" << std::endl;
                }
            } else {
                std::cout << "No metatable" << std::endl;
            }
            // No __newindex metamethod or no metatable, perform direct assignment
            std::cout << "Performing direct assignment" << std::endl;
            tablePtr->set(key, value);
        }
        std::cout << "SETTABLE completed successfully" << std::endl;
        std::cout << "=================================" << std::endl;
    }

    void VM::op_newtable(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        // Create new table
        GCRef<Table> table = make_gc_table();
        setReg(a, Value(table));
    }
    
    void VM::op_add(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        // Try basic arithmetic first
        if (lhs.isNumber() && rhs.isNumber()) {
            LuaNumber result = lhs.asNumber() + rhs.asNumber();
            setReg(a, Value(result));
            return;
        }

        // Basic arithmetic failed, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Add, lhs, rhs);
            setReg(a, result);
        } catch (const LuaException&) {
            // No metamethod found, throw appropriate error
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_sub(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value bval = getReg(b);
        Value cval = getReg(c);

        // nil值不能用于算术运算
        if (bval.isNil()) {
            throw LuaException("attempt to perform arithmetic on nil value (left operand)");
        } else if (cval.isNil()) {
            throw LuaException("attempt to perform arithmetic on nil value (right operand)");
        } else if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            setReg(a, Value(bn - cn));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values (types: " +
                             std::to_string(static_cast<int>(bval.type())) + " and " +
                             std::to_string(static_cast<int>(cval.type())) + ")");
        }
    }
    
    void VM::op_mul(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        Value bval = getReg(b);
        Value cval = getReg(c);

        // nil值不能用于算术运算
        if (bval.isNil()) {
            throw LuaException("attempt to perform arithmetic on nil value (left operand)");
        } else if (cval.isNil()) {
            throw LuaException("attempt to perform arithmetic on nil value (right operand)");
        } else if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            LuaNumber result = bn * cn;
            setReg(a, Value(result));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values (types: " +
                             std::to_string(static_cast<int>(bval.type())) + " and " +
                             std::to_string(static_cast<int>(cval.type())) + ")");
        }
    }
    
    void VM::op_div(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        u32 c = i.getC();
        
        Value bval = getReg(b);
        Value cval = getReg(c);

        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();

            if (cn == 0) {
                throw LuaException("attempt to divide by zero");
            }

            setReg(a, Value(bn / cn));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_not(Instruction i) {
        u32 a = i.getA();
        u32 b = i.getB();
        
        Value bval = getReg(b);
        setReg(a, Value(!bval.asBoolean()));
    }
    
    void VM::op_eq(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value bval = getReg(b);
        Value cval = getReg(c);

        // nil值可以与任何值比较，只有两个nil相等
        bool equal;
        if (bval.isNil() && cval.isNil()) {
            equal = true;
        } else if (bval.isNil() || cval.isNil()) {
            equal = false;
        } else {
            equal = (bval == cval);
        }

        setReg(a, Value(equal));
    }
    
    void VM::op_lt(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value bval = getReg(b);
        Value cval = getReg(c);

        // Less than comparison

        bool result;

        // nil值不能用于大小比较
        if (bval.isNil() || cval.isNil()) {
            throw LuaException("attempt to compare nil value");
        } else if (bval.isNumber() && cval.isNumber()) {
            result = bval.asNumber() < cval.asNumber();
        } else if (bval.isString() && cval.isString()) {
            result = bval.asString() < cval.asString();
        } else {
            throw LuaException("attempt to compare incompatible values (types: " +
                             std::to_string(static_cast<int>(bval.type())) + " and " +
                             std::to_string(static_cast<int>(cval.type())) + ")");
        }
        setReg(a, Value(result));
    }
    
    void VM::op_le(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value bval = getReg(b);
        Value cval = getReg(c);

        bool result;

        // nil值不能用于大小比较
        if (bval.isNil() || cval.isNil()) {
            throw LuaException("attempt to compare nil value");
        } else if (bval.isNumber() && cval.isNumber()) {
            result = bval.asNumber() <= cval.asNumber();
        } else if (bval.isString() && cval.isString()) {
            result = bval.asString() <= cval.asString();
        } else {
            throw LuaException("attempt to compare incompatible values (types: " +
                             std::to_string(static_cast<int>(bval.type())) + " and " +
                             std::to_string(static_cast<int>(cval.type())) + ")");
        }
        setReg(a, Value(result));
    }
    
    void VM::op_jmp(Instruction i) {
        int sbx = i.getSBx();
        pc += sbx;
    }
    
    void VM::op_test(Instruction i) {
        u8 a = i.getA();  // Register to test
        u8 c = i.getC();  // Skip next instruction if test fails

        Value val = getReg(a);
        bool isTrue = val.isTruthy();

        // If condition is false and C is 0, or condition is true and C is 1, skip next instruction
        if ((c == 0 && !isTrue) || (c == 1 && isTrue)) {
            pc++;  // Skip next instruction
        }
    }
    
    void VM::op_call(Instruction i) {
        u8 a = i.getA();  // Function register
        u8 b = i.getB();  // Number of arguments + 1, if 0 means use all values from a+1 to top
        u8 c = i.getC();  // Expected number of return values + 1, if 0 means all return values needed

        // Debug output disabled

        // Boundary check: Function nesting depth before call
        if (callDepth >= MAX_FUNCTION_NESTING_DEPTH - 1) {
            throw LuaException(ERR_NESTING_TOO_DEEP);
        }

        // Get function object
        Value func = getReg(a);

        // Get function from register

        // Check if it's a function
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }
        
        // Get number of arguments
        int nargs = (b == 0) ? (state->getTop() - a) : (b - 1);

        // Lua 5.1官方设计：设置正确的栈状态供Native函数访问
        // 1. 保存当前栈状态
        int oldTop = state->getTop();
        int oldBase = registerBase;

        // 2. 将寄存器中的参数复制到栈顶（按Lua 5.1官方约定）
        for (int i = 1; i <= nargs; ++i) {
            int argReg = a + i;  // 参数在寄存器a+1, a+2, ...
            Value arg = getReg(argReg);

            // Push argument to stack

            state->push(arg);  // 将参数push到栈顶
        }

        // 3. 调用函数（根据函数类型选择调用方法）
        Value result;
        if (func.isFunction()) {
            auto function = func.asFunction();
            if (function->getType() == Function::Type::Native) {
                // Native函数调用
                result = state->callNative(func, nargs);
            } else {
                // Lua函数调用
                result = state->callLua(func, nargs);
            }
        } else {
            throw LuaException("attempt to call a non-function value");
        }

        // Debug output disabled

        // 4. 恢复栈状态
        // Debug output disabled
        state->setTop(oldTop);

        // Handle return values based on expected count (c parameter)
        int expectedReturns = (c == 0) ? -1 : (c - 1);  // -1 means all returns needed

        if (expectedReturns == 0) {
            // No return values expected, just clean up
        }
        else if (expectedReturns == 1 || expectedReturns == -1) {
            // Single return value or all returns (simplified to single for now)
            setReg(a, result);
        }
        else {
            // Multiple return values expected
            // For now, we only handle single return value from state->call
            // In a full implementation, state->call would need to return multiple values
            setReg(a, result);

            // Fill remaining expected slots with nil
            for (int i = 1; i < expectedReturns; ++i) {
                setReg(a + i, Value(nullptr));
            }
        }

        // Clean up extra stack space
        // 确保不会清理当前函数的寄存器空间
        int minRequiredTop = registerBase + a + std::max(1, expectedReturns);
        if (state->getTop() > minRequiredTop) {
            // Debug output disabled
            for (int i = state->getTop(); i > minRequiredTop; --i) {
                state->pop();
            }
        }
    }
    
    void VM::op_return(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();

        // Debug output disabled

        // b-1 is the number of values to return, if b=0, return all values from a to top
        if (b == 0) {
            // Return all values from register a to top
            // Calculate how many values to return
            int numValues = state->getTop() - a;
            if (numValues <= 0) {
                // No values to return, push nil
                state->push(Value(nullptr));

            } else {
                // Return all values from register a onwards
                // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
                for (int i = 0; i < numValues; ++i) {
                    Value returnValue = getReg(a + i);
                    state->push(returnValue);

                }
            }
        } else if (b == 1) {
            // No return values, push nil
            state->push(Value(nullptr));

        } else {
            // Return exactly b-1 values
            int numValues = b - 1;
            for (int i = 0; i < numValues; ++i) {
                // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
                Value returnValue = getReg(a + i);  // Get value from register a+i
                state->push(returnValue);  // Push return value to stack top
            }
        }
    }
    
    void VM::op_closure(Instruction i) {
        u8 a = i.getA();  // Target register
        u16 bx = i.getBx(); // Function prototype index


        
        // Get function prototype from current function's prototypes
        if (!currentFunction || currentFunction->getType() != Function::Type::Lua) {
            throw LuaException("CLOSURE instruction outside Lua function");
        }
        
        // Get the prototype from the current function's prototype list
        const Vec<GCRef<Function>>& prototypes = currentFunction->getPrototypes();
        if (bx >= prototypes.size()) {
            throw LuaException("Invalid prototype index in CLOSURE instruction");
        }
        
        GCRef<Function> prototype = prototypes[bx];
        if (!prototype) {
            throw LuaException("Null prototype in CLOSURE instruction");
        }
        
        // Boundary check 1: Upvalue count limit
        if (prototype->getUpvalueCount() > MAX_UPVALUES_PER_CLOSURE) {
            throw LuaException(ERR_TOO_MANY_UPVALUES);
        }
        
        // Boundary check 2: Memory usage estimation
        usize estimatedSize = prototype->estimateMemoryUsage();
        if (estimatedSize > MAX_CLOSURE_MEMORY_SIZE) {
            throw LuaException(ERR_MEMORY_EXHAUSTED);
        }
        
        // Create a new closure that shares code and constants with the prototype
        // but has its own upvalue bindings
        GCRef<Function> closure;
        try {
            closure = Function::createLua(
                std::make_shared<Vec<Instruction>>(prototype->getCode()),  // Share code
                prototype->getConstants(),  // Share constants
                prototype->getPrototypes(), // Share nested prototypes
                prototype->getParamCount(),
                prototype->getLocalCount(),
                prototype->getUpvalueCount()
            );
        } catch (const std::bad_alloc&) {
            throw LuaException(ERR_MEMORY_EXHAUSTED);
        }
        
        // Bind upvalues from the current environment

        for (u32 upvalIndex = 0; upvalIndex < prototype->getUpvalueCount(); upvalIndex++) {
            // Read the next instruction to get upvalue binding info
            // Note: pc was already incremented in runInstruction, so we read from current pc
            if (pc >= code->size()) break;
            
            Instruction upvalInstr = (*code)[pc];
            pc++;  // Advance to next instruction for next iteration
            u8 isLocal = upvalInstr.getA();
            u8 index = upvalInstr.getB();


            
            GCRef<Upvalue> upvalue;
            
            if (isLocal) {
                // Capture a local variable from the current stack frame
                // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
                Value* location = getRegPtr(index);





                upvalue = findOrCreateUpvalue(location);
            } else {
                // Inherit an upvalue from the current function
                if (currentFunction && index < currentFunction->getUpvalueCount()) {
                    // Get upvalue from current function
                    upvalue = currentFunction->getUpvalue(index);
                }
            }
            
            // Set upvalue in the new closure
            if (upvalue) {
                closure->setUpvalue(upvalIndex, upvalue);
            }
        }
        
        // Store the closure in the target register
        // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
        setReg(a, Value(closure));
    }
    
    void VM::op_getupval(Instruction i) {
        u8 a = i.getA();  // Target register
        u8 b = i.getB();  // Upvalue index

        if (!currentFunction || currentFunction->getType() != Function::Type::Lua) {
            throw LuaException("GETUPVAL instruction outside Lua function");
        }

        // Boundary check 1: Valid upvalue index
        if (!currentFunction->isValidUpvalueIndex(b)) {
            throw LuaException(ERR_INVALID_UPVALUE_INDEX);
        }

        GCRef<Upvalue> upvalue = currentFunction->getUpvalue(b);
        if (!upvalue) {
            throw LuaException("Null upvalue in GETUPVAL instruction");
        }

        // Boundary check 2: Upvalue lifecycle validation
        if (!upvalue->isValidForAccess()) {
            throw LuaException(ERR_DESTROYED_UPVALUE);
        }

        // Get the value from the upvalue safely
        Value value;
        try {
            value = upvalue->getSafeValue();
        } catch (const std::runtime_error& e) {
            throw LuaException(e.what());
        }

        // Store in target register
        // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
        setReg(a, value);
    }
    
    void VM::op_setupval(Instruction i) {
        u8 a = i.getA();  // Source register
        u8 b = i.getB();  // Upvalue index
        
        if (!currentFunction || currentFunction->getType() != Function::Type::Lua) {
            throw LuaException("SETUPVAL instruction outside Lua function");
        }
        
        // Boundary check 1: Valid upvalue index
        if (!currentFunction->isValidUpvalueIndex(b)) {
            throw LuaException(ERR_INVALID_UPVALUE_INDEX);
        }
        
        GCRef<Upvalue> upvalue = currentFunction->getUpvalue(b);
        if (!upvalue) {
            throw LuaException("Null upvalue in SETUPVAL instruction");
        }
        
        // Boundary check 2: Upvalue lifecycle validation
        if (!upvalue->isValidForAccess()) {
            throw LuaException(ERR_DESTROYED_UPVALUE);
        }
        
        // Get the value from the source register
        // Lua 5.1官方设计：使用0基索引，直接使用寄存器编号
        Value value = getReg(a);
        
        // Set the value in the upvalue safely
        try {
            upvalue->setValue(value);
        } catch (const std::runtime_error& e) {
            throw LuaException(e.what());
        }
    }
    
    GCRef<Upvalue> VM::findOrCreateUpvalue(Value* location) {
        // Search for existing open upvalue pointing to this location
        Upvalue* current = openUpvalues.get();
        Upvalue* prev = nullptr;
        
        // Walk the linked list to find the upvalue or insertion point
        while (current && current->getStackLocation() > location) {
            prev = current;
            current = current->getNext();
        }
        
        // If we found an existing upvalue for this location, return it
        if (current && current->pointsTo(location)) {
            return GCRef<Upvalue>(current);
        }
        
        // Create a new upvalue
        GCRef<Upvalue> newUpvalue = Upvalue::create(location);
        
        // Insert into the sorted linked list
        newUpvalue->setNext(current);
        if (prev) {
            prev->setNext(newUpvalue.get());
        } else {
            openUpvalues = newUpvalue;
        }
        
        return newUpvalue;
    }
    
    void VM::closeUpvalues(Value* level) {
        // Close all upvalues at or above the given stack level
        while (openUpvalues && openUpvalues->getStackLocation() >= level) {
            Upvalue* upvalue = openUpvalues.get();
            openUpvalues = GCRef<Upvalue>(upvalue->getNext());
            
            // Close the upvalue
            upvalue->close();
            upvalue->setNext(nullptr);
        }
    }
    
    void VM::closeAllUpvalues() {
        // Close all open upvalues
        while (openUpvalues) {
            Upvalue* upvalue = openUpvalues.get();
            openUpvalues = GCRef<Upvalue>(upvalue->getNext());
            
            upvalue->close();
            upvalue->setNext(nullptr);
        }
    }
    
    void VM::op_mod(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        Value bval = getReg(b);
        Value cval = getReg(c);

        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();

            if (cn == 0) {
                throw LuaException("attempt to perform modulo by zero");
            }

            setReg(a, Value(fmod(bn, cn)));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_pow(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();
        
        Value bval = getReg(b);
        Value cval = getReg(c);

        if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            setReg(a, Value(pow(bn, cn)));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values");
        }
    }
    
    void VM::op_unm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();

        Value operand = getReg(b);

        // Try basic arithmetic first
        if (operand.isNumber()) {
            setReg(a, Value(-operand.asNumber()));
            return;
        }

        // Basic arithmetic failed, try metamethod
        try {
            Value result = MetaMethodManager::callUnaryMetaMethod(state, MetaMethod::Unm, operand);
            setReg(a, result);
        } catch (const LuaException&) {
            // No metamethod found, throw appropriate error
            throw LuaException("attempt to perform arithmetic on non-number value");
        }
    }
    
    void VM::op_len(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();

        Value bval = getReg(b);

        if (bval.isNil()) {
            throw LuaException("attempt to get length of nil value");
        } else if (bval.isString()) {
            LuaNumber length = static_cast<LuaNumber>(bval.asString().length());
            setReg(a, Value(length));
        } else if (bval.isTable()) {
            // For tables, get the array part length
            auto table = bval.asTable();
            LuaNumber length = static_cast<LuaNumber>(table->getArraySize());
            setReg(a, Value(length));
        } else {
            throw LuaException("attempt to get length of non-string/table value (type: " +
                             std::to_string(static_cast<int>(bval.type())) + ")");
        }
    }
    
    void VM::op_concat(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        // Try basic string/number concatenation first
        if ((lhs.isString() || lhs.isNumber()) && (rhs.isString() || rhs.isNumber())) {
            // Convert to strings and concatenate
            Str lstr, rstr;

            if (lhs.isString()) {
                lstr = lhs.asString();
            } else {
                LuaNumber num = lhs.asNumber();
                if (num == std::floor(num)) {
                    lstr = std::to_string(static_cast<long long>(num));
                } else {
                    lstr = std::to_string(num);
                }
            }

            if (rhs.isString()) {
                rstr = rhs.asString();
            } else {
                LuaNumber num = rhs.asNumber();
                if (num == std::floor(num)) {
                    rstr = std::to_string(static_cast<long long>(num));
                } else {
                    rstr = std::to_string(num);
                }
            }

            setReg(a, Value(lstr + rstr));
            return;
        }

        // Basic concatenation failed, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Concat, lhs, rhs);
            setReg(a, result);
        } catch (const LuaException&) {
            // No metamethod found, throw appropriate error
            throw LuaException("attempt to concatenate non-string/number values");
        }
    }
    
    void VM::markReferences(GarbageCollector* gc) {
        // Mark current function
        if (currentFunction) {
            gc->markObject(currentFunction.get());
        }

        // Mark all open upvalues
        Upvalue* current = openUpvalues.get();
        while (current) {
            gc->markObject(current);
            current = current->getNext();
        }

        // Mark upvalues in call frame
        for (const auto& upvalue : callFrameUpvalues) {
            if (upvalue) {
                gc->markObject(upvalue.get());
            }
        }
    }

    // === Metamethod-aware Operations Implementation ===

    Value VM::getTableValueMM(const Value& table, const Value& key) {
        return CoreMetaMethods::handleIndex(state, table, key);
    }

    void VM::setTableValueMM(const Value& table, const Value& key, const Value& value) {
        CoreMetaMethods::handleNewIndex(state, table, key, value);
    }

    Value VM::callValueMM(const Value& func, const Vec<Value>& args) {
        return CoreMetaMethods::handleCall(state, func, args);
    }

    // === Arithmetic Operations with Metamethods ===

    Value VM::performAddMM(const Value& lhs, const Value& rhs) {
        // Try direct numeric operation first
        if (lhs.isNumber() && rhs.isNumber()) {
            return Value(lhs.asNumber() + rhs.asNumber());
        }

        // Try metamethod
        return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Add, lhs, rhs);
    }

    Value VM::performSubMM(const Value& lhs, const Value& rhs) {
        // Try direct numeric operation first
        if (lhs.isNumber() && rhs.isNumber()) {
            return Value(lhs.asNumber() - rhs.asNumber());
        }

        // Try metamethod
        return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Sub, lhs, rhs);
    }

    Value VM::performMulMM(const Value& lhs, const Value& rhs) {
        // Try direct numeric operation first
        if (lhs.isNumber() && rhs.isNumber()) {
            return Value(lhs.asNumber() * rhs.asNumber());
        }

        // Try metamethod
        return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Mul, lhs, rhs);
    }

    Value VM::performDivMM(const Value& lhs, const Value& rhs) {
        // Try direct numeric operation first
        if (lhs.isNumber() && rhs.isNumber()) {
            LuaNumber divisor = rhs.asNumber();
            if (divisor == 0) {
                throw LuaException("attempt to divide by zero");
            }
            return Value(lhs.asNumber() / divisor);
        }

        // Try metamethod
        return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Div, lhs, rhs);
    }

    Value VM::performModMM(const Value& lhs, const Value& rhs) {
        // Try direct numeric operation first
        if (lhs.isNumber() && rhs.isNumber()) {
            LuaNumber divisor = rhs.asNumber();
            if (divisor == 0) {
                throw LuaException("attempt to perform modulo by zero");
            }
            return Value(fmod(lhs.asNumber(), divisor));
        }

        // Try metamethod
        return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Mod, lhs, rhs);
    }

    Value VM::performPowMM(const Value& lhs, const Value& rhs) {
        // Try direct numeric operation first
        if (lhs.isNumber() && rhs.isNumber()) {
            return Value(pow(lhs.asNumber(), rhs.asNumber()));
        }

        // Try metamethod
        return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Pow, lhs, rhs);
    }

    Value VM::performUnmMM(const Value& operand) {
        // Try direct numeric operation first
        if (operand.isNumber()) {
            return Value(-operand.asNumber());
        }

        // Try metamethod
        return MetaMethodManager::callUnaryMetaMethod(state, MetaMethod::Unm, operand);
    }

    Value VM::performConcatMM(const Value& lhs, const Value& rhs) {
        // Try direct string/number concatenation first
        if ((lhs.isString() || lhs.isNumber()) && (rhs.isString() || rhs.isNumber())) {
            // Convert to strings and concatenate
            Str lstr, rstr;

            if (lhs.isString()) {
                lstr = lhs.asString();
            } else {
                LuaNumber num = lhs.asNumber();
                if (num == std::floor(num)) {
                    lstr = std::to_string(static_cast<long long>(num));
                } else {
                    lstr = std::to_string(num);
                }
            }

            if (rhs.isString()) {
                rstr = rhs.asString();
            } else {
                LuaNumber num = rhs.asNumber();
                if (num == std::floor(num)) {
                    rstr = std::to_string(static_cast<long long>(num));
                } else {
                    rstr = std::to_string(num);
                }
            }

            return Value(lstr + rstr);
        }

        // Try metamethod, but if not found, throw appropriate error
        try {
            return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Concat, lhs, rhs);
        } catch (const LuaException&) {
            // If no metamethod found, throw concatenation-specific error
            throw LuaException("attempt to concatenate non-string/number values");
        }
    }

    // === Comparison Operations with Metamethods ===

    bool VM::performEqMM(const Value& lhs, const Value& rhs) {
        // Try direct comparison first
        if (lhs.type() == rhs.type()) {
            // Same type, try direct comparison
            if (lhs.isNil()) {
                return true; // nil == nil
            } else if (lhs.isBoolean()) {
                return lhs.asBoolean() == rhs.asBoolean();
            } else if (lhs.isNumber()) {
                return lhs.asNumber() == rhs.asNumber();
            } else if (lhs.isString()) {
                return lhs.asString() == rhs.asString();
            }
            // For tables, functions, userdata, try metamethod
        } else {
            // Different types, only equal if both have same __eq metamethod
            Value lhsHandler = MetaMethodManager::getMetaMethod(lhs, MetaMethod::Eq);
            Value rhsHandler = MetaMethodManager::getMetaMethod(rhs, MetaMethod::Eq);

            if (lhsHandler.isNil() || rhsHandler.isNil() || !(lhsHandler == rhsHandler)) {
                return false; // No metamethod or different metamethods
            }
        }

        // Try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Eq, lhs, rhs);
            return result.isTruthy();
        } catch (const LuaException&) {
            return false; // No metamethod found
        }
    }

    bool VM::performLtMM(const Value& lhs, const Value& rhs) {
        // Try direct comparison first
        if (lhs.type() == rhs.type()) {
            if (lhs.isNumber() && rhs.isNumber()) {
                return lhs.asNumber() < rhs.asNumber();
            } else if (lhs.isString() && rhs.isString()) {
                return lhs.asString() < rhs.asString();
            }
        }

        // Try metamethod
        Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Lt, lhs, rhs);
        return result.isTruthy();
    }

    bool VM::performLeMM(const Value& lhs, const Value& rhs) {
        // Try direct comparison first
        if (lhs.type() == rhs.type()) {
            if (lhs.isNumber() && rhs.isNumber()) {
                return lhs.asNumber() <= rhs.asNumber();
            } else if (lhs.isString() && rhs.isString()) {
                return lhs.asString() <= rhs.asString();
            }
        }

        // Try metamethod
        Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Le, lhs, rhs);
        return result.isTruthy();
    }

    // === Metamethod-aware Instruction Handlers ===

    void VM::op_gettable_mm(Instruction i) {
        u8 a = i.getA();  // Target register
        u8 b = i.getB();  // Table register
        u8 c = i.getC();  // Key register or constant index

        Value table = getReg(b);

        // Handle key: if c >= 256, it's a constant index (c-256)
        Value key;
        if (c >= 256) {
            key = getConstant(c - 256);
        } else {
            key = getReg(c);
        }

        Value result = getTableValueMM(table, key);
        setReg(a, result);
    }

    void VM::op_settable_mm(Instruction i) {
        u8 a = i.getA();  // Table register
        u8 b = i.getB();  // Key register or constant index
        u8 c = i.getC();  // Value register or constant index

        Value table = getReg(a);

        // Handle key: if b >= 256, it's a constant index (b-256)
        Value key;
        if (b >= 256) {
            key = getConstant(b - 256);
        } else {
            key = getReg(b);
        }

        // Handle value: if c >= 256, it's a constant index (c-256)
        Value value;
        if (c >= 256) {
            value = getConstant(c - 256);
        } else {
            value = getReg(c);
        }

        setTableValueMM(table, key, value);
    }

    void VM::op_call_mm(Instruction i) {
        u8 a = i.getA();  // Function register
        u8 b = i.getB();  // Number of arguments + 1
        u8 c = i.getC();  // Expected number of return values + 1

        Value func = getReg(a);

        // Get arguments
        int nargs = (b == 0) ? (state->getTop() - a) : (b - 1);
        Vec<Value> args;
        args.reserve(nargs);

        for (int i = 1; i <= nargs; ++i) {
            args.push_back(getReg(a + i));
        }

        // Call with metamethod support
        Value result = callValueMM(func, args);

        // Handle return values
        int expectedReturns = (c == 0) ? -1 : (c - 1);
        if (expectedReturns == 0) {
            // No return values expected
        } else {
            // Store result in function register
            setReg(a, result);

            // Fill remaining expected slots with nil
            for (int i = 1; i < expectedReturns; ++i) {
                setReg(a + i, Value());
            }
        }
    }

    void VM::op_add_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performAddMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_sub_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performSubMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_mul_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performMulMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_div_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performDivMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_mod_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performModMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_pow_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performPowMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_unm_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();

        Value operand = getReg(b);

        Value result = performUnmMM(operand);
        setReg(a, result);
    }

    void VM::op_concat_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        Value result = performConcatMM(lhs, rhs);
        setReg(a, result);
    }

    void VM::op_eq_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        bool result = performEqMM(lhs, rhs);
        setReg(a, Value(result));
    }

    void VM::op_lt_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        bool result = performLtMM(lhs, rhs);
        setReg(a, Value(result));
    }

    void VM::op_le_mm(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        Value lhs = getReg(b);
        Value rhs = getReg(c);

        bool result = performLeMM(lhs, rhs);
        setReg(a, Value(result));
    }
}
