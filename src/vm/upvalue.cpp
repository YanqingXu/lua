#include "upvalue.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/memory/allocator.hpp"
#include "../gc/barriers/write_barrier.hpp"  // 为写屏障支持
#include "lua_state.hpp"  // 为LuaState类型
#include <stdexcept>

namespace Lua {
    Upvalue::Upvalue(Value* location)
        : GCObject(GCObjectType::Upvalue, sizeof(Upvalue))
        , state(State::Open)
        , stackLocation(location)
        , closedValue()  // Initialize closedValue to nil
        , next(nullptr) {
        if (location == nullptr) {
            throw std::invalid_argument("Upvalue location cannot be null");
        }
    }
    
    Upvalue::~Upvalue() {
        // No special cleanup needed since closedValue is now a regular member
        // The Value destructor will be called automatically
    }
    
    void Upvalue::markReferences(GarbageCollector* gc) {
        // Lua 5.1兼容的upvalue GC标记实现

        // 1. 标记upvalue的值
        if (state == State::Closed && closedValue.isGCObject()) {
            GCObject* valueObj = closedValue.asGCObject();
            if (valueObj) {
                gc->markObject(valueObj);
            }
        }
        // 注意：当upvalue是开放状态时，值在栈上，会被State对象的markReferences方法标记

        // 2. 标记链表中的下一个upvalue
        if (next) {
            gc->markObject(next);
        }

        // 3. 如果需要，可以遍历整个upvalue链表进行标记
        // 但通常每个upvalue会单独调用markReferences，所以这里只标记直接引用
    }
    
    usize Upvalue::getSize() const {
        return sizeof(Upvalue);
    }
    
    usize Upvalue::getAdditionalSize() const {
        // Upvalue doesn't have additional dynamic memory
        return 0;
    }
    
    Value Upvalue::getValue() const {
        if (state == State::Open) {
            return *stackLocation;
        } else {
            return closedValue;
        }
    }
    
    void Upvalue::setValue(const Value& value) {
        if (state == State::Open) {
            *stackLocation = value;
        } else {
            closedValue = value;
        }
    }

    void Upvalue::setValueWithBarrier(const Value& value, LuaState* L) {
        // 应用写屏障 - upvalue引用新的GC对象
        if (L && value.isGCObject()) {
            GCObject* valueObj = value.asGCObject();
            if (valueObj) {
                // upvalue对象引用新的GC对象时需要写屏障
                luaC_objbarrier(L, this, valueObj);
            }
        }

        // 设置值
        setValue(value);
    }
    
    void Upvalue::close() {
        if (state == State::Open) {
            // Move the value from stack to closed storage
            Value valueToClose = *stackLocation;
            // DEBUG: Removed debug output for cleaner testing

            // Change state to closed
            state = State::Closed;

            // Simply assign the value (no placement new needed)
            closedValue = valueToClose;

            // DEBUG: Removed debug output for cleaner testing

            // stackLocation is no longer valid
            stackLocation = nullptr;
        }
    }

    void Upvalue::closeWithBarrier(LuaState* L) {
        if (state == State::Open) {
            // 获取要关闭的值
            Value valueToClose = *stackLocation;

            // 应用写屏障 - upvalue将引用关闭的值
            if (L && valueToClose.isGCObject()) {
                GCObject* valueObj = valueToClose.asGCObject();
                if (valueObj) {
                    // upvalue对象引用新的GC对象时需要写屏障
                    luaC_objbarrier(L, this, valueObj);
                }
            }

            // 执行关闭操作
            close();
        }
    }

    void Upvalue::setNextWithBarrier(Upvalue* nextUpvalue, LuaState* L) {
        // 应用写屏障 - upvalue引用下一个upvalue
        if (L && nextUpvalue) {
            // upvalue对象引用另一个upvalue时需要写屏障
            luaC_objbarrier(L, this, nextUpvalue);
        }

        // 设置下一个upvalue
        setNext(nextUpvalue);
    }

    bool Upvalue::isAboveStackLevel(Value* level) const {
        // 只有开放的upvalue才有栈位置概念
        if (state != State::Open || !stackLocation || !level) {
            return false;
        }

        // 检查栈位置是否在指定级别之上
        // 注意：栈向下增长，所以较高的地址表示较低的栈位置
        return stackLocation >= level;
    }
    
    Value* Upvalue::getStackLocation() const {
        if (state == State::Open) {
            return stackLocation;
        }
        return nullptr;
    }
    
    bool Upvalue::pointsTo(Value* location) const {
        return state == State::Open && stackLocation == location;
    }
    
    Value Upvalue::getSafeValue() const {
        if (!isValidForAccess()) {
            throw std::runtime_error(ERR_DESTROYED_UPVALUE);
        }
        return getValue();
    }
    
    bool Upvalue::isValidForAccess() const {
        if (state == State::Open) {
            // Check if stack location is still valid
            return stackLocation != nullptr;
        } else {
            // For closed upvalues, always valid
            return true;
        }
    }
    
    // Factory function to create upvalues using GC allocator
    GCRef<Upvalue> Upvalue::create(Value* location) {
        extern GCAllocator* g_gcAllocator;
        
        if (g_gcAllocator) {
            Upvalue* obj = g_gcAllocator->allocateObject<Upvalue>(GCObjectType::Upvalue, location);
            return GCRef<Upvalue>(obj);
        } else {
            // Fallback to direct allocation
            Upvalue* obj = new Upvalue(location);
            return GCRef<Upvalue>(obj);
        }
    }
}