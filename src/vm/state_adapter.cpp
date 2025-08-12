#include "state_adapter.hpp"
#include "../common/exceptions.hpp"
#include <iostream>
#include <sstream>

namespace Lua {

    StateAdapter::StateAdapter(State* state, bool takeOwnership)
        : state_(state), luaState_(nullptr), globalState_(nullptr),
          useLuaState_(false), ownState_(takeOwnership), ownLuaState_(false), ownGlobalState_(false),
          stateCallCount_(0), luaStateCallCount_(0) {
        if (!state_) {
            throw LuaException("StateAdapter: State cannot be null");
        }
        initialize();
    }

    StateAdapter::StateAdapter(LuaState* luaState, bool takeOwnership)
        : state_(nullptr), luaState_(luaState), globalState_(nullptr),
          useLuaState_(true), ownState_(false), ownLuaState_(takeOwnership), ownGlobalState_(false),
          stateCallCount_(0), luaStateCallCount_(0) {
        if (!luaState_) {
            throw LuaException("StateAdapter: LuaState cannot be null");
        }
        globalState_ = luaState_->getGlobalState();
        initialize();
    }

    StateAdapter::StateAdapter(State* state, LuaState* luaState, bool takeOwnership)
        : state_(state), luaState_(luaState), globalState_(nullptr),
          useLuaState_(false), ownState_(takeOwnership), ownLuaState_(takeOwnership), ownGlobalState_(false),
          stateCallCount_(0), luaStateCallCount_(0) {
        if (!state_ && !luaState_) {
            throw LuaException("StateAdapter: At least one implementation must be provided");
        }
        if (luaState_) {
            globalState_ = luaState_->getGlobalState();
        }
        initialize();
    }

    StateAdapter* StateAdapter::createFresh(bool useLuaState) {
        if (useLuaState) {
            // Create new GlobalState and LuaState
            GlobalState* globalState = new GlobalState();
            LuaState* luaState = globalState->newThread();
            StateAdapter* adapter = new StateAdapter(luaState, false);
            adapter->ownGlobalState_ = true;
            return adapter;
        } else {
            // Create new State
            State* state = new State();
            return new StateAdapter(state, true);
        }
    }

    StateAdapter::~StateAdapter() {
        cleanup();
    }

    void StateAdapter::push(const Value& value) {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            luaState_->push(value);
        } else if (state_) {
            incrementStateCallCount();
            state_->push(value);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    Value StateAdapter::pop() {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->pop();
        } else if (state_) {
            incrementStateCallCount();
            return state_->pop();
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    Value& StateAdapter::get(int idx) {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->get(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            return state_->get(idx);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    void StateAdapter::set(int idx, const Value& value) {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            luaState_->set(static_cast<i32>(idx), value);
        } else if (state_) {
            incrementStateCallCount();
            state_->set(idx, value);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    int StateAdapter::getTop() const {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return static_cast<int>(luaState_->getTop());
        } else if (state_) {
            incrementStateCallCount();
            return state_->getTop();
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    void StateAdapter::setTop(int idx) {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            luaState_->setTop(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            state_->setTop(idx);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    void StateAdapter::setGlobal(const Str& name, const Value& value) {
        validateState();

        if (useLuaState_ && globalState_) {
            incrementLuaStateCallCount();
            globalState_->setGlobal(name, value);
        } else if (state_) {
            incrementStateCallCount();
            state_->setGlobal(name, value);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    Value StateAdapter::getGlobal(const Str& name) {
        validateState();

        if (useLuaState_ && globalState_) {
            incrementLuaStateCallCount();
            return globalState_->getGlobal(name);
        } else if (state_) {
            incrementStateCallCount();
            return state_->getGlobal(name);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    Value StateAdapter::call(const Value& function, const Vec<Value>& args) {
        validateState();
        
        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            // TODO: Implement LuaState function call when available
            throw LuaException("StateAdapter: LuaState function call not yet implemented");
        } else if (state_) {
            incrementStateCallCount();
            return state_->call(function, args);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    bool StateAdapter::doString(const Str& code) {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            // For LuaState, we need to implement doString functionality
            // This is a simplified implementation - in a full system, this would
            // involve parsing, compiling, and executing the code

            // For now, fall back to State implementation if available
            if (state_) {
                return state_->doString(code);
            } else {
                throw LuaException("StateAdapter: LuaState doString requires State fallback");
            }
        } else if (state_) {
            incrementStateCallCount();
            return state_->doString(code);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    Value StateAdapter::doStringWithResult(const Str& code) {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            // For LuaState, we need to implement doStringWithResult functionality
            // This is a simplified implementation

            // For now, fall back to State implementation if available
            if (state_) {
                return state_->doStringWithResult(code);
            } else {
                throw LuaException("StateAdapter: LuaState doStringWithResult requires State fallback");
            }
        } else if (state_) {
            incrementStateCallCount();
            return state_->doStringWithResult(code);
        } else {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    bool StateAdapter::isNil(int idx) const {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->isNil(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            return state_->isNil(idx);
        } else {
            return true; // Default to nil if no implementation
        }
    }

    bool StateAdapter::isBoolean(int idx) const {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->isBoolean(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            return state_->isBoolean(idx);
        } else {
            return false;
        }
    }

    bool StateAdapter::isNumber(int idx) const {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->isNumber(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            return state_->isNumber(idx);
        } else {
            return false;
        }
    }

    bool StateAdapter::isString(int idx) const {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->isString(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            return state_->isString(idx);
        } else {
            return false;
        }
    }

    bool StateAdapter::isFunction(int idx) const {
        validateState();

        if (useLuaState_ && luaState_) {
            incrementLuaStateCallCount();
            return luaState_->isFunction(static_cast<i32>(idx));
        } else if (state_) {
            incrementStateCallCount();
            return state_->isFunction(idx);
        } else {
            return false;
        }
    }

    void StateAdapter::enableLuaState(bool enable) {
        if (enable && !luaState_) {
            throw LuaException("StateAdapter: Cannot enable LuaState - no LuaState instance available");
        }
        useLuaState_ = enable;
    }

    const char* StateAdapter::getCurrentImplementation() const {
        if (useLuaState_ && luaState_) {
            return "LuaState";
        } else if (state_) {
            return "State";
        } else {
            return "None";
        }
    }

    void StateAdapter::getPerformanceStats(usize& stateCallCount, usize& luaStateCallCount) const {
        stateCallCount = stateCallCount_;
        luaStateCallCount = luaStateCallCount_;
    }

    void StateAdapter::resetPerformanceStats() {
        stateCallCount_ = 0;
        luaStateCallCount_ = 0;
    }

    bool StateAdapter::isValid() const {
        return (state_ != nullptr) || (luaState_ != nullptr);
    }

    Str StateAdapter::getStatusString() const {
        std::ostringstream oss;
        oss << "StateAdapter[";
        oss << "impl=" << getCurrentImplementation();
        oss << ", state=" << (state_ ? "yes" : "no");
        oss << ", luaState=" << (luaState_ ? "yes" : "no");
        oss << ", globalState=" << (globalState_ ? "yes" : "no");
        oss << ", stateCalls=" << stateCallCount_;
        oss << ", luaStateCalls=" << luaStateCallCount_;
        oss << "]";
        return oss.str();
    }

    void StateAdapter::initialize() {
        // Initialization logic if needed
    }

    void StateAdapter::cleanup() {
        if (ownState_ && state_) {
            delete state_;
            state_ = nullptr;
        }
        
        if (ownLuaState_ && luaState_) {
            delete luaState_;
            luaState_ = nullptr;
        }
        
        if (ownGlobalState_ && globalState_) {
            delete globalState_;
            globalState_ = nullptr;
        }
    }

    void StateAdapter::validateState() const {
        if (!isValid()) {
            throw LuaException("StateAdapter: No valid implementation available");
        }
    }

    // Factory functions
    namespace StateAdapterFactory {
        StateAdapter* createBackwardCompatible() {
            return StateAdapter::createFresh(false);
        }
        
        StateAdapter* createForwardCompatible() {
            return StateAdapter::createFresh(true);
        }
        
        StateAdapter* createMigrationTest() {
            State* state = new State();
            GlobalState* globalState = new GlobalState();
            LuaState* luaState = globalState->newThread();
            
            StateAdapter* adapter = new StateAdapter(state, luaState, true);
            adapter->ownGlobalState_ = true;
            return adapter;
        }
    }

} // namespace Lua
