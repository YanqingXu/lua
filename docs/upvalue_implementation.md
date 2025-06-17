# Upvalue Implementation Guide

## Overview

This document describes the implementation of upvalues in our Lua interpreter, focusing on Phase 3 of the project which includes VM execution, upvalue lifecycle management, and garbage collection integration.

## What are Upvalues?

Upvalues are Lua's mechanism for implementing lexical scoping in closures. When a function references a variable from an outer scope, that variable becomes an "upvalue" for the inner function. Upvalues allow closures to capture and maintain access to variables from their defining environment.

## Implementation Architecture

### Core Components

1. **Upvalue Class** (`upvalue.hpp`, `upvalue.cpp`)
   - Manages individual upvalue objects
   - Handles open/closed state transitions
   - Integrates with garbage collection

2. **VM Integration** (`vm.hpp`, `vm.cpp`)
   - Manages upvalue lifecycle during execution
   - Maintains linked list of open upvalues
   - Handles upvalue closure when stack frames are destroyed

3. **Function Integration** (`function.hpp`, `function.cpp`)
   - Stores upvalue references in function objects
   - Provides access methods for upvalue manipulation

### Upvalue States

Upvalues have two states:

- **Open**: Points to a location on the stack
- **Closed**: Contains a copy of the value (stack location no longer valid)

```cpp
enum class State {
    Open,   // Points to stack location
    Closed  // Contains closed value
};
```

### Data Structure

```cpp
class Upvalue : public GCObject {
private:
    State state;
    union {
        Value* stackLocation;  // When open
        Value closedValue;     // When closed
    };
    Upvalue* next;  // Linked list pointer
};
```

## VM Upvalue Management

### Open Upvalue List

The VM maintains a sorted linked list of open upvalues, ordered by stack address (highest to lowest). This allows efficient:

- Finding existing upvalues for a given stack location
- Closing upvalues when stack frames are destroyed
- Garbage collection of unreachable upvalues

### Key Operations

#### 1. Finding or Creating Upvalues

```cpp
GCRef<Upvalue> VM::findOrCreateUpvalue(Value* location) {
    // Search existing open upvalues
    // Create new upvalue if not found
    // Insert into sorted linked list
}
```

#### 2. Closing Upvalues

```cpp
void VM::closeUpvalues(Value* level) {
    // Close all upvalues at or above the given stack level
    // Move values from stack to upvalue objects
}
```

#### 3. Cleanup

```cpp
void VM::closeAllUpvalues() {
    // Close all remaining open upvalues
    // Called when VM execution ends
}
```

## Instruction Implementation

### OP_CLOSURE

Creates a new closure from a function prototype:

1. Create new function instance
2. Read upvalue binding instructions
3. For each upvalue:
   - If local: capture from current stack frame
   - If upvalue: inherit from current function
4. Bind upvalues to new closure

### OP_GETUPVAL

Retrieves value from an upvalue:

1. Get upvalue index from instruction
2. Retrieve upvalue from current function
3. Get value from upvalue (handles open/closed states)
4. Push value onto stack

### OP_SETUPVAL

Sets value in an upvalue:

1. Get upvalue index and value from instruction
2. Retrieve upvalue from current function
3. Set value in upvalue (handles open/closed states)

## Garbage Collection Integration

### GC Object Type

Upvalues are registered as a GC object type:

```cpp
enum class GCObjectType {
    // ... other types
    Upvalue
};
```

### Reference Marking

Upvalues participate in mark-and-sweep garbage collection:

```cpp
void Upvalue::markReferences(GarbageCollector* gc) {
    if (state == State::Closed) {
        closedValue.mark(gc);
    }
    // Open upvalues are marked through stack scanning
}
```

### VM Reference Marking

The VM marks all upvalue references during GC:

```cpp
void VM::markReferences(GarbageCollector* gc) {
    // Mark current function
    // Mark all open upvalues
    // Mark call frame upvalues
}
```

## Usage Examples

### Simple Closure

```lua
function createCounter()
    local count = 0  -- Becomes upvalue
    return function()
        count = count + 1
        return count
    end
end
```

### Multiple Upvalues

```lua
function createCalculator(initial)
    local value = initial    -- Upvalue 1
    local operations = 0     -- Upvalue 2
    
    return function(op, x)
        if op == "add" then
            value = value + x
        else
            value = value - x
        end
        operations = operations + 1
        return value, operations
    end
end
```

## Testing

Comprehensive tests are provided in `upvalue_test.cpp`:

1. **Upvalue Creation**: Tests basic upvalue creation and state
2. **Upvalue Closing**: Tests open-to-closed state transitions
3. **VM Management**: Tests VM upvalue lifecycle management
4. **Instruction Handling**: Tests upvalue-related VM instructions

## Performance Considerations

### Memory Efficiency

- Upvalues use union for space efficiency
- Linked list avoids dynamic array reallocations
- GC integration prevents memory leaks

### Execution Efficiency

- Sorted upvalue list enables O(n) closure operations
- Direct pointer access for open upvalues
- Minimal overhead for closed upvalues

### GC Efficiency

- Precise marking prevents false positives
- Automatic cleanup when functions are destroyed
- Integration with existing GC infrastructure

## Future Enhancements

1. **Optimization**: Implement upvalue caching for frequently accessed upvalues
2. **Debugging**: Add upvalue inspection capabilities for debugger
3. **Profiling**: Add upvalue access profiling for performance analysis
4. **Serialization**: Implement upvalue serialization for state persistence

## Conclusion

This upvalue implementation provides a robust foundation for Lua's closure semantics while maintaining efficiency and proper integration with the garbage collector. The design follows Lua's reference implementation patterns while adapting to our modern C++ architecture.