#pragma once

#include "../common/types.hpp"
#include "lua_state.hpp"
#include "register_file.hpp"
#include "function.hpp"
#include "upvalue.hpp"
#include "call_result.hpp"
#include "../gc/core/gc_ref.hpp"

namespace Lua {
    /**
     * @brief Static VM class for Lua execution
     *
     * This class provides static methods for VM execution, following the Lua 5.1
     * design pattern. It operates directly on LuaState objects without maintaining
     * instance state, eliminating the VM-State circular dependency.
     */
    class VM {
    public:
        // Main execution functions (static, no instance state)
        /**
         * @brief Execute Lua code in the given state
         * @param L Lua state to execute in
         */
        static void execute(LuaState* L);

        /**
         * @brief Call function with specified arguments and results
         * @param L Lua state
         * @param nargs Number of arguments
         * @param nresults Number of expected results
         */
        static void call(LuaState* L, i32 nargs, i32 nresults);

        /**
         * @brief Protected call with error handling
         * @param L Lua state
         * @param nargs Number of arguments
         * @param nresults Number of expected results
         * @param errfunc Error function index
         * @return i32 Error code (LUA_OK on success)
         */
        static i32 pcall(LuaState* L, i32 nargs, i32 nresults, i32 errfunc);

        /**
         * @brief Execute a single instruction
         * @param L Lua state
         * @param i Instruction to execute
         */
        static void executeInstruction(LuaState* L, Instruction i);

    private:
        // Register file operations (all static)
        /**
         * @brief Create RegisterFile for LuaState
         * @param L Lua state
         * @return RegisterFile RegisterFile instance
         */
        static RegisterFile createRegisterFile(LuaState* L);

        /**
         * @brief Get register value using RegisterFile
         * @param rf Register file
         * @param reg Register index
         * @return Value& Reference to register value
         */
        static Value& getRegister(RegisterFile& rf, i32 reg);

        /**
         * @brief Set register value using RegisterFile
         * @param rf Register file
         * @param reg Register index
         * @param val Value to set
         */
        static void setRegister(RegisterFile& rf, i32 reg, const Value& val);

        /**
         * @brief Adjust results after function call
         * @param L Lua state
         * @param nresults Expected number of results
         */
        static void adjustResults(LuaState* L, i32 nresults);

        // Instruction handlers (all static, operate on LuaState)
        static void op_move(LuaState* L, Instruction i);
        static void op_loadk(LuaState* L, Instruction i);
        static void op_loadbool(LuaState* L, Instruction i);
        static void op_loadnil(LuaState* L, Instruction i);
        static void op_getglobal(LuaState* L, Instruction i);
        static void op_setglobal(LuaState* L, Instruction i);
        static void op_gettable(LuaState* L, Instruction i);
        static void op_settable(LuaState* L, Instruction i);
        static void op_newtable(LuaState* L, Instruction i);
        static void op_add(LuaState* L, Instruction i);
        static void op_sub(LuaState* L, Instruction i);
        static void op_mul(LuaState* L, Instruction i);
        static void op_div(LuaState* L, Instruction i);
        static void op_mod(LuaState* L, Instruction i);
        static void op_pow(LuaState* L, Instruction i);
        static void op_unm(LuaState* L, Instruction i);
        static void op_not(LuaState* L, Instruction i);
        static void op_len(LuaState* L, Instruction i);
        static void op_concat(LuaState* L, Instruction i);
        static void op_eq(LuaState* L, Instruction i);
        static void op_lt(LuaState* L, Instruction i);
        static void op_le(LuaState* L, Instruction i);
        static void op_jmp(LuaState* L, Instruction i);
        static void op_test(LuaState* L, Instruction i);
        static void op_call(LuaState* L, Instruction i);
        static void op_return(LuaState* L, Instruction i);
        static void op_vararg(LuaState* L, Instruction i);
        static void op_closure(LuaState* L, Instruction i);
        static void op_getupval(LuaState* L, Instruction i);
        static void op_setupval(LuaState* L, Instruction i);

        // Helper functions
        static Value getConstant(LuaState* L, u32 idx);
        static Function* getCurrentFunction(LuaState* L);
        static void setCurrentFunction(LuaState* L, Function* func);

        // Error handling
        static void throwError(LuaState* L, const Str& message);

        // Prevent instantiation (all methods are static)
        VM() = delete;
        VM(const VM&) = delete;
        VM& operator=(const VM&) = delete;
    };
}
