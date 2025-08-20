#pragma once

#include "../common/types.hpp"
#include "value.hpp"
#include "lua_state.hpp"
#include "global_state.hpp"
#include "function.hpp"
#include "instruction.hpp"

namespace Lua {
    
    /**
     * @brief VM Executor - Central VM execution engine following Lua 5.1 design
     * 
     * This class implements the central VM execution loop based on Lua 5.1's luaV_execute.
     * It provides a single, centralized execution engine that avoids nested VM loops
     * and properly handles recursive function calls using reentry mechanisms.
     */
    class VMExecutor {
    public:
        /**
         * @brief Execute a Lua function using the centralized VM loop
         * @param L Lua state
         * @param func Function to execute
         * @param args Function arguments
         * @return Value Function result
         */
        static Value execute(LuaState* L, GCRef<Function> func, const Vec<Value>& args);

        /**
         * @brief Execute a Lua function in the current execution context without stack manipulation
         * @param L Lua state
         * @param func Function to execute
         * @param args Function arguments
         * @return Value Function result
         */
        static Value executeInContext(LuaState* L, GCRef<Function> func, const Vec<Value>& args);
        
        /**
         * @brief Execute VM instructions starting from current CallInfo
         * @param L Lua state
         * @return Value Execution result
         */
        static Value executeLoop(LuaState* L);
        
    private:
        // Instruction handlers (following Lua 5.1 opcode set)
        static void handleMove(LuaState* L, Instruction instr, Value* base);
        static void handleLoadK(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleLoadBool(LuaState* L, Instruction instr, Value* base, u32& pc);
        static void handleLoadNil(LuaState* L, Instruction instr, Value* base);
        static void handleGetUpval(LuaState* L, Instruction instr, Value* base);
        static void handleSetUpval(LuaState* L, Instruction instr, Value* base);
        static void handleGetGlobal(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleSetGlobal(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleGetTable(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleSetTable(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleNewTable(LuaState* L, Instruction instr, Value* base);
        static void handleSelf(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        
        // Arithmetic operations
        static void handleAdd(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleSub(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleMul(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleDiv(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleMod(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handlePow(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleUnm(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleNot(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleLen(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        static void handleConcat(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants);
        
        // Control flow
        static void handleJmp(LuaState* L, Instruction instr, u32& pc);
        static void handleEq(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc);
        static void handleLt(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc);
        static void handleLe(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc);
        static void handleTest(LuaState* L, Instruction instr, Value* base, u32& pc);
        static void handleTestSet(LuaState* L, Instruction instr, Value* base, u32& pc);
        
        // Function calls and returns
        static bool handleCall(LuaState* L, Instruction instr, Value* base, u32& pc);
        static bool handleTailCall(LuaState* L, Instruction instr, Value* base);
        static bool handleReturn(LuaState* L, Instruction instr, Value* base);
        
        // Closures and upvalues
        static void handleClosure(LuaState* L, Instruction instr, Value* base, const Vec<GCRef<Function>>& prototypes, u32& pc);
        static void handleClose(LuaState* L, Instruction instr, Value* base);

        // Loops
        static void handleForLoop(LuaState* L, Instruction instr, Value* base, u32& pc);
        static void handleForPrep(LuaState* L, Instruction instr, Value* base, u32& pc);
        static void handleTForLoop(LuaState* L, Instruction instr, Value* base, u32& pc);

        // Table operations
        static void handleSetList(LuaState* L, Instruction instr, Value* base);

        // Vararg operations
        static void handleVararg(LuaState* L, Instruction instr, Value* base);
        
        // Helper functions
        static Value* getRK(Value* base, const Vec<Value>& constants, u16 rk);
        static bool isConstant(u16 rk) { return rk & 0x100; }  // 0x100 = 256 = BITRK
        static u16 getConstantIndex(u16 rk) { return rk & 0xFF; }  // 0xFF = 255 = MAXINDEXRK
        
        // Error handling
        static void vmError(LuaState* L, const char* msg);
        static void typeError(LuaState* L, const Value& val, const char* op);
        
        // Debugging support
        static void debugInstruction(LuaState* L, Instruction instr, u32 pc);
    };
    
} // namespace Lua
