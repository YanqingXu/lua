#include "upvalue.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/memory/allocator.hpp"
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
        if (state == State::Closed && closedValue.isGCObject()) {
            gc->markObject(closedValue.asGCObject());
        }
        // Note: When open, the value is on the stack and will be marked
        // by the State object's markReferences method
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