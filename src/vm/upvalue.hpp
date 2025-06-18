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
        union {
            Value* stackLocation;  // When open: points to stack
            Value closedValue;     // When closed: contains the value
        };
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
        
        // Safe getValue with explicit boundary checking
        Value getSafeValue() const;
        
        // Check if upvalue is in a valid state for access
        bool isValidForAccess() const;
        
        // Close the upvalue (move from stack to closed state)
        void close();
        
        // Check if upvalue is open
        bool isOpen() const { return state == State::Open; }
        
        // Check if upvalue is closed
        bool isClosed() const { return state == State::Closed; }
        
        // Get stack location (only valid when open)
        Value* getStackLocation() const;
        
        // Get/set next upvalue in linked list
        Upvalue* getNext() const { return next; }
        void setNext(Upvalue* nextUpvalue) { next = nextUpvalue; }
        
        // Check if this upvalue points to a specific stack location
        bool pointsTo(Value* location) const;
        
        // Factory method to create upvalues
        static GCRef<Upvalue> create(Value* location);
    };
}