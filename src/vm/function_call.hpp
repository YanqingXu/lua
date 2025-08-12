#pragma once

#include "../common/types.hpp"
#include "lua_state.hpp"
#include "register_file.hpp"
#include "call_stack.hpp"
#include "value.hpp"

namespace Lua {
    
    // Forward declarations
    class LuaState;
    class RegisterFile;
    class CallStack;
    struct Value;
    
    /**
     * @brief Function call optimization module
     * 
     * This module provides optimized function call mechanisms,
     * following Lua 5.1 design patterns with performance enhancements.
     * It handles precall/postcall processing, tail call optimization,
     * and unified call interfaces.
     */
    namespace FunctionCall {
        
        // Call result constants
        constexpr i32 LUA_MULTRET = -1;     // Multiple return values
        constexpr i32 LUA_YIELD = -2;       // Function yielded
        constexpr i32 LUA_ERRRUN = 2;       // Runtime error
        constexpr i32 LUA_ERRMEM = 4;       // Memory allocation error
        constexpr i32 LUA_ERRERR = 5;       // Error in error handler
        
        /**
         * @brief Call status enumeration
         */
        enum class CallStatus {
            Success,        // Call completed successfully
            Yielded,        // Call yielded (coroutine)
            Error,          // Runtime error occurred
            MemoryError,    // Memory allocation error
            ErrorInHandler  // Error in error handler
        };
        
        /**
         * @brief Call result structure
         */
        struct CallResult {
            CallStatus status;      // Call status
            i32 nresults;          // Number of results returned
            Str errorMessage;      // Error message (if any)
            
            CallResult(CallStatus s = CallStatus::Success, i32 n = 0, const Str& msg = "")
                : status(s), nresults(n), errorMessage(msg) {}
            
            bool isSuccess() const { return status == CallStatus::Success; }
            bool isYielded() const { return status == CallStatus::Yielded; }
            bool isError() const { return status != CallStatus::Success && status != CallStatus::Yielded; }
        };
        
        // Main call interface
        /**
         * @brief Execute function call with optimized processing
         * @param L Lua state
         * @param func Function to call (on stack)
         * @param nargs Number of arguments
         * @param nresults Expected number of results
         * @return CallResult Call execution result
         */
        CallResult call(LuaState* L, Value* func, i32 nargs, i32 nresults);
        
        /**
         * @brief Execute protected function call
         * @param L Lua state
         * @param func Function to call (on stack)
         * @param nargs Number of arguments
         * @param nresults Expected number of results
         * @param errfunc Error function index (0 = no error function)
         * @return CallResult Call execution result
         */
        CallResult pcall(LuaState* L, Value* func, i32 nargs, i32 nresults, i32 errfunc = 0);
        
        // Precall processing
        /**
         * @brief Process function call setup
         * @param L Lua state
         * @param func Function to call
         * @param nresults Expected number of results
         * @return i32 1 if Lua function, 0 if C function, -1 if error
         */
        i32 precall(LuaState* L, Value* func, i32 nresults);
        
        /**
         * @brief Process Lua function precall
         * @param L Lua state
         * @param func Lua function to call
         * @param nresults Expected number of results
         * @return i32 1 for Lua function
         */
        i32 precallLua(LuaState* L, Value* func, i32 nresults);
        
        /**
         * @brief Process C function precall
         * @param L Lua state
         * @param func C function to call
         * @param nresults Expected number of results
         * @return i32 0 for C function
         */
        i32 precallC(LuaState* L, Value* func, i32 nresults);
        
        /**
         * @brief Process metamethod precall
         * @param L Lua state
         * @param func Object with __call metamethod
         * @param nresults Expected number of results
         * @return i32 Result of metamethod call
         */
        i32 precallMetamethod(LuaState* L, Value* func, i32 nresults);
        
        // Postcall processing
        /**
         * @brief Process function call cleanup
         * @param L Lua state
         * @param firstResult First result on stack
         * @param nresults Number of results to keep
         */
        void postcall(LuaState* L, Value* firstResult, i32 nresults);
        
        /**
         * @brief Process Lua function postcall
         * @param L Lua state
         * @param firstResult First result on stack
         * @param nresults Number of results to keep
         */
        void postcallLua(LuaState* L, Value* firstResult, i32 nresults);
        
        /**
         * @brief Process C function postcall
         * @param L Lua state
         * @param firstResult First result on stack
         * @param nresults Number of results to keep
         */
        void postcallC(LuaState* L, Value* firstResult, i32 nresults);
        
        // Tail call optimization
        /**
         * @brief Execute tail call optimization
         * @param L Lua state
         * @param func Function to tail call
         * @param nargs Number of arguments
         * @return CallResult Tail call result
         */
        CallResult tailcall(LuaState* L, Value* func, i32 nargs);
        
        /**
         * @brief Check if tail call optimization is possible
         * @param L Lua state
         * @param func Function to check
         * @return bool True if tail call is possible
         */
        bool canTailCall(LuaState* L, Value* func);
        
        /**
         * @brief Prepare tail call by moving arguments
         * @param L Lua state
         * @param func Function position
         * @param nargs Number of arguments
         */
        void prepareTailCall(LuaState* L, Value* func, i32 nargs);
        
        // Stack management helpers
        /**
         * @brief Adjust function arguments on stack
         * @param L Lua state
         * @param func Function position
         * @param nargs Actual number of arguments
         * @param expectedArgs Expected number of arguments
         */
        void adjustArguments(LuaState* L, Value* func, i32 nargs, i32 expectedArgs);
        
        /**
         * @brief Adjust function results on stack
         * @param L Lua state
         * @param firstResult First result position
         * @param actualResults Actual number of results
         * @param expectedResults Expected number of results
         */
        void adjustResults(LuaState* L, Value* firstResult, i32 actualResults, i32 expectedResults);
        
        /**
         * @brief Copy function results to correct position
         * @param L Lua state
         * @param dest Destination position
         * @param src Source position
         * @param nresults Number of results to copy
         */
        void copyResults(LuaState* L, Value* dest, Value* src, i32 nresults);
        
        // Error handling
        /**
         * @brief Handle function call error
         * @param L Lua state
         * @param errorMsg Error message
         * @param errfunc Error function index
         * @return CallResult Error result
         */
        CallResult handleError(LuaState* L, const Str& errorMsg, i32 errfunc = 0);
        
        /**
         * @brief Create error message for call failure
         * @param func Function that failed
         * @param reason Failure reason
         * @return Str Error message
         */
        Str createErrorMessage(Value* func, const Str& reason);
        
        // Performance optimization
        /**
         * @brief Fast path for simple function calls
         * @param L Lua state
         * @param func Function to call
         * @param nargs Number of arguments
         * @param nresults Expected number of results
         * @return CallResult Call result (may fallback to slow path)
         */
        CallResult fastCall(LuaState* L, Value* func, i32 nargs, i32 nresults);
        
        /**
         * @brief Check if fast call path is available
         * @param L Lua state
         * @param func Function to check
         * @return bool True if fast path is available
         */
        bool canUseFastPath(LuaState* L, Value* func);
        
        // Integration with RegisterFile and CallStack
        /**
         * @brief Execute call using RegisterFile abstraction
         * @param rf Register file
         * @param funcReg Function register
         * @param nargs Number of arguments
         * @param nresults Expected number of results
         * @return CallResult Call result
         */
        CallResult callWithRegisterFile(RegisterFile& rf, i32 funcReg, i32 nargs, i32 nresults);
        
        /**
         * @brief Execute call using CallStack management
         * @param L Lua state
         * @param cs Call stack
         * @param func Function to call
         * @param nargs Number of arguments
         * @param nresults Expected number of results
         * @return CallResult Call result
         */
        CallResult callWithCallStack(LuaState* L, CallStack& cs, Value* func, i32 nargs, i32 nresults);
        
        // Debugging and diagnostics
        /**
         * @brief Dump call state for debugging
         * @param L Lua state
         * @param func Function being called
         * @param nargs Number of arguments
         */
        void dumpCallState(LuaState* L, Value* func, i32 nargs);
        
        /**
         * @brief Validate call parameters
         * @param L Lua state
         * @param func Function to validate
         * @param nargs Number of arguments
         * @param nresults Expected number of results
         * @return bool True if parameters are valid
         */
        bool validateCallParameters(LuaState* L, Value* func, i32 nargs, i32 nresults);

    } // namespace FunctionCall

} // namespace Lua
