#pragma once

#include "../common/types.hpp"
#include "../common/defines.hpp"
#include "../gc/core/gc_object.hpp"
#include "value.hpp"

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    
    // Upvalue represents a captured variable from an outer scope
    class Upvalue : public GCObject {
    public:
        enum class State {
            Open,   // Points to a stack location
            Closed  // Contains its own value
        };
        
    private:
        State state;
        Value* stackLocation;  // When open: points to stack
        Value closedValue;     // When closed: contains the value
        Upvalue* next;  // For linked list of open upvalues
        
    public:
        // Constructor for open upvalue
        explicit Upvalue(Value* location);
        
        // Destructor
        ~Upvalue();
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Get the current value with boundary checking
        Value getValue() const;
        
        // Set the current value with boundary checking
        void setValue(const Value& value);

        // Set value with write barrier support - Lua 5.1兼容
        void setValueWithBarrier(const Value& value, LuaState* L);

        // Safe getValue with explicit boundary checking
        Value getSafeValue() const;
        
        // Check if upvalue is in a valid state for access
        bool isValidForAccess() const;
        
        // Close the upvalue (move from stack to closed state)
        void close();

        // Close with write barrier support - Lua 5.1兼容
        void closeWithBarrier(LuaState* L);

        // Check if upvalue is open
        bool isOpen() const { return state == State::Open; }

        // Check if upvalue is closed
        bool isClosed() const { return state == State::Closed; }
        
        // Get stack location (only valid when open)
        Value* getStackLocation() const;
        
        // Get/set next upvalue in linked list
        Upvalue* getNext() const { return next; }
        void setNext(Upvalue* nextUpvalue) { next = nextUpvalue; }

        // Set next with write barrier support - Lua 5.1兼容
        void setNextWithBarrier(Upvalue* nextUpvalue, LuaState* L);

        // Upvalue链表遍历 - 用于GC标记
        template<typename Func>
        void traverseUpvalueChain(Func&& func) {
            Upvalue* current = this;
            while (current) {
                func(current);
                current = current->next;
            }
        }

        // 检查upvalue是否在指定栈位置之上（用于关闭操作）
        bool isAboveStackLevel(Value* level) const;
        
        // Check if this upvalue points to a specific stack location
        bool pointsTo(Value* location) const;
        
        // Factory method to create upvalues
        static GCRef<Upvalue> create(Value* location);
    };
}