#include "lua_coroutine.hpp"
#include "lua_state.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/barriers/write_barrier.hpp"  // 为写屏障支持
#include <iostream>
#include <algorithm>  // 为std::find

namespace Lua {
    
    // LuaCoroutine implementation
    LuaCoroutine::LuaCoroutine(State* parent, LuaState* luaState)
        : GCObject(GCObjectType::Thread, sizeof(LuaCoroutine))
        , parentState_(parent), luaState_(luaState), status_(CoroutineStatus::SUSPENDED)
        , stackTop_(0), stackSize_(256), parentCoroutine_(nullptr) {
        // 初始化协程栈
        stack_.reserve(stackSize_);
        callStack_.reserve(64);  // 初始调用栈大小

        // 初始化子协程列表
        childCoroutines_.reserve(8);
    }
    
    LuaCoroutine::~LuaCoroutine() {
        // Cleanup is handled by unique_ptr
    }

    void LuaCoroutine::markReferences(GarbageCollector* gc) {
        // Lua 5.1兼容的协程GC标记实现
        // 参考官方lgc.c中的traversestack函数

        // 1. 标记yield值中的GC对象
        for (const auto& value : lastYieldValues_) {
            if (value.isGCObject()) {
                GCObject* obj = value.asGCObject();
                if (obj) {
                    gc->markObject(obj);
                }
            }
        }

        // 2. 遍历并标记协程栈中的所有值
        for (usize i = 0; i < stackTop_ && i < stack_.size(); ++i) {
            const Value& value = stack_[i];
            if (value.isGCObject()) {
                GCObject* obj = value.asGCObject();
                if (obj) {
                    gc->markObject(obj);
                }
            }
        }

        // 3. 标记调用栈中的值
        for (const auto& value : callStack_) {
            if (value.isGCObject()) {
                GCObject* obj = value.asGCObject();
                if (obj) {
                    gc->markObject(obj);
                }
            }
        }

        // 4. 标记父协程引用
        if (parentCoroutine_) {
            gc->markObject(parentCoroutine_);
        }

        // 5. 标记所有子协程
        for (LuaCoroutine* child : childCoroutines_) {
            if (child) {
                gc->markObject(child);
            }
        }

        // 注意：parentState_和luaState_由其他地方管理，不需要在这里标记
    }

    usize LuaCoroutine::getSize() const {
        return sizeof(LuaCoroutine);
    }

    usize LuaCoroutine::getAdditionalSize() const {
        // 计算协程栈和调用栈占用的额外内存
        usize stackMemory = stack_.capacity() * sizeof(Value);
        usize callStackMemory = callStack_.capacity() * sizeof(Value);
        usize childListMemory = childCoroutines_.capacity() * sizeof(LuaCoroutine*);

        return stackMemory + callStackMemory + childListMemory;
    }

    // === 协程栈管理实现 ===

    void LuaCoroutine::pushValue(const Value& value) {
        if (stackTop_ >= stack_.size()) {
            // 扩展栈大小
            stack_.resize(stackTop_ + 1);
        }
        stack_[stackTop_++] = value;
    }

    Value LuaCoroutine::popValue() {
        if (stackTop_ == 0) {
            return Value(); // 返回nil值
        }
        return stack_[--stackTop_];
    }

    const Value& LuaCoroutine::getStackValue(usize index) const {
        if (index >= stackTop_ || index >= stack_.size()) {
            static const Value nilValue;
            return nilValue;
        }
        return stack_[index];
    }

    void LuaCoroutine::setStackValue(usize index, const Value& value) {
        if (index >= stack_.size()) {
            stack_.resize(index + 1);
        }
        if (index >= stackTop_) {
            stackTop_ = index + 1;
        }
        stack_[index] = value;
    }

    // === 写屏障支持实现 ===

    void LuaCoroutine::pushValueWithBarrier(const Value& value, LuaState* L) {
        // 在压栈前应用写屏障
        if (L && value.isGCObject()) {
            GCObject* valueObj = value.asGCObject();
            if (valueObj) {
                // 协程对象引用新的GC对象时需要写屏障
                luaC_objbarrier(L, this, valueObj);
            }
        }

        pushValue(value);
    }

    void LuaCoroutine::setStackValueWithBarrier(usize index, const Value& value, LuaState* L) {
        // 在设置栈值前应用写屏障
        if (L && value.isGCObject()) {
            GCObject* valueObj = value.asGCObject();
            if (valueObj) {
                // 协程对象引用新的GC对象时需要写屏障
                luaC_objbarrier(L, this, valueObj);
            }
        }

        setStackValue(index, value);
    }

    void LuaCoroutine::setStatusWithBarrier(CoroutineStatus newStatus, LuaState* L) {
        // 状态变更时的写屏障
        if (L && status_ != newStatus) {
            // 协程状态变更可能影响GC，应用写屏障
            luaC_objbarrier(L, this, nullptr);
        }

        status_ = newStatus;
    }

    // === 协程间引用管理实现 ===

    void LuaCoroutine::setParentCoroutine(LuaCoroutine* parent) {
        parentCoroutine_ = parent;

        // 如果设置了父协程，将自己添加到父协程的子列表中
        if (parent) {
            parent->addChildCoroutine(this);
        }
    }

    void LuaCoroutine::addChildCoroutine(LuaCoroutine* child) {
        if (child && std::find(childCoroutines_.begin(), childCoroutines_.end(), child) == childCoroutines_.end()) {
            childCoroutines_.push_back(child);
        }
    }

    void LuaCoroutine::removeChildCoroutine(LuaCoroutine* child) {
        auto it = std::find(childCoroutines_.begin(), childCoroutines_.end(), child);
        if (it != childCoroutines_.end()) {
            childCoroutines_.erase(it);
        }
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
                } catch (const std::exception& /*e*/) {
                    status_ = CoroutineStatus::DEAD;
                    return CoroutineResult{false, CoroutineStatus::DEAD};
                }
            }
            
            return CoroutineResult{true, result.values, status_};
            
        } catch (const std::exception& /*e*/) {
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
            
        } catch (const std::exception& /*e*/) {
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
