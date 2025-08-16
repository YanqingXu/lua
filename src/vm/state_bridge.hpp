#pragma once

#include "../common/types.hpp"
#include "lua_state.hpp"
#include "global_state.hpp"

namespace Lua {
    /**
     * @brief Bridge adapter for gradual migration from State to LuaState
     * 
     * This class provides a compatibility layer that allows the current VM
     * implementation to work with both the old State class and the new
     * LuaState/GlobalState architecture during the refactoring process.
     * 
     * Design principles:
     * - Maintain full backward compatibility
     * - Enable gradual migration
     * - Preserve all current functionality
     * - Provide clear migration path
     */
    class StateBridge {
    private:
        // Current implementation (old State)
        State* oldState_;
        
        // New implementation (LuaState + GlobalState)
        GlobalState* globalState_;
        LuaState* luaState_;
        
        // Migration mode flag
        bool useNewImplementation_;

    public:
        /**
         * @brief Construct bridge with old State (backward compatibility)
         */
        explicit StateBridge(State* oldState);
        
        /**
         * @brief Construct bridge with new LuaState (forward compatibility)
         */
        explicit StateBridge(LuaState* luaState);
        
        /**
         * @brief Destructor
         */
        ~StateBridge();

        // Stack operations (unified interface)
        void push(const Value& value);
        Value pop();
        Value& get(int idx);
        void set(int idx, const Value& value);
        int getTop() const;
        void setTop(int idx);

        // Global variables (unified interface)
        void setGlobal(const Str& name, const Value& value);
        Value getGlobal(const Str& name);

        // Function calls (unified interface)
        Value call(const Value& function, const Vec<Value>& args);

        // String operations (unified interface)
        bool doString(const Str& code);
        Value doStringWithResult(const Str& code);

        // Type checking (unified interface)
        bool isNil(int idx) const;
        bool isBoolean(int idx) const;
        bool isNumber(int idx) const;
        bool isString(int idx) const;
        bool isFunction(int idx) const;

        // Access to underlying implementations
        State* getOldState() const { return oldState_; }
        LuaState* getLuaState() const { return luaState_; }
        GlobalState* getGlobalState() const { return globalState_; }
        
        // Migration control
        bool isUsingNewImplementation() const { return useNewImplementation_; }
        void enableNewImplementation(bool enable) { useNewImplementation_ = enable; }

        // Factory methods for creating bridge instances
        static StateBridge* createWithOldState();
        static StateBridge* createWithNewState();

    private:
        // Internal helper methods
        void initializeOldState();
        void initializeNewState();
        void cleanup();
        
        // Non-copyable
        StateBridge(const StateBridge&) = delete;
        StateBridge& operator=(const StateBridge&) = delete;
    };

    /**
     * @brief Global bridge instance for current VM implementation
     * 
     * This allows the current VM code to continue working while we
     * gradually migrate to the new architecture.
     */
    extern StateBridge* g_currentStateBridge;

    /**
     * @brief Initialize global bridge with old State (for backward compatibility)
     */
    void initializeStateBridge();

    /**
     * @brief Cleanup global bridge
     */
    void cleanupStateBridge();

    /**
     * @brief Get current state bridge
     */
    StateBridge* getCurrentStateBridge();
}
