#pragma once

#include "../common/types.hpp"
#include "value.hpp"
#include <coroutine>
#include <memory>
#include <vector>
#include <exception>

namespace Lua {
    // Forward declarations
    class State;
    class LuaState;
    
    /**
     * @brief Lua coroutine status enumeration (Lua 5.1 compatible)
     */
    enum class CoroutineStatus : u8 {
        SUSPENDED = 0,  // Coroutine is suspended (can be resumed)
        RUNNING = 1,    // Coroutine is currently running
        NORMAL = 2,     // Coroutine is active but not running (calling another coroutine)
        DEAD = 3        // Coroutine has finished or encountered an error
    };
    
    /**
     * @brief Lua coroutine result structure
     */
    struct CoroutineResult {
        bool success;           // Whether the operation succeeded
        Vec<Value> values;      // Return/yield values
        CoroutineStatus status; // Current coroutine status
        Str errorMessage;       // Error message if failed
        
        CoroutineResult() : success(true), status(CoroutineStatus::SUSPENDED) {}
        CoroutineResult(bool s, CoroutineStatus st) : success(s), status(st) {}
        CoroutineResult(bool s, const Vec<Value>& vals, CoroutineStatus st) 
            : success(s), values(vals), status(st) {}
    };
    
    /**
     * @brief C++20 coroutine promise type for Lua coroutines
     * 
     * This class defines the coroutine promise interface required by C++20
     * coroutines, mapping Lua coroutine semantics to C++ coroutine operations.
     */
    class LuaCoroutinePromise {
    public:
        // C++20 coroutine promise interface
        struct promise_type {
            CoroutineResult result_;
            std::exception_ptr exception_;
            
            // Required by C++20 coroutine interface
            LuaCoroutinePromise get_return_object() {
                return LuaCoroutinePromise{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            
            void unhandled_exception() {
                exception_ = std::current_exception();
            }
            
            // Lua-specific coroutine operations
            std::suspend_always yield_value(const CoroutineResult& result) {
                result_ = result;
                return {};
            }
            
            void return_value(const CoroutineResult& result) {
                result_ = result;
                result_.status = CoroutineStatus::DEAD;
            }
            
            // Get current result
            const CoroutineResult& getCurrentResult() const { return result_; }
            
            // Check if exception occurred
            bool hasException() const { return exception_ != nullptr; }
            void rethrowException() { if (exception_) std::rethrow_exception(exception_); }
        };
        
    private:
        std::coroutine_handle<promise_type> handle_;
        
    public:
        explicit LuaCoroutinePromise(std::coroutine_handle<promise_type> h) : handle_(h) {}
        
        // Move-only type
        LuaCoroutinePromise(const LuaCoroutinePromise&) = delete;
        LuaCoroutinePromise& operator=(const LuaCoroutinePromise&) = delete;
        
        LuaCoroutinePromise(LuaCoroutinePromise&& other) noexcept : handle_(other.handle_) {
            other.handle_ = {};
        }
        
        LuaCoroutinePromise& operator=(LuaCoroutinePromise&& other) noexcept {
            if (this != &other) {
                if (handle_) handle_.destroy();
                handle_ = other.handle_;
                other.handle_ = {};
            }
            return *this;
        }
        
        ~LuaCoroutinePromise() {
            if (handle_) handle_.destroy();
        }
        
        // Coroutine operations
        bool resume() {
            if (!handle_ || handle_.done()) return false;
            handle_.resume();
            return !handle_.done();
        }
        
        bool isDone() const {
            return !handle_ || handle_.done();
        }
        
        CoroutineStatus getStatus() const {
            if (!handle_) return CoroutineStatus::DEAD;
            if (handle_.done()) return CoroutineStatus::DEAD;
            return handle_.promise().getCurrentResult().status;
        }
        
        const CoroutineResult& getResult() const {
            if (!handle_) {
                static CoroutineResult deadResult{false, CoroutineStatus::DEAD};
                return deadResult;
            }
            return handle_.promise().getCurrentResult();
        }
        
        // Check for exceptions
        bool hasException() const {
            return handle_ && handle_.promise().hasException();
        }
        
        void rethrowException() {
            if (handle_) handle_.promise().rethrowException();
        }
    };
    
    /**
     * @brief Lua coroutine wrapper class
     * 
     * This class wraps a C++20 coroutine and provides Lua 5.1 compatible
     * coroutine interface and semantics.
     */
    class LuaCoroutine {
    private:
        UPtr<LuaCoroutinePromise> coroutine_;
        State* parentState_;
        LuaState* luaState_;
        CoroutineStatus status_;
        Vec<Value> lastYieldValues_;
        
    public:
        explicit LuaCoroutine(State* parent, LuaState* luaState);
        ~LuaCoroutine();
        
        // Lua 5.1 coroutine API
        CoroutineResult resume(const Vec<Value>& args);
        CoroutineResult yield(const Vec<Value>& values);
        CoroutineStatus getStatus() const { return status_; }
        
        // State management
        State* getParentState() const { return parentState_; }
        LuaState* getLuaState() const { return luaState_; }
        
        // Execution
        void setCoroutineFunction(UPtr<LuaCoroutinePromise> coro);
        
    private:
        void updateStatus_();
    };
    
    /**
     * @brief Coroutine function signature for Lua functions
     */
    using LuaCoroutineFunction = LuaCoroutinePromise(*)(LuaCoroutine* coro, const Vec<Value>& args);
    
    /**
     * @brief Create a Lua coroutine from a Lua function
     */
    LuaCoroutinePromise createLuaCoroutine(LuaCoroutine* coro, const Vec<Value>& args);
    
    /**
     * @brief Coroutine manager for handling multiple coroutines
     */
    class CoroutineManager {
    private:
        Vec<UPtr<LuaCoroutine>> coroutines_;
        LuaCoroutine* currentCoroutine_;
        
    public:
        CoroutineManager();
        ~CoroutineManager();
        
        // Coroutine lifecycle
        LuaCoroutine* createCoroutine(State* parent, LuaState* luaState);
        void destroyCoroutine(LuaCoroutine* coro);
        
        // Execution management
        void setCurrentCoroutine(LuaCoroutine* coro) { currentCoroutine_ = coro; }
        LuaCoroutine* getCurrentCoroutine() const { return currentCoroutine_; }
        
        // Cleanup
        void cleanup();
    };
}
