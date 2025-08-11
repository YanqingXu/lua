#pragma once

#include "../common/types.hpp"
#include "state.hpp"
#include "function.hpp"
#include "upvalue.hpp"
#include "call_result.hpp"
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

        // --- CallFrame stack (phase 4: memberized) ---
        struct CallFrame {
            int base;
            usize pc;
            GCRef<Function> func;
            int nresults;
        };
        Vec<CallFrame> frameStack;

        // Call depth tracking for nesting boundary checks
        usize callDepth;

        // Vararg support
        Vec<Value> varargs;  // Store varargs for current function
        int varargsCount;    // Number of varargs available
        int actualArgsCount; // Actual number of arguments passed to function

        // Upvalue management
        GCRef<Upvalue> openUpvalues; // Linked list of open upvalues
        Vec<GCRef<Upvalue>> callFrameUpvalues; // Upvalues for current call frame

    public:
        explicit VM(State* state);

        // Execute function (single return value for backward compatibility)
        Value execute(GCRef<Function> function);

        // Execute function with multiple return values support
        CallResult executeMultiple(GCRef<Function> function);

        // Set actual argument count for vararg support
        void setActualArgsCount(int count) { actualArgsCount = count; }

        // GC integration
        void markReferences(GarbageCollector* gc);

        // Public methods for testing
        GCRef<Upvalue> findOrCreateUpvalue(Value* location);
        void closeUpvalues(Value* level);
        void closeAllUpvalues();

        // Metamethod-aware operations
        Value getTableValueMM(const Value& table, const Value& key);
        void setTableValueMM(const Value& table, const Value& key, const Value& value);
        Value callValueMM(const Value& func, const Vec<Value>& args);
        CallResult callValueMMMultiple(const Value& func, const Vec<Value>& args);

        // In-context function call (avoids creating new VM instance)
        CallResult callFunctionInContext(const Value& func, const Vec<Value>& args);

        // Execute function in current VM context (Lua 5.1 style)
        Value executeInContext(GCRef<Function> function, const Vec<Value>& args);
        CallResult executeInContextMultiple(GCRef<Function> function, const Vec<Value>& args);

        // Arithmetic operations with metamethods
        Value performAddMM(const Value& lhs, const Value& rhs);
        Value performSubMM(const Value& lhs, const Value& rhs);
        Value performMulMM(const Value& lhs, const Value& rhs);
        Value performDivMM(const Value& lhs, const Value& rhs);
        Value performModMM(const Value& lhs, const Value& rhs);
        Value performPowMM(const Value& lhs, const Value& rhs);
        Value performUnmMM(const Value& operand);
        Value performConcatMM(const Value& lhs, const Value& rhs);

        // Comparison operations with metamethods
        bool performEqMM(const Value& lhs, const Value& rhs);
        bool performLtMM(const Value& lhs, const Value& rhs);
        bool performLeMM(const Value& lhs, const Value& rhs);

    private:
        // Run one instruction
        bool runInstruction();

        // Get constant
        Value getConstant(u32 idx) const;

        // Register access with frame base
        Value getReg(int reg) const;
        void setReg(int reg, const Value& value);
        Value* getRegPtr(int reg);  // Get pointer to register for upvalue access

        // --- Helpers to consolidate registerBase arithmetic ---
        inline int absReg(int reg) const {
            if (!frameStack.empty()) return frameStack.back().base + reg;
            return registerBase + reg; // fallback during transition
        }
        inline int regEndTop(int startReg, int count) const {
            if (!frameStack.empty()) return frameStack.back().base + startReg + count;
            return registerBase + startReg + count; // fallback during transition
        }

        // --- Frame stack operations (phase 4: memberized) ---
        inline void pushFrame(GCRef<Function> func, int nresults) {
            frameStack.push_back(CallFrame{registerBase, pc, currentFunction, nresults});
            currentFunction = func;
            // code/constants will be set by execute/executeMultiple when needed
        }
        inline void popFrame() {
            if (frameStack.empty()) return; // defensive
            CallFrame cf = frameStack.back();
            frameStack.pop_back();
            registerBase = cf.base;
            pc = cf.pc;
            currentFunction = cf.func;
        }

        // --- Debug assertions (enabled in debug builds) ---
        inline void assertFrameConsistency() const {
        #if defined(_DEBUG) || !defined(NDEBUG)
            if (!frameStack.empty()) {
                int frameBase = frameStack.back().base;
                int viaAbs = absReg(0);
                (void)frameBase;
                (void)viaAbs;
            }
        #endif
        }

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
        void op_vararg(Instruction i);
        void op_closure(Instruction i);
        void op_getupval(Instruction i);
        void op_setupval(Instruction i);

        // Metamethod-aware instruction handlers
        void op_gettable_mm(Instruction i);
        void op_settable_mm(Instruction i);
        void op_call_mm(Instruction i);
        void op_add_mm(Instruction i);
        void op_sub_mm(Instruction i);
        void op_mul_mm(Instruction i);
        void op_div_mm(Instruction i);
        void op_mod_mm(Instruction i);
        void op_pow_mm(Instruction i);
        void op_unm_mm(Instruction i);
        void op_concat_mm(Instruction i);
        void op_eq_mm(Instruction i);
        void op_lt_mm(Instruction i);
        void op_le_mm(Instruction i);
    };
}
