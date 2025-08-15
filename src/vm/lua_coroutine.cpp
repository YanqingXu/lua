#include "lua_coroutine.hpp"
#include "lua_state.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include "../gc/core/garbage_collector.hpp"
#include <iostream>

namespace Lua {
    
    // LuaCoroutine implementation
    LuaCoroutine::LuaCoroutine(State* parent, LuaState* luaState)
        : GCObject(GCObjectType::Thread, sizeof(LuaCoroutine))
        , parentState_(parent), luaState_(luaState), status_(CoroutineStatus::SUSPENDED) {
    }
    
    LuaCoroutine::~LuaCoroutine() {
        // Cleanup is handled by unique_ptr
    }

    void LuaCoroutine::markReferences(GarbageCollector* gc) {
        // Mark any GC objects referenced by this coroutine
        // For now, we'll mark the yield values which may contain GC objects
        for (const auto& value : lastYieldValues_) {
            value.markReferences(gc);
        }

        // Note: parentState_ and luaState_ are managed elsewhere and don't need marking here
        // The coroutine promise is managed by unique_ptr and doesn't contain GC objects directly
    }
    
    CoroutineResult LuaCoroutine::resume(const Vec<Value>& args) {
        if (status_ == CoroutineStatus::DEAD) {
            return CoroutineResult{false, CoroutineStatus::DEAD};
        }
        
        try {
            if (!coroutine_) {
                // Create coroutine on first resume
                coroutine_ = std::make_unique<LuaCoroutinePromise>(createLuaCoroutine(this, args));
            }
            
            // Set status to running
            status_ = CoroutineStatus::RUNNING;
            
            // Resume the C++20 coroutine
            bool continued = coroutine_->resume();
            
            // Update status based on coroutine state
            updateStatus_();
            
            // Get result from coroutine
            const CoroutineResult& result = coroutine_->getResult();
            
            // Handle exceptions
            if (coroutine_->hasException()) {
                try {
                    coroutine_->rethrowException();
                } catch (const std::exception& e) {
                    status_ = CoroutineStatus::DEAD;
                    return CoroutineResult{false, CoroutineStatus::DEAD};
                }
            }
            
            return CoroutineResult{true, result.values, status_};
            
        } catch (const std::exception& e) {
            status_ = CoroutineStatus::DEAD;
            return CoroutineResult{false, CoroutineStatus::DEAD};
        }
    }
    
    CoroutineResult LuaCoroutine::yield(const Vec<Value>& values) {
        if (status_ != CoroutineStatus::RUNNING) {
            return CoroutineResult{false, status_};
        }
        
        // Store yield values
        lastYieldValues_ = values;
        status_ = CoroutineStatus::SUSPENDED;
        
        return CoroutineResult{true, values, CoroutineStatus::SUSPENDED};
    }
    
    void LuaCoroutine::setCoroutineFunction(UPtr<LuaCoroutinePromise> coro) {
        coroutine_ = std::move(coro);
    }
    
    void LuaCoroutine::updateStatus_() {
        if (!coroutine_) {
            status_ = CoroutineStatus::SUSPENDED;
            return;
        }
        
        if (coroutine_->isDone()) {
            status_ = CoroutineStatus::DEAD;
        } else {
            CoroutineStatus coroStatus = coroutine_->getStatus();
            if (coroStatus == CoroutineStatus::DEAD) {
                status_ = CoroutineStatus::DEAD;
            } else {
                status_ = CoroutineStatus::SUSPENDED;
            }
        }
    }
    
    // Coroutine function implementation
    LuaCoroutinePromise createLuaCoroutine(LuaCoroutine* coro, const Vec<Value>& args) {
        try {
            // This is a simplified implementation
            // In a full implementation, this would execute Lua function bytecode
            
            // Simulate some work
            CoroutineResult result{true, CoroutineStatus::SUSPENDED};
            
            // Example: yield a value
            result.values.push_back(Value(42));
            co_yield result;
            
            // Example: yield another value
            result.values.clear();
            result.values.push_back(Value(84));
            co_yield result;
            
            // Return final result
            result.values.clear();
            result.values.push_back(Value(126));
            result.status = CoroutineStatus::DEAD;
            co_return result;
            
        } catch (const std::exception& e) {
            CoroutineResult errorResult{false, CoroutineStatus::DEAD};
            co_return errorResult;
        }
    }
    
    // CoroutineManager implementation
    CoroutineManager::CoroutineManager() : currentCoroutine_(nullptr) {
    }
    
    CoroutineManager::~CoroutineManager() {
        cleanup();
    }
    
    LuaCoroutine* CoroutineManager::createCoroutine(State* parent, LuaState* luaState) {
        try {
            auto coroutine = std::make_unique<LuaCoroutine>(parent, luaState);
            LuaCoroutine* ptr = coroutine.get();
            coroutines_.push_back(std::move(coroutine));
            return ptr;
        } catch (const std::exception& e) {
            std::cerr << "Error creating coroutine: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    void CoroutineManager::destroyCoroutine(LuaCoroutine* coro) {
        if (!coro) return;
        
        // Remove from coroutines list
        auto it = std::find_if(coroutines_.begin(), coroutines_.end(),
            [coro](const UPtr<LuaCoroutine>& ptr) { return ptr.get() == coro; });
        
        if (it != coroutines_.end()) {
            if (currentCoroutine_ == coro) {
                currentCoroutine_ = nullptr;
            }
            coroutines_.erase(it);
        }
    }
    
    void CoroutineManager::cleanup() {
        currentCoroutine_ = nullptr;
        coroutines_.clear();
    }
}
