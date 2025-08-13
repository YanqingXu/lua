#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "value.hpp"
#include "call_result.hpp"
#include "lua_state.hpp"
#include "global_state.hpp"
#include "instruction.hpp"
#include "lua_coroutine.hpp"
#include "debug_info.hpp"
#include <iostream>
#include <memory>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class VM;
    class GlobalState;

    class State : public GCObject {
    private:
        // Lua 5.1 standard architecture components
        UPtr<GlobalState> ownedGlobalState_;  // Owned GlobalState instance
        GlobalState* globalState_;            // GlobalState reference (owned or external)
        LuaState* luaState_;                 // Main thread LuaState instance

        // Legacy compatibility members (deprecated but kept for transition)
        Vec<Value> stack;                    // Legacy stack (unused in new implementation)
        int top;                            // Legacy top (unused in new implementation)
        HashMap<Str, Value> globals;        // Legacy globals (unused in new implementation)
        bool useGlobalState_;               // Always true in new implementation

        // Function call context tracking
        GCRef<Function> currentFunction_;   // Track current function for closure creation

        // Upvalue storage for closures
        HashMap<void*, Vec<Value>> functionUpvalues_;  // Per-function upvalue storage
        HashMap<void*, usize> functionToClosureId_;    // Map function to closure instance ID
        usize nextClosureId_;                          // Next available closure instance ID

        // Migration control
        bool fullyMigrated_;                // Flag indicating complete migration to Lua 5.1 architecture

        // Coroutine state (legacy)
        bool isCoroutine_;                  // Flag indicating if this is a coroutine
        State* parentState_;                // Parent state (for coroutines)
        Vec<State*> childCoroutines_;       // Child coroutines created by this state (legacy)

        // C++20 coroutine infrastructure
        UPtr<CoroutineManager> coroutineManager_;  // C++20 coroutine manager

        // Enhanced error handling and debugging
        UPtr<DebugInfoManager> debugInfo_;         // Debug information manager
        UPtr<DebugCallStack> debugCallStack_;      // Enhanced call stack for debugging
        std::string currentSourceFile_;            // Current source file being executed
        i32 currentSourceLine_;                    // Current source line being executed
        LuaCoroutine* currentCoroutine_;           // Current active coroutine

        // Note: VM is now static, no instance references needed
        
    public:
        // Constructors
        State();                                    // Original constructor (backward compatibility)
        explicit State(GlobalState* globalState);  // New constructor with GlobalState
        ~State();
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Stack operations
        void push(const Value& value);
        Value pop();
        Value& get(int idx);
        void set(int idx, const Value& value);
        Value* getPtr(int idx);  // Get pointer to stack element
        
        // Check stack element types
        bool isNil(int idx) const;
        bool isBoolean(int idx) const;
        bool isNumber(int idx) const;
        bool isString(int idx) const;
        bool isTable(int idx) const;
        bool isFunction(int idx) const;
        
        // Convert stack elements
        LuaBoolean toBoolean(int idx) const;
        LuaNumber toNumber(int idx) const;
        Str toString(int idx) const;
        GCRef<Table> toTable(int idx);
        GCRef<Function> toFunction(int idx);
        
        // Global variable operations
        void setGlobal(const Str& name, const Value& value);
        Value getGlobal(const Str& name);
        
        // Get stack size
        int getTop() const;

        // Set stack size (for clearing stack)
        void setTop(int newTop);
        void clearStack();
        
        // Call function (single return value for backward compatibility)
        Value call(const Value& func, const Vec<Value>& args);

        // Call function with multiple return values support
        CallResult callMultiple(const Value& func, const Vec<Value>& args);

        // VM context-aware function calls (Lua 5.1 style)
        Value callSafe(const Value& func, const Vec<Value>& args);
        CallResult callSafeMultiple(const Value& func, const Vec<Value>& args);

        // Note: VM methods removed - VM is now static

        // Native function call with arguments already on stack (Lua 5.1 design)
        Value callNative(const Value& func, int nargs);

        // Native function call with multiple return values (Lua 5.1 standard)
        i32 callNativeMultiple(const Value& func, int nargs);

        // Lua function call with arguments already on stack
        Value callLua(const Value& func, int nargs);

        // Optimized function call for VM CALL instruction
        Value callOptimized_(const Value& func, u8 a, u8 b, u8 c, Vec<Value>& registers);

        // Optimized Lua function call using register-based approach
        Value callLuaOptimized_(GCRef<Function> function, u8 a, u8 b, u8 c, Vec<Value>& registers);

        // Inline Lua function execution to avoid nested VM loops
        Value executeLuaFunctionInline_(GCRef<Function> function, const Vec<Value>& args);

        // Enhanced error handling and debugging
        DebugInfoManager* getDebugInfo() const { return debugInfo_.get(); }
        DebugCallStack* getDebugCallStack() const { return debugCallStack_.get(); }

        // Source location tracking
        void setCurrentSourceLocation(const std::string& filename, i32 line);
        std::string getCurrentSourceFile() const { return currentSourceFile_; }
        i32 getCurrentSourceLine() const { return currentSourceLine_; }

        // Enhanced exception throwing with context
        void throwError(const std::string& message);
        void throwError(const std::string& message, const std::string& filename, i32 line);
        void throwErrorWithContext(const std::string& message, const std::string& context = "");

        // Call stack management for debugging
        void pushDebugFrame(const std::string& functionName, const std::string& filename = "", i32 line = -1);
        void popDebugFrame();
        void setLocalVariableDebugInfo(const std::string& name, const Value& value);

        // Code execution
        bool doString(const Str& code);
        bool doFile(const Str& filename);

        // Execute code and return result (for REPL)
        Value doStringWithResult(const Str& code);

        // Phase 1 refactoring: GlobalState integration methods
        /**
         * @brief Get associated GlobalState (if any)
         */
        GlobalState* getGlobalState() const { return globalState_; }

        /**
         * @brief Check if using GlobalState
         */
        bool isUsingGlobalState() const { return useGlobalState_ && globalState_; }

        /**
         * @brief Enable/disable GlobalState usage
         */
        void setUseGlobalState(bool use) { useGlobalState_ = use; }

        /**
         * @brief Set GlobalState (for migration)
         */
        void setGlobalState(GlobalState* globalState) {
            globalState_ = globalState;
            useGlobalState_ = (globalState != nullptr);
        }

        // Lua 5.1 architecture access methods
        /**
         * @brief Get LuaState instance for advanced operations
         */
        LuaState* getLuaState() const { return luaState_; }

        /**
         * @brief Get current function for closure creation
         */
        GCRef<Function> getCurrentFunction() const { return currentFunction_; }

        /**
         * @brief Set current function for closure creation
         */
        void setCurrentFunction(GCRef<Function> func) { currentFunction_ = func; }

        // C++20 coroutine methods (Lua 5.1 compatible)
        /**
         * @brief Create a new coroutine using C++20 coroutines
         * @param func Function to run in coroutine
         * @return LuaCoroutine* Pointer to new coroutine
         */
        LuaCoroutine* createCoroutine(GCRef<Function> func);

        /**
         * @brief Resume a C++20 coroutine with arguments
         * @param coro Coroutine to resume
         * @param args Arguments to pass to coroutine
         * @return CoroutineResult Result from coroutine
         */
        CoroutineResult resumeCoroutine(LuaCoroutine* coro, const Vec<Value>& args);

        /**
         * @brief Yield from current C++20 coroutine
         * @param values Values to yield
         * @return CoroutineResult Yield result
         */
        CoroutineResult yieldFromCoroutine(const Vec<Value>& values);

        /**
         * @brief Get coroutine status
         * @param coro Coroutine to check
         * @return CoroutineStatus Current status
         */
        CoroutineStatus getCoroutineStatus(LuaCoroutine* coro);

        /**
         * @brief Check if this state is a coroutine
         * @return bool True if this is a coroutine
         */
        bool isCoroutine() const;

        // Legacy coroutine methods (for backward compatibility)
        State* newCoroutine();  // Legacy method
        Value resumeCoroutine(State* coro, const Vec<Value>& args);  // Legacy method
        bool yieldCoroutine(const Vec<Value>& values);  // Legacy method

    private:
        // Internal migration helpers
        void initializeLua5_1Architecture_();
        void migrateLegacyState_();
        void cleanupLua5_1Architecture_();

        // Stack delegation helpers
        Value* indexToLuaStackAddr_(int idx);
        int luaStackAddrToIndex_(Value* addr);

        // CallInfo optimized function call
        Value callWithCallInfo_(GCRef<Function> function, const Vec<Value>& args);
        Value executeLuaFunctionWithCallInfo_(GCRef<Function> function);
        Value executeInstructionWithCallInfo_(const Instruction& instr, Vec<Value>& registers,
                                             const Vec<Value>& constants, size_t& pc);

        // Closure management
        GCRef<Function> createClosureFromPrototype(GCRef<Function> prototype);
    };
}
