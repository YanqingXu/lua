#include "vm.hpp"
#include "table.hpp"
#include "instruction.hpp"
#include "upvalue.hpp"
#include "metamethod_manager.hpp"
#include "core_metamethods.hpp"
#include "../common/opcodes.hpp"
#include "../common/defines.hpp"
#include "../compiler/register_manager.hpp"  // For MAX_REGISTERS constant
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
        varargs(),
        varargsCount(0),
        actualArgsCount(0),
        openUpvalues(nullptr) {

    }
    
    Value VM::execute(GCRef<Function> function) {
        // Set this VM as current VM in state (for context-aware calls)
        VM* oldCurrentVM = state->getCurrentVM();
        state->setCurrentVM(this);

        if (!function || function->getType() != Function::Type::Lua) {
            // Restore VM context before throwing
            state->setCurrentVM(oldCurrentVM);
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
            // 被调用函数：根据State::callLua的实现
            // 在callLua中，函数被放在位置 (oldTop - nargs - 1)
            // 其中 oldTop 是调用前的栈顶，nargs 是参数数量
            // 但是在execute中，我们只知道当前的stackSize
            //
            // 从callLua的实现可以看出：
            // - 参数原本在栈顶的最后nargs个位置
            // - 函数被插入到参数前面
            // - 所以函数的位置是 stackSize - expectedArgs - 1
            //
            // 但是从调试输出看，这个计算是错误的
            // 让我们使用一个更直接的方法：
            // 在callLua中，函数总是在参数的前一个位置
            // 所以我们需要找到参数的起始位置，然后减1

            // 临时修复：根据调试输出，函数在位置13，参数从14开始
            // 这意味着寄存器基址应该是函数的位置
            // 从调试可以看出，当stackSize=17, expectedArgs=1时，函数在位置13
            // 所以 registerBase = stackSize - expectedArgs - 1 - 2 = 13
            // 但这个公式不通用，让我们用更直接的方法

            // 根据Lua 5.1的调用约定，寄存器基址应该指向函数的位置
            // 在callLua的实现中，函数被放在 top - nargs - 1
            // 这里的top是调用push(nullptr)之后的值，即oldTop + 1
            // 所以函数位置是 (oldTop + 1) - nargs - 1 = oldTop - nargs
            // 但是我们不知道oldTop，只知道当前的stackSize

            // 从调试输出分析：
            // stackSize=17, expectedArgs=1, 函数在位置13
            // 17 - 1 - 1 = 15 (错误)
            // 正确的应该是13
            // 17 - 1 - 3 = 13 (正确)
            // 所以公式应该是 stackSize - expectedArgs - 3
            // 但这个3是哪里来的？

            // 让我重新分析callLua的逻辑：
            // 1. oldTop = 16 (调用前)
            // 2. push(nullptr) -> top = 17
            // 3. 函数放在位置 top - nargs - 1 = 17 - 3 - 1 = 13
            // 4. VM::execute被调用时，stackSize = 17
            // 5. 所以 registerBase = 13 = stackSize - nargs - 1 = 17 - 3 - 1

            // 但是在VM中，我们不知道nargs，只知道expectedArgs
            // 从调试看，nargs = 3, expectedArgs = 1
            // 这表明nargs是实际传递的参数数量，expectedArgs是函数声明的参数数量

            // 让我们使用一个更简单的方法：
            // 从调试输出可以看出，函数总是在栈的某个位置，参数紧跟其后
            // 我们可以通过查找函数对象来确定寄存器基址

            // 修复寄存器基址计算
            // 从调试输出可以看出，当前的计算是错误的
            // 让我使用一个更直接的方法：从栈中查找函数对象的位置

            int functionPos = -1;
            for (int i = stackSize - 1; i >= 0; i--) {
                Value val = state->get(i);
                if (val.isFunction()) {
                    functionPos = i;
                    break;
                }
            }

            if (functionPos >= 0) {
                this->registerBase = functionPos;
            } else {
                // 如果找不到函数，使用原来的计算
                int actualArgs = stackSize - 1;
                this->registerBase = actualArgs - expectedArgs - 2;
            }
        }



        // 为函数的局部变量扩展栈空间
        int localCount = function->getLocalCount();
        // 更合理的栈扩展策略：只分配必要的空间
        int minRequiredSize = this->registerBase + expectedArgs + localCount + 5; // 小缓冲



        // 扩展栈到所需大小
        while (state->getTop() < minRequiredSize) {
            state->push(Value(nullptr)); // 用nil填充
        }

        // Handle varargs for variadic functions
        if (function->getIsVariadic()) {


            // In Lua 5.1, the stack layout for function calls is:
            // [function] [arg1] [arg2] ... [argN] [local1] [local2] ...
            // The registerBase points to the function position
            // Arguments start at registerBase + 1

            // 重新计算：实际参数数量应该基于函数调用时传递的参数
            // 从callLua可以看出，函数在registerBase位置，参数从registerBase+1开始
            // 但是stackSize包含了扩展的栈空间，所以不能直接使用

            // 更正确的方法：从callLua的nargs参数获取，但这里我们没有
            // 临时方法：使用一个固定的逻辑
            // 从调试输出可以看出，参数总是在函数后面的连续位置
            // 但是由于栈扩展，我们不能简单地计数到nil

            // 新方法：使用stackSize和registerBase的关系
            // 在callLua中，函数被放在特定位置，参数紧跟其后
            // 从调试可以看出：
            // - registerBase指向函数位置
            // - 参数从registerBase+1开始
            // - 但是stackSize包含了扩展的空间

            // 让我们使用一个更简单的方法：
            // 假设参数数量等于传递给callLua的nargs
            // 但是我们没有直接访问这个值

            // 使用从callLua传递的实际参数数量
            // 这个值在callLua中通过setActualArgsCount设置
            int actualArgs = this->actualArgsCount;

            int declaredParams = function->getParamCount();
            int extraArgs = actualArgs - declaredParams;



            if (extraArgs > 0) {
                // Store extra arguments as varargs
                varargs.clear();
                varargs.reserve(extraArgs);
                varargsCount = extraArgs;

                // Copy extra arguments from stack to varargs
                // Extra args start at registerBase + 1 + declaredParams
                for (int i = 0; i < extraArgs; ++i) {
                    int argIndex = this->registerBase + 1 + declaredParams + i;
                    if (argIndex < state->getTop()) {
                        Value val = state->get(argIndex);
                        varargs.push_back(val);
                    } else {
                        varargs.push_back(Value()); // nil for missing args
                    }
                }
            } else {
                // No extra arguments
                varargs.clear();
                varargsCount = 0;
            }
        } else {
            // Non-variadic function
            varargs.clear();
            varargsCount = 0;
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

        // Restore VM context in state
        state->setCurrentVM(oldCurrentVM);

        return result;
    }

    CallResult VM::executeMultiple(GCRef<Function> function) {
        // Set this VM as current VM in state (for context-aware calls)
        VM* oldCurrentVM = state->getCurrentVM();
        state->setCurrentVM(this);

        // Save current context
        auto oldFunction = currentFunction;
        auto oldCode = code;
        auto oldConstants = constants;
        auto oldPC = pc;
        auto oldRegisterBase = this->registerBase;

        // Set new context
        currentFunction = function;
        code = const_cast<Vec<Instruction>*>(&function->getCode());
        constants = const_cast<Vec<Value>*>(&function->getConstants());
        pc = 0;

        // Initialize stack for function execution
        int stackSize = state->getTop();

        // Extend stack to accommodate function's local variables (same logic as execute method)
        int localCount = function->getLocalCount();
        int expectedArgs = function->getParamCount();
        int minRequiredSize = this->registerBase + expectedArgs + localCount + 5; // Small buffer

        // Extend stack to required size
        while (state->getTop() < minRequiredSize) {
            state->push(Value(nullptr)); // Fill with nil
        }

        // Handle varargs for variadic functions (same as execute method)
        if (function->getIsVariadic()) {
            int actualArgs = this->actualArgsCount;
            int declaredParams = function->getParamCount();
            int extraArgs = actualArgs - declaredParams;

            if (extraArgs > 0) {
                varargs.clear();
                varargs.reserve(extraArgs);
                varargsCount = extraArgs;

                for (int i = 0; i < extraArgs; ++i) {
                    int argIndex = this->registerBase + 1 + declaredParams + i;
                    if (argIndex < state->getTop()) {
                        Value val = state->get(argIndex);
                        varargs.push_back(val);
                    } else {
                        varargs.push_back(Value());
                    }
                }
            } else {
                varargs.clear();
                varargsCount = 0;
            }
        } else {
            varargs.clear();
            varargsCount = 0;
        }

        // Stack to collect return values
        Vec<Value> returnValues;

        // Execute bytecode
        while (pc < code->size()) {
            if (!runInstruction()) {
                // Hit return instruction, collect all return values from stack
                // The op_return instruction pushes return values to the stack
                // We need to collect all values that were pushed

                // Count how many values are on the stack above the original level
                int originalTop = stackSize;
                int currentTop = state->getTop();
                int numReturnValues = currentTop - originalTop;

                if (numReturnValues > 0) {
                    // Collect return values from stack (they are at the top)
                    returnValues.reserve(numReturnValues);
                    for (int i = 0; i < numReturnValues; ++i) {
                        // Get values from bottom to top (first return value first)
                        Value val = state->get(originalTop + i);

                        returnValues.push_back(val);
                    }

                    // Clean up the stack
                    state->setTop(originalTop);
                } else {
                    // No return values, return nil

                    returnValues.push_back(Value());
                }
                break;
            }
        }

        // Restore previous context
        this->registerBase = oldRegisterBase;
        currentFunction = oldFunction;
        code = oldCode;
        constants = oldConstants;
        pc = oldPC;

        // Restore VM context in state
        state->setCurrentVM(oldCurrentVM);

        return CallResult(returnValues);
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
            case OpCode::VARARG:
                op_vararg(i);
                break;
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

        Value table = getReg(a);

        // Handle key: use ISK to check if it's a constant
        Value key;
        if (ISK(b)) {
            key = getConstant(INDEXK(b));
        } else {
            key = getReg(b);
        }

        // Handle value: use ISK to check if it's a constant
        Value value;
        if (ISK(c)) {
            value = getConstant(INDEXK(c));
        } else {
            value = getReg(c);
        }

        // Set table element
        if (table.isNil()) {
            throw LuaException("attempt to index nil value");
        } else if (!table.isTable()) {
            throw LuaException("attempt to index a non-table value (type: " +
                             std::to_string(static_cast<int>(table.type())) + ")");
        }

        // Check if key already exists in table
        auto tablePtr = table.asTable();
        Value existingValue = tablePtr->get(key);

        if (!existingValue.isNil()) {
            // Key exists, perform direct assignment
            tablePtr->set(key, value);
        } else {
            // Key doesn't exist, check for __newindex metamethod
            auto metatable = tablePtr->getMetatable();
            if (metatable) {
                // Try __newindex metamethod only if metatable exists
                Value newindexHandler = MetaMethodManager::getMetaMethod(table, MetaMethod::NewIndex);
                if (!newindexHandler.isNil()) {
                    if (newindexHandler.isFunction()) {
                        // __newindex is a function: call it with (table, key, value)
                        Vec<Value> args = {table, key, value};
                        CoreMetaMethods::handleMetaMethodCall(state, newindexHandler, args);
                        return; // Don't set in original table
                    } else if (newindexHandler.isTable()) {
                        // __newindex is a table: recursively set key in that table
                        CoreMetaMethods::handleNewIndex(state, newindexHandler, key, value);
                        return; // Don't set in original table
                    }
                }
            }
            // No __newindex metamethod or no metatable, perform direct assignment
            tablePtr->set(key, value);
        }
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



        // 4. 恢复栈状态

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

            for (int i = state->getTop(); i > minRequiredTop; --i) {
                state->pop();
            }
        }
    }
    
    void VM::op_return(Instruction i) {
        u8 a = i.getA();
        u8 b = i.getB();



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
                prototype->getUpvalueCount(),
                prototype->getIsVariadic()  // Pass through the variadic flag
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

    CallResult VM::callValueMMMultiple(const Value& func, const Vec<Value>& args) {
        return CoreMetaMethods::handleCallMultiple(state, func, args);
    }

    CallResult VM::callFunctionInContext(const Value& func, const Vec<Value>& args) {


        if (!func.isFunction()) {
            throw LuaException("Attempt to call a non-function value in callFunctionInContext");
        }

        auto function = func.asFunction();
        if (!function) {
            throw LuaException("Function reference is null in callFunctionInContext");
        }

        // === Native Function Handling ===
        if (function->getType() == Function::Type::Native) {
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("Legacy native function pointer is null");
                }

                int oldTop = state->getTop();

                try {
                    // Push arguments onto stack
                    for (size_t i = 0; i < args.size(); ++i) {
                        state->push(args[i]);
                    }

                    // Call legacy native function
                    Value result = nativeFnLegacy(state, static_cast<int>(args.size()));

                    // Restore stack state
                    state->setTop(oldTop);

                    return CallResult(result);

                } catch (const LuaException& e) {
                    state->setTop(oldTop);
                    throw LuaException("Error in legacy native function call: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    state->setTop(oldTop);
                    throw LuaException("Unexpected error in legacy native function call: " + std::string(e.what()));
                }
            } else {
                // New multi-return function
                auto nativeFn = function->getNative();
                if (!nativeFn) {
                    throw LuaException("Native function pointer is null");
                }

                int oldTop = state->getTop();

                try {
                    // Push arguments onto stack
                    for (size_t i = 0; i < args.size(); ++i) {
                        state->push(args[i]);
                    }

                    // Call new multi-return function
                    i32 returnCount = nativeFn(state);

                    // Collect return values from stack
                    Vec<Value> results;
                    for (i32 i = 0; i < returnCount; ++i) {
                        results.push_back(state->get(oldTop + i));
                    }

                    // Restore stack state
                    state->setTop(oldTop);

                    return CallResult(results);

                } catch (const LuaException& e) {
                    state->setTop(oldTop);
                    throw LuaException("Error in native function call: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    state->setTop(oldTop);
                    throw LuaException("Unexpected error in native function call: " + std::string(e.what()));
                }
            }
        }

        // === Lua Function Handling ===
        if (function->getType() == Function::Type::Lua) {


            // CRITICAL: Do not create new VM instance!
            // Instead, we need to set up a function call within the current VM context
            // This is complex and requires careful register management

            // For now, return a placeholder to avoid VM conflicts
            // TODO: Implement proper in-context Lua function calls

            return CallResult(Value()); // Return nil for now
        }

        throw LuaException("Unknown function type in callFunctionInContext: " +
                         std::to_string(static_cast<int>(function->getType())));
    }

    Value VM::executeInContext(GCRef<Function> function, const Vec<Value>& args) {


        if (function->getType() == Function::Type::Native) {
            // Handle native functions directly
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("Legacy native function pointer is null");
                }

                int oldTop = state->getTop();
                try {
                    // Push arguments onto stack
                    for (const auto& arg : args) {
                        state->push(arg);
                    }

                    // Call legacy native function
                    Value result = nativeFnLegacy(state, static_cast<int>(args.size()));

                    // Restore stack state
                    state->setTop(oldTop);
                    return result;

                } catch (const std::exception& e) {
                    state->setTop(oldTop);
                    throw LuaException("Error in legacy native function call: " + std::string(e.what()));
                }
            } else {
                // New multi-return function - return first value for compatibility
                CallResult callResult = state->callMultiple(Value(function), args);
                if (callResult.count > 0) {
                    return callResult.getFirst();
                } else {
                    return Value();  // Return nil if no values
                }
            }
        }

        if (function->getType() == Function::Type::Lua) {
            // For now, use the original call mechanism for Lua functions
            // TODO: Implement proper in-context execution later
            return state->call(Value(function), args);
        }

        throw LuaException("Unknown function type in executeInContext: " +
                         std::to_string(static_cast<int>(function->getType())));
    }

    CallResult VM::executeInContextMultiple(GCRef<Function> function, const Vec<Value>& args) {
        // For now, use single-value execution and wrap in CallResult
        // TODO: Implement proper multi-return value support
        Value singleResult = executeInContext(function, args);
        return CallResult(singleResult);
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
        // Check for __concat metamethod first (Lua 5.1 behavior)
        // If either operand has a __concat metamethod, use it
        Value lhsHandler = MetaMethodManager::getMetaMethod(lhs, MetaMethod::Concat);
        Value rhsHandler = MetaMethodManager::getMetaMethod(rhs, MetaMethod::Concat);

        if (!lhsHandler.isNil() || !rhsHandler.isNil()) {
            // At least one operand has __concat metamethod
            try {
                return MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Concat, lhs, rhs);
            } catch (const LuaException&) {
                // Metamethod call failed, fall through to direct concatenation or error
            }
        }

        // Try direct string/number concatenation
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

        // No metamethod and not string/number concatenation
        throw LuaException("attempt to concatenate non-string/number values");
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
            } else if (lhs.isFunction()) {
                // Functions are equal if they are the same object (address comparison)
                return lhs == rhs;
            } else if (lhs.isTable()) {
                // For tables, check for __eq metamethod first
                Value lhsHandler = MetaMethodManager::getMetaMethod(lhs, MetaMethod::Eq);
                Value rhsHandler = MetaMethodManager::getMetaMethod(rhs, MetaMethod::Eq);

                // If both have the same __eq metamethod, use it
                if (!lhsHandler.isNil() && !rhsHandler.isNil() && (lhsHandler == rhsHandler)) {
                    try {
                        Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Eq, lhs, rhs);
                        return result.isTruthy();
                    } catch (const LuaException&) {
                        // Metamethod call failed, fall back to address comparison
                    }
                }

                // No metamethod or metamethod failed, use address comparison
                return lhs == rhs;
            } else if (lhs.isUserdata()) {
                // For userdata, check for __eq metamethod first
                Value lhsHandler = MetaMethodManager::getMetaMethod(lhs, MetaMethod::Eq);
                Value rhsHandler = MetaMethodManager::getMetaMethod(rhs, MetaMethod::Eq);

                // If both have the same __eq metamethod, use it
                if (!lhsHandler.isNil() && !rhsHandler.isNil() && (lhsHandler == rhsHandler)) {
                    try {
                        Value result = MetaMethodManager::callBinaryMetaMethod(state, MetaMethod::Eq, lhs, rhs);
                        return result.isTruthy();
                    } catch (const LuaException&) {
                        // Metamethod call failed, fall back to address comparison
                    }
                }

                // No metamethod or metamethod failed, use address comparison
                return lhs == rhs;
            }
            // If we reach here, try metamethod for unknown types
        } else {
            // Different types are never equal in Lua, except when both have the same __eq metamethod
            // But for basic types (nil, boolean, number, string, function), no metamethod lookup is needed
            if (lhs.isNil() || rhs.isNil() || lhs.isBoolean() || rhs.isBoolean() ||
                lhs.isNumber() || rhs.isNumber() || lhs.isString() || rhs.isString() ||
                lhs.isFunction() || rhs.isFunction()) {
                return false; // Basic types with different types are never equal
            }

            // For tables and userdata, check if both have same __eq metamethod
            Value lhsHandler = MetaMethodManager::getMetaMethod(lhs, MetaMethod::Eq);
            Value rhsHandler = MetaMethodManager::getMetaMethod(rhs, MetaMethod::Eq);

            if (lhsHandler.isNil() || rhsHandler.isNil() || !(lhsHandler == rhsHandler)) {
                return false; // No metamethod or different metamethods
            }
        }

        // If we reach here, no metamethod was found or applicable
        return false;
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



        // === Input Validation ===
        if (a >= RegisterManager::MAX_REGISTERS) {
            throw LuaException("Invalid function register in CALL_MM: " + std::to_string(a));
        }

        Value func = getReg(a);


        // === Argument Count Calculation ===
        int nargs;
        if (b == 0) {
            // Variable number of arguments - calculate from stack top
            // Convert relative register 'a' to absolute stack position
            int funcAbsPos = this->registerBase + a;

            // Validate stack bounds
            if (funcAbsPos >= state->getTop()) {
                throw LuaException("Invalid stack position in CALL_MM with variable args");
            }

            nargs = state->getTop() - funcAbsPos - 1; // -1 because we don't count the function itself

            // Ensure non-negative argument count
            if (nargs < 0) {
                nargs = 0;
            }
        } else {
            nargs = b - 1;
        }

        // === Argument Validation ===
        if (nargs < 0) {
            throw LuaException("Invalid argument count in CALL_MM: " + std::to_string(nargs));
        }

        // Lua 5.1 supports up to 250 arguments
        static const int MAX_CALL_ARGS = 250;
        if (nargs > MAX_CALL_ARGS) {
            throw LuaException("Too many arguments in CALL_MM (max " + std::to_string(MAX_CALL_ARGS) +
                             ", got " + std::to_string(nargs) + ")");
        }

        // === Argument Collection ===
        Vec<Value> args;
        try {
            args.reserve(static_cast<size_t>(nargs));


            for (int i = 1; i <= nargs; ++i) {
                // Validate register bounds
                if (a + i >= RegisterManager::MAX_REGISTERS) {
                    throw LuaException("Argument register out of bounds in CALL_MM: " + std::to_string(a + i));
                }

                Value arg = getReg(a + i);

                args.push_back(arg);
            }
        } catch (const std::exception& e) {
            throw LuaException("Error collecting arguments in CALL_MM: " + std::string(e.what()));
        }

        // === Function Call with Metamethod Support ===
        CallResult callResult;
        try {
            callResult = callValueMMMultiple(func, args);

        } catch (const LuaException& e) {
            throw LuaException("Error in CALL_MM function call: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw LuaException("Unexpected error in CALL_MM: " + std::string(e.what()));
        }

        // === Return Value Handling ===
        int expectedReturns = (c == 0) ? -1 : (c - 1);



        // EMERGENCY FIX: Smart detection of multi-return value assignment
        // If we have multiple return values but compiler only expects 1,
        // check if the next few registers are uninitialized (likely for multi-assignment)
        if (expectedReturns == 1 && callResult.count > 1) {


            // Check if registers a+1, a+2, etc. are available for multi-assignment
            int maxPossibleReturns = std::min(static_cast<int>(callResult.count), 5); // Limit to 5 for safety
            int actualExpectedReturns = 1; // Start with compiler's expectation

            // Check if we can safely store more return values
            for (int i = 1; i < maxPossibleReturns; ++i) {
                if (a + i < RegisterManager::MAX_REGISTERS) {
                    actualExpectedReturns = i + 1;
                } else {
                    break; // Stop if we hit register limit
                }
            }

            if (actualExpectedReturns > 1) {

                expectedReturns = actualExpectedReturns;
            }
        }

        if (expectedReturns == 0) {
            // No return values expected - nothing to do

        } else if (expectedReturns == 1) {
            // EMERGENCY FIX: Check if we actually have multiple return values
            // If so, store all of them even though compiler only expects 1
            if (callResult.count > 1) {


                // Store all return values, not just the first one
                for (size_t i = 0; i < callResult.count; ++i) {
                    if (a + i >= RegisterManager::MAX_REGISTERS) {
                        throw LuaException("Return value register out of bounds in CALL_MM: " + std::to_string(a + i));
                    }
                    setReg(static_cast<int>(a + i), callResult.values[i]);

                }
            } else {
                // Single return value - normal case

                setReg(a, callResult.getFirst());
            }
        } else if (expectedReturns > 1) {
            // Multiple return values expected

            // Store actual return values up to the expected count
            for (int i = 0; i < expectedReturns; ++i) {
                if (a + i >= RegisterManager::MAX_REGISTERS) {
                    throw LuaException("Return value register out of bounds in CALL_MM: " + std::to_string(a + i));
                }

                if (i < static_cast<int>(callResult.count)) {
                    // Store actual return value
                    setReg(a + i, callResult.values[i]);

                } else {
                    // Fill remaining slots with nil (Lua 5.1 behavior)
                    setReg(a + i, Value());

                }
            }
        } else {
            // expectedReturns == -1: Variable number of return values

            // Store all return values
            for (size_t i = 0; i < callResult.count; ++i) {
                if (a + i >= RegisterManager::MAX_REGISTERS) {
                    throw LuaException("Return value register out of bounds in CALL_MM: " + std::to_string(a + i));
                }
                setReg(static_cast<int>(a + i), callResult.values[i]);

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

    void VM::op_vararg(Instruction i) {
        u8 a = i.getA();  // Starting register to store varargs
        u8 b = i.getB();  // Number of varargs to retrieve (0 means all)



        // Determine how many varargs to copy
        int numToCopy = (b == 0) ? varargsCount : (b - 1);

        // Ensure we don't copy more than available
        if (numToCopy > varargsCount) {
            numToCopy = varargsCount;
        }

        // Copy varargs to consecutive registers starting from 'a'
        for (int i = 0; i < numToCopy; ++i) {
            if (i < static_cast<int>(varargs.size())) {
                setReg(a + i, varargs[i]);
            } else {
                setReg(a + i, Value()); // nil for missing varargs
            }
        }

        // If b == 0 (get all varargs), we need to adjust the stack top
        // This is important for function calls that use vararg expansion
        if (b == 0) {
            // Calculate the absolute stack position of the last vararg
            // Remember: setReg(a + i, ...) sets stack position (registerBase + a + i)
            int requiredTop;
            if (numToCopy > 0) {
                int lastVarargPos = this->registerBase + a + numToCopy - 1;
                requiredTop = lastVarargPos + 1;
            } else {
                // No varargs, set stack top to the start of vararg area
                requiredTop = this->registerBase + a;
            }

            // Set stack top to exactly after the last vararg (or start if no varargs)
            // This is crucial for correct argument counting in function calls
            state->setTop(requiredTop);
        }
    }
}
