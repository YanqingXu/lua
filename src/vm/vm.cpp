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

namespace Lua {
    VM::VM(State* state) : 
        state(state),
        currentFunction(nullptr),
        code(nullptr),
        constants(nullptr),
        pc(0),
        callDepth(0),
        openUpvalues(nullptr) {
    }
    
    Value VM::execute(GCRef<Function> function) {
        if (!function || function->getType() != Function::Type::Lua) {
            throw LuaException("Cannot execute non-Lua function");
        }
        
        // Boundary check: Function nesting depth
        if (callDepth >= MAX_FUNCTION_NESTING_DEPTH) {
            throw LuaException(ERR_NESTING_TOO_DEEP);
        }
        
        // Increment call depth
        ++callDepth;
        
        // Save current context
        auto oldFunction = currentFunction;
        auto oldCode = code;
        auto oldConstants = constants;
        auto oldPC = pc;
        auto oldCallDepth = callDepth - 1;
        
        // Set new context
        currentFunction = function;
        
        // Get function bytecode and constant table
        const auto& functionCode = function->getCode();
        code = const_cast<Vec<Instruction>*>(&functionCode);
        
        const auto& functionConstants = function->getConstants();
        constants = const_cast<Vec<Value>*>(&functionConstants);
        
        pc = 0;
        
        // Clear call frame upvalues for new execution
        callFrameUpvalues.clear();
        
        Value result = Value(nullptr);  // Default return value is nil
        
        try {
            // Execute bytecode
            while (pc < code->size()) {
                if (!runInstruction()) {
                    // Hit return instruction, get return value from stack top
                    if (state->getTop() > 0) {
                        result = state->get(-1);  // Get top value
                        state->pop();  // Remove return value from stack
                    }
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "VM execution error: " << e.what() << std::endl;

            // Close upvalues and clean up before restoring context
            closeAllUpvalues();
            callFrameUpvalues.clear();
            
            // Restore old context
            currentFunction = oldFunction;
            code = oldCode;
            constants = oldConstants;
            pc = oldPC;
            callDepth = oldCallDepth;
            
            // Re-throw exception
            throw;
        }
        
        // Close upvalues when function execution ends
        closeAllUpvalues();
        callFrameUpvalues.clear();
        
        // Restore old context
        currentFunction = oldFunction;
        code = oldCode;
        constants = oldConstants;
        pc = oldPC;
        callDepth = oldCallDepth;
        
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
        state->set(a + 1, make_gc_table());
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
    
    void VM::op_test(Instruction i) {
        u8 a = i.getA();  // Register to test
        u8 c = i.getC();  // Skip next instruction if test fails
        
        Value val = state->get(a + 1);
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
            // Return all values from register a to top
            // For now, just return the value in register a
            if (a < state->getTop()) {
                Value returnValue = state->get(a + 1);  // Get value from register a (1-indexed)
                state->push(returnValue);  // Push return value to stack top
            } else {
                state->push(Value(nullptr));  // Push nil if register is invalid
            }
        } else if (b == 1) {
            // No return values, push nil
            state->push(Value(nullptr));
        } else {
            // Return exactly b-1 values, for now just handle single value
            if (a < state->getTop()) {
                Value returnValue = state->get(a + 1);  // Get value from register a (1-indexed)
                state->push(returnValue);  // Push return value to stack top
            } else {
                state->push(Value(nullptr));  // Push nil if register is invalid
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
            pc++;
            if (pc >= code->size()) break;
            
            Instruction upvalInstr = (*code)[pc];
            u8 isLocal = upvalInstr.getA();
            u8 index = upvalInstr.getB();
            
            GCRef<Upvalue> upvalue;
            
            if (isLocal) {
                // Capture a local variable from the current stack frame
                Value* location = &state->get(index + 1);
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
        state->set(a + 1, Value(closure));
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
        state->set(a + 1, value);
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
        Value value = state->get(a + 1);
        
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
