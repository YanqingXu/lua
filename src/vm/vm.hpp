#pragma once

#include "../common/types.hpp"
#include "state.hpp"
#include "function.hpp"
#include "upvalue.hpp"
#include "../gc/core/gc_ref.hpp"

namespace Lua {
    class VM {
    private:
        State* state;
        GCRef<Function> currentFunction;
        Vec<Instruction>* code;
        Vec<Value>* constants;
        usize pc; // Program counter
        int registerBase; // Base of current function's registers (like Lua 5.1 CallInfo.base)

        // Call depth tracking for nesting boundary checks
        usize callDepth;
        
        // Upvalue management
        GCRef<Upvalue> openUpvalues; // Linked list of open upvalues
        Vec<GCRef<Upvalue>> callFrameUpvalues; // Upvalues for current call frame
        
    public:
        explicit VM(State* state);
        
        // Execute function
        Value execute(GCRef<Function> function);
        
        // GC integration
        void markReferences(GarbageCollector* gc);
        
        // Public methods for testing
        GCRef<Upvalue> findOrCreateUpvalue(Value* location);
        void closeUpvalues(Value* level);
        void closeAllUpvalues();
        
    private:
        // Run one instruction
        bool runInstruction();
        
        // Get constant
        Value getConstant(u32 idx) const;

        // Register access with frame base
        Value getReg(int reg) const;
        void setReg(int reg, const Value& value);
        Value* getRegPtr(int reg);  // Get pointer to register for upvalue access
        
        // Handle various opcodes
        void op_move(Instruction i);
        void op_loadk(Instruction i);
        void op_loadbool(Instruction i);
        void op_loadnil(Instruction i);
        void op_getglobal(Instruction i);
        void op_setglobal(Instruction i);
        void op_gettable(Instruction i);
        void op_settable(Instruction i);
        void op_newtable(Instruction i);
        void op_add(Instruction i);
        void op_sub(Instruction i);
        void op_mul(Instruction i);
        void op_div(Instruction i);
        void op_mod(Instruction i);
        void op_pow(Instruction i);
        void op_unm(Instruction i);
        void op_not(Instruction i);
        void op_len(Instruction i);
        void op_concat(Instruction i);
        void op_eq(Instruction i);
        void op_lt(Instruction i);
        void op_le(Instruction i);
        void op_jmp(Instruction i);
        void op_test(Instruction i);
        void op_call(Instruction i);
        void op_return(Instruction i);
        void op_closure(Instruction i);
        void op_getupval(Instruction i);
        void op_setupval(Instruction i);
    };
}
