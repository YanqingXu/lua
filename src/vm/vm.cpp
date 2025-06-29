#include "vm.hpp"
#include "table.hpp"
#include "instruction.hpp"
#include "upvalue.hpp"
#include "../common/opcodes.hpp"
#include "../common/defines.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/core/garbage_collector.hpp"
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



        // 调试：显示栈内容
        // std::cout << "[DEBUG] VM::execute stackSize=" << stackSize
        //           << ", expectedArgs=" << expectedArgs
        //           << ", registerBase=" << this->registerBase << std::endl;
        // for (int i = 0; i < stackSize; ++i) {
        //     Value val = state->get(i);
        //     std::cout << "[DEBUG] Stack[" << i << "] = " << val.toString()
        //               << " (type=" << (int)val.type() << ")" << std::endl;
        // }
        
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

#ifdef DEBUG_VM_REGISTERS
        std::cout << "[DEBUG] getReg: reg=" << reg
                  << ", stackPos=" << stackPos
                  << ", registerBase=" << registerBase
                  << ", value_type=" << (int)val.type() << std::endl;
#endif
        return val;
    }

    void VM::setReg(int reg, const Value& value) {
        // Convert VM register (0-based) to stack position using register base
        int stackPos = registerBase + reg;

        // Debug output disabled

#ifdef DEBUG_VM_REGISTERS
        std::cout << "[DEBUG] setReg: reg=" << reg
                  << ", stackPos=" << stackPos
                  << ", registerBase=" << registerBase
                  << ", value_type=" << (int)value.type() << std::endl;
#endif

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

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] MOVE: from_reg=" << (int)b
                  << " to_reg=" << (int)a
                  << ", value_type=" << (int)val.type()
                  << ", value=" << val.toString() << std::endl;
#endif

        setReg(a, val);
    }
    
    void VM::op_loadk(Instruction i) {
        u8 a = i.getA();
        u16 bx = i.getBx();

        Value constant = getConstant(bx);

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] LOADK: reg=" << (int)a
                  << ", constIdx=" << bx
                  << ", value_type=" << (int)constant.type()
                  << ", value=" << constant.toString() << std::endl;
#endif

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

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] GETGLOBAL: key='" << key.asString()
                  << "', reg=" << (int)a
                  << ", value_type=" << (int)globalValue.type()
                  << ", is_function=" << globalValue.isFunction() << std::endl;
#endif

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
        u8 c = i.getC();  // Key register

        Value table = getReg(b);
        Value key = getReg(c);

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] GETTABLE: table_reg=" << (int)b
                  << ", key_reg=" << (int)c
                  << ", table_type=" << (int)table.type()
                  << ", key_type=" << (int)key.type() << std::endl;
#endif

        if (table.isNil()) {
            throw LuaException("attempt to index nil value");
        } else if (!table.isTable()) {
            throw LuaException("attempt to index a non-table value (type: " +
                             std::to_string(static_cast<int>(table.type())) + ")");
        }

        Value result = table.asTable()->get(key);
        setReg(a, result);
    }

    void VM::op_settable(Instruction i) {
        u8 a = i.getA();  // Table register
        u8 b = i.getB();  // Key register
        u8 c = i.getC();  // Value register

        Value table = getReg(a);
        Value key = getReg(b);
        Value value = getReg(c);

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] SETTABLE: table_reg=" << (int)a
                  << ", key_reg=" << (int)b
                  << ", value_reg=" << (int)c
                  << ", table_type=" << (int)table.type()
                  << ", key_type=" << (int)key.type()
                  << ", value_type=" << (int)value.type() << std::endl;
#endif

        if (table.isNil()) {
            throw LuaException("attempt to index nil value");
        } else if (!table.isTable()) {
            throw LuaException("attempt to index a non-table value (type: " +
                             std::to_string(static_cast<int>(table.type())) + ")");
        }

        table.asTable()->set(key, value);
    }

    void VM::op_newtable(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] NEWTABLE: reg=" << (int)a << std::endl;
#endif
        GCRef<Table> table = make_gc_table();
        setReg(a, Value(table));

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] NEWTABLE: created table, type=" << (int)Value(table).type() << std::endl;
#endif
    }
    
    void VM::op_add(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();
        u8 c = i.getC();

        // std::cout << "[DEBUG] ADD instruction: a=" << (int)a << ", b=" << (int)b << ", c=" << (int)c
        //           << ", registerBase=" << registerBase << std::endl;

        Value bval = getReg(b);
        Value cval = getReg(c);

        // std::cout << "[DEBUG] ADD values: bval=" << bval.toString() << " (type=" << (int)bval.type()
        //           << "), cval=" << cval.toString() << " (type=" << (int)cval.type() << ")" << std::endl;

        // nil值不能用于算术运算
        if (bval.isNil()) {
            std::cerr << "[DEBUG] ADD: Left operand is nil, reg=" << (int)b << std::endl;
            throw LuaException("attempt to perform arithmetic on nil value (left operand)");
        } else if (cval.isNil()) {
            std::cerr << "[DEBUG] ADD: Right operand is nil, reg=" << (int)c << std::endl;
            throw LuaException("attempt to perform arithmetic on nil value (right operand)");
        } else if (bval.isNumber() && cval.isNumber()) {
            LuaNumber bn = bval.asNumber();
            LuaNumber cn = cval.asNumber();
            LuaNumber result = bn + cn;
            setReg(a, Value(result));
        } else {
            throw LuaException("attempt to perform arithmetic on non-number values (types: " +
                             std::to_string(static_cast<int>(bval.type())) + " and " +
                             std::to_string(static_cast<int>(cval.type())) + ")");
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

        // std::cout << "[DEBUG] LT: a=" << (int)a << ", b=" << (int)b << ", c=" << (int)c
        //           << ", bval=" << bval.toString() << ", cval=" << cval.toString() << std::endl;

        bool result;

        // nil值不能用于大小比较
        if (bval.isNil() || cval.isNil()) {
            throw LuaException("attempt to compare nil value");
        } else if (bval.isNumber() && cval.isNumber()) {
            result = bval.asNumber() < cval.asNumber();
            // std::cout << "[DEBUG] LT result: " << bval.asNumber() << " < " << cval.asNumber() << " = " << result << std::endl;
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
            std::cerr << "[DEBUG] LE: Comparing with nil value, reg_b=" << (int)b
                      << " (nil=" << bval.isNil() << "), reg_c=" << (int)c
                      << " (nil=" << cval.isNil() << ")" << std::endl;
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

        // std::cout << "[DEBUG] TEST: a=" << (int)a << ", c=" << (int)c
        //           << ", val=" << val.toString() << ", isTrue=" << isTrue << std::endl;

        // If condition is false and C is 0, or condition is true and C is 1, skip next instruction
        if ((c == 0 && !isTrue) || (c == 1 && isTrue)) {
            // std::cout << "[DEBUG] TEST: Skipping next instruction" << std::endl;
            pc++;  // Skip next instruction
        } else {
            // std::cout << "[DEBUG] TEST: Not skipping" << std::endl;
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

#ifdef DEBUG_VM_INSTRUCTIONS
        std::cout << "[DEBUG] CALL: reg=" << (int)a
                  << ", value_type=" << (int)func.type()
                  << ", is_function=" << func.isFunction() << std::endl;
#endif

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

            // std::cout << "[DEBUG] CALL ARG: i=" << i << ", argReg=" << argReg
            //           << ", value_type=" << (int)arg.type()
            //           << ", value=" << arg.toString() << std::endl;

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
        
        Value bval = getReg(b);

        if (bval.isNumber()) {
            setReg(a, Value(-bval.asNumber()));
        } else {
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

        Value bval = getReg(b);
        Value cval = getReg(c);

        // Debug output disabled

        // Convert values to strings and concatenate
        Str bstr, cstr;

        // 处理左操作数
        if (bval.isNil()) {
            throw LuaException("attempt to concatenate nil value (left operand)");
        } else if (bval.isString()) {
            bstr = bval.asString();
        } else if (bval.isNumber()) {
            // Format number as integer if it's a whole number, otherwise as float
            LuaNumber num = bval.asNumber();
            if (num == std::floor(num)) {
                bstr = std::to_string(static_cast<long long>(num));
            } else {
                bstr = std::to_string(num);
            }
        } else {
            throw LuaException("attempt to concatenate non-string/number value (left operand type: " +
                             std::to_string(static_cast<int>(bval.type())) + ")");
        }

        // 处理右操作数
        if (cval.isNil()) {
            throw LuaException("attempt to concatenate nil value (right operand)");
        } else if (cval.isString()) {
            cstr = cval.asString();
        } else if (cval.isNumber()) {
            // Format number as integer if it's a whole number, otherwise as float
            LuaNumber num = cval.asNumber();
            if (num == std::floor(num)) {
                cstr = std::to_string(static_cast<long long>(num));
            } else {
                cstr = std::to_string(num);
            }
        } else {
            throw LuaException("attempt to concatenate non-string/number value (right operand type: " +
                             std::to_string(static_cast<int>(cval.type())) + ")");
        }

        Str result = bstr + cstr;
        setReg(a, Value(result));
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
}
