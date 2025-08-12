#include "state_bridge.hpp"
#include "../common/exceptions.hpp"
#include <iostream>

namespace Lua {
    // Global bridge instance
    StateBridge* g_currentStateBridge = nullptr;

    StateBridge::StateBridge(State* oldState) 
        : oldState_(oldState), globalState_(nullptr), luaState_(nullptr), useNewImplementation_(false) {
        if (!oldState_) {
            throw LuaException("StateBridge: oldState cannot be null");
        }
    }

    StateBridge::StateBridge(LuaState* luaState) 
        : oldState_(nullptr), globalState_(nullptr), luaState_(luaState), useNewImplementation_(true) {
        if (!luaState_) {
            throw LuaException("StateBridge: luaState cannot be null");
        }
        globalState_ = luaState_->getGlobalState();
    }

    StateBridge::~StateBridge() {
        cleanup();
    }

    void StateBridge::push(const Value& value) {
        if (useNewImplementation_ && luaState_) {
            luaState_->push(value);
        } else if (oldState_) {
            oldState_->push(value);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    Value StateBridge::pop() {
        if (useNewImplementation_ && luaState_) {
            return luaState_->pop();
        } else if (oldState_) {
            return oldState_->pop();
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    Value& StateBridge::get(int idx) {
        if (useNewImplementation_ && luaState_) {
            // Note: LuaState might have different indexing, need to adapt
            return luaState_->get(idx);
        } else if (oldState_) {
            return oldState_->get(idx);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    void StateBridge::set(int idx, const Value& value) {
        if (useNewImplementation_ && luaState_) {
            luaState_->set(idx, value);
        } else if (oldState_) {
            oldState_->set(idx, value);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    int StateBridge::getTop() const {
        if (useNewImplementation_ && luaState_) {
            return luaState_->getTop();
        } else if (oldState_) {
            return oldState_->getTop();
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    void StateBridge::setTop(int idx) {
        if (useNewImplementation_ && luaState_) {
            luaState_->setTop(idx);
        } else if (oldState_) {
            oldState_->setTop(idx);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    void StateBridge::setGlobal(const Str& name, const Value& value) {
        if (useNewImplementation_ && globalState_) {
            globalState_->setGlobal(name, value);
        } else if (oldState_) {
            oldState_->setGlobal(name, value);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    Value StateBridge::getGlobal(const Str& name) {
        if (useNewImplementation_ && globalState_) {
            return globalState_->getGlobal(name);
        } else if (oldState_) {
            return oldState_->getGlobal(name);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    Value StateBridge::call(const Value& function, const Vec<Value>& args) {
        if (useNewImplementation_ && luaState_) {
            return luaState_->call(function, args);
        } else if (oldState_) {
            return oldState_->call(function, args);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    bool StateBridge::doString(const Str& code) {
        if (useNewImplementation_ && luaState_) {
            return luaState_->doString(code);
        } else if (oldState_) {
            return oldState_->doString(code);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    Value StateBridge::doStringWithResult(const Str& code) {
        if (useNewImplementation_ && luaState_) {
            return luaState_->doStringWithResult(code);
        } else if (oldState_) {
            return oldState_->doStringWithResult(code);
        } else {
            throw LuaException("StateBridge: No valid state available");
        }
    }

    bool StateBridge::isNil(int idx) const {
        if (useNewImplementation_ && luaState_) {
            return luaState_->isNil(idx);
        } else if (oldState_) {
            return oldState_->isNil(idx);
        } else {
            return true; // Default to nil if no state
        }
    }

    bool StateBridge::isBoolean(int idx) const {
        if (useNewImplementation_ && luaState_) {
            return luaState_->isBoolean(idx);
        } else if (oldState_) {
            return oldState_->isBoolean(idx);
        } else {
            return false;
        }
    }

    bool StateBridge::isNumber(int idx) const {
        if (useNewImplementation_ && luaState_) {
            return luaState_->isNumber(idx);
        } else if (oldState_) {
            return oldState_->isNumber(idx);
        } else {
            return false;
        }
    }

    bool StateBridge::isString(int idx) const {
        if (useNewImplementation_ && luaState_) {
            return luaState_->isString(idx);
        } else if (oldState_) {
            return oldState_->isString(idx);
        } else {
            return false;
        }
    }

    bool StateBridge::isFunction(int idx) const {
        if (useNewImplementation_ && luaState_) {
            return luaState_->isFunction(idx);
        } else if (oldState_) {
            return oldState_->isFunction(idx);
        } else {
            return false;
        }
    }

    StateBridge* StateBridge::createWithOldState() {
        State* state = new State();
        return new StateBridge(state);
    }

    StateBridge* StateBridge::createWithNewState() {
        // Create GlobalState first
        GlobalState* globalState = new GlobalState();
        
        // Create LuaState with GlobalState
        LuaState* luaState = globalState->newThread();
        
        return new StateBridge(luaState);
    }

    void StateBridge::initializeOldState() {
        // Old state initialization is handled in State constructor
    }

    void StateBridge::initializeNewState() {
        // New state initialization is handled in LuaState constructor
    }

    void StateBridge::cleanup() {
        // Note: We don't delete the states here as they might be managed elsewhere
        // This is just for cleanup of bridge-specific resources
    }

    // Global bridge management functions
    void initializeStateBridge() {
        if (g_currentStateBridge) {
            cleanupStateBridge();
        }
        
        // Start with old implementation for backward compatibility
        g_currentStateBridge = StateBridge::createWithOldState();
        
        std::cerr << "[StateBridge] Initialized with old State implementation" << std::endl;
    }

    void cleanupStateBridge() {
        if (g_currentStateBridge) {
            delete g_currentStateBridge;
            g_currentStateBridge = nullptr;
        }
    }

    StateBridge* getCurrentStateBridge() {
        return g_currentStateBridge;
    }
}
