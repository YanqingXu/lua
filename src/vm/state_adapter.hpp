#pragma once

#include "../common/types.hpp"
#include "lua_state.hpp"
#include "global_state.hpp"
#include "value.hpp"

namespace Lua {
    /**
     * @brief Adapter class for gradual migration from State to LuaState
     * 
     * This class provides a unified interface that can route operations to either
     * the current State implementation or the target LuaState implementation,
     * enabling gradual migration while maintaining full compatibility.
     * 
     * Design principles:
     * - Unified API that works with both implementations
     * - Runtime switching between State and LuaState
     * - Full backward compatibility
     * - Performance monitoring and comparison
     * - Gradual feature migration
     */
    class StateAdapter {
    private:
        // Implementation selection
        State* state_;              // Current State implementation
        LuaState* luaState_;        // Target LuaState implementation
        GlobalState* globalState_;  // Associated GlobalState
        
        // Migration control
        bool useLuaState_;          // Flag to control which implementation to use
        bool ownState_;             // Whether adapter owns the State instance
        bool ownLuaState_;          // Whether adapter owns the LuaState instance
        bool ownGlobalState_;       // Whether adapter owns the GlobalState instance
        
        // Performance monitoring
        mutable usize stateCallCount_;    // Number of calls to State implementation
        mutable usize luaStateCallCount_; // Number of calls to LuaState implementation

    public:
        /**
         * @brief Create adapter with existing State (backward compatibility)
         */
        explicit StateAdapter(State* state, bool takeOwnership = false);
        
        /**
         * @brief Create adapter with existing LuaState (forward compatibility)
         */
        explicit StateAdapter(LuaState* luaState, bool takeOwnership = false);
        
        /**
         * @brief Create adapter with both implementations (migration mode)
         */
        StateAdapter(State* state, LuaState* luaState, bool takeOwnership = false);
        
        /**
         * @brief Create adapter with new instances (fresh start)
         */
        static StateAdapter* createFresh(bool useLuaState = false);
        
        /**
         * @brief Destructor
         */
        ~StateAdapter();

        // Unified stack operations interface
        /**
         * @brief Push value onto stack
         */
        void push(const Value& value);
        
        /**
         * @brief Pop value from stack
         */
        Value pop();
        
        /**
         * @brief Get value at stack index
         */
        Value& get(int idx);
        
        /**
         * @brief Set value at stack index
         */
        void set(int idx, const Value& value);
        
        /**
         * @brief Get current stack top
         */
        int getTop() const;
        
        /**
         * @brief Set stack top
         */
        void setTop(int idx);

        // Unified global variable operations
        /**
         * @brief Set global variable
         */
        void setGlobal(const Str& name, const Value& value);
        
        /**
         * @brief Get global variable
         */
        Value getGlobal(const Str& name);

        // Unified function call operations
        /**
         * @brief Call function with arguments
         */
        Value call(const Value& function, const Vec<Value>& args);

        // Unified code execution
        /**
         * @brief Execute Lua code string
         */
        bool doString(const Str& code);
        
        /**
         * @brief Execute Lua code and return result
         */
        Value doStringWithResult(const Str& code);

        // Type checking operations
        /**
         * @brief Check if value at index is nil
         */
        bool isNil(int idx) const;
        
        /**
         * @brief Check if value at index is boolean
         */
        bool isBoolean(int idx) const;
        
        /**
         * @brief Check if value at index is number
         */
        bool isNumber(int idx) const;
        
        /**
         * @brief Check if value at index is string
         */
        bool isString(int idx) const;
        
        /**
         * @brief Check if value at index is function
         */
        bool isFunction(int idx) const;

        // Migration control
        /**
         * @brief Switch to LuaState implementation
         */
        void enableLuaState(bool enable = true);
        
        /**
         * @brief Check if using LuaState implementation
         */
        bool isUsingLuaState() const { return useLuaState_ && luaState_; }
        
        /**
         * @brief Get current implementation name
         */
        const char* getCurrentImplementation() const;

        // Access to underlying implementations
        /**
         * @brief Get State instance (may be null)
         */
        State* getState() const { return state_; }
        
        /**
         * @brief Get LuaState instance (may be null)
         */
        LuaState* getLuaState() const { return luaState_; }
        
        /**
         * @brief Get GlobalState instance (may be null)
         */
        GlobalState* getGlobalState() const { return globalState_; }

        // Performance monitoring
        /**
         * @brief Get performance statistics
         */
        void getPerformanceStats(usize& stateCallCount, usize& luaStateCallCount) const;
        
        /**
         * @brief Reset performance counters
         */
        void resetPerformanceStats();

        // Utility methods
        /**
         * @brief Validate adapter state
         */
        bool isValid() const;
        
        /**
         * @brief Get adapter status string
         */
        Str getStatusString() const;

    private:
        // Internal helper methods
        void initialize();
        void cleanup();
        void validateState() const;
        
        // Performance tracking
        void incrementStateCallCount() const { ++stateCallCount_; }
        void incrementLuaStateCallCount() const { ++luaStateCallCount_; }
        
        // Non-copyable
        StateAdapter(const StateAdapter&) = delete;
        StateAdapter& operator=(const StateAdapter&) = delete;
    };

    /**
     * @brief Factory functions for creating StateAdapter instances
     */
    namespace StateAdapterFactory {
        /**
         * @brief Create adapter for backward compatibility mode
         */
        StateAdapter* createBackwardCompatible();
        
        /**
         * @brief Create adapter for forward compatibility mode
         */
        StateAdapter* createForwardCompatible();
        
        /**
         * @brief Create adapter for migration testing mode
         */
        StateAdapter* createMigrationTest();
    }
}
