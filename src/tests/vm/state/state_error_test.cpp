#include "state_error_test.hpp"

namespace Lua {
namespace Tests {

void StateErrorTest::testStackOverflow() {
    State state;
    
    bool overflowDetected = false;
    int pushCount = 0;
    
    try {
        // Try to push beyond stack limit
        for (int i = 0; i < 200000; ++i) {
            state.push(Value(i));
            pushCount++;
        }
    } catch (const std::exception& e) {
        overflowDetected = true;
        // Verify exception message contains relevant information
        std::string msg = e.what();
        assert(msg.find("overflow") != std::string::npos || 
               msg.find("stack") != std::string::npos);
    }
    
    // Either overflow was detected or we have a very large stack
    assert(overflowDetected || pushCount > 100000);
    
    // State should still be usable after overflow
    if (overflowDetected) {
        // Try a simple operation
        int currentTop = state.getTop();
        assert(currentTop >= 0);
    }
}

void StateErrorTest::testStackUnderflow() {
    State state;
    
    // Test pop from empty stack
    bool underflowDetected = false;
    try {
        state.pop();
    } catch (const std::exception& e) {
        underflowDetected = true;
        std::string msg = e.what();
        assert(msg.find("underflow") != std::string::npos || 
               msg.find("stack") != std::string::npos);
    }
    
    assert(underflowDetected);
    
    // Test multiple pops from empty stack
    for (int i = 0; i < 5; ++i) {
        bool exceptionThrown = false;
        try {
            state.pop();
        } catch (const std::exception&) {
            exceptionThrown = true;
        }
        assert(exceptionThrown);
    }
}

void StateErrorTest::testStackConsistencyAfterError() {
    State state;
    
    // Add some valid data
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    
    assert(state.getTop() == 3);
    
    // Cause an underflow error
    try {
        for (int i = 0; i < 10; ++i) {
            state.pop();
        }
    } catch (const std::exception&) {
        // Expected
    }
    
    // Stack should still be in a consistent state
    int top = state.getTop();
    assert(top >= 0);
    
    // Should be able to perform normal operations
    state.push(Value(42));
    assert(state.get(state.getTop()).asNumber() == 42);
}

void StateErrorTest::testInvalidPositiveIndex() {
    State state;
    
    state.push(Value(10));
    state.push(Value(20));
    
    // Test accessing beyond stack top
    Value val1 = state.get(5);  // Beyond top
    assert(val1.isNil());
    
    Value val2 = state.get(100);  // Way beyond top
    assert(val2.isNil());
    
    // Test type checking with invalid indices
    assert(state.isNil(5) == true);
    assert(state.isNumber(100) == false);
    
    // Test conversion with invalid indices
    assert(state.toNumber(5) == 0.0);
    assert(state.toString(100) == "");
}

void StateErrorTest::testInvalidNegativeIndex() {
    State state;
    
    state.push(Value(10));
    state.push(Value(20));
    
    // Test accessing beyond stack bottom
    Value val1 = state.get(-5);  // Beyond bottom
    assert(val1.isNil());
    
    Value val2 = state.get(-100);  // Way beyond bottom
    assert(val2.isNil());
    
    // Test type checking with invalid negative indices
    assert(state.isNil(-5) == true);
    assert(state.isNumber(-100) == false);
}

void StateErrorTest::testZeroIndexHandling() {
    State state;
    
    state.push(Value(42));
    
    // Index 0 should be invalid in Lua
    Value val = state.get(0);
    assert(val.isNil());
    
    assert(state.isNil(0) == true);
    assert(state.isNumber(0) == false);
    assert(state.toNumber(0) == 0.0);
    
    // Setting at index 0 should be ignored
    state.set(0, Value(999));
    assert(state.get(1).asNumber() == 42);  // Original value unchanged
}

void StateErrorTest::testExtremeIndexValues() {
    State state;
    
    state.push(Value(42));
    
    // Test with very large positive indices
    assert(state.get(1000000).isNil());
    assert(state.get(INT_MAX).isNil());
    
    // Test with very large negative indices
    assert(state.get(-1000000).isNil());
    assert(state.get(INT_MIN).isNil());
    
    // These should not crash the program
    state.set(1000000, Value(1));
    state.set(-1000000, Value(2));
}

void StateErrorTest::testCallNonFunctionValue() {
    State state;
    
    Vec<Value> args;
    
    // Test calling different non-function types
    Value types[] = {
        Value(nullptr),      // nil
        Value(42),           // number
        Value("string"),     // string
        Value(true)          // boolean
    };
    
    for (const Value& val : types) {
        bool exceptionThrown = false;
        try {
            state.call(val, args);
        } catch (const std::exception& e) {
            exceptionThrown = true;
            std::string msg = e.what();
            assert(msg.find("function") != std::string::npos || 
                   msg.find("call") != std::string::npos);
        }
        assert(exceptionThrown);
    }
}

void StateErrorTest::testCallWithInvalidArguments() {
    State state;
    
    // Note: This test is limited without actual function objects
    // We test the error handling mechanism
    
    Value nonFunction = Value(42);
    Vec<Value> args;
    args.push_back(Value(nullptr));
    
    bool exceptionThrown = false;
    try {
        state.call(nonFunction, args);
    } catch (const std::exception&) {
        exceptionThrown = true;
    }
    
    assert(exceptionThrown);
}

void StateErrorTest::testCodeExecutionErrors() {
    State state;
    
    // Test invalid Lua syntax
    bool success = state.doString("invalid syntax $$$ @@@");
    assert(!success);
    
    // Test incomplete statements
    success = state.doString("x = ");
    assert(!success);
    
    // Test undefined function calls
    success = state.doString("undefined_function()");
    assert(!success);
    
    // Test malformed expressions
    success = state.doString("1 + + 2");
    assert(!success);
    
    // State should remain usable after errors
    success = state.doString("x = 42");
    assert(success);
    assert(state.getGlobal("x").asNumber() == 42);
}

void StateErrorTest::testMaxStackSize() {
    State state;
    
    // Try to approach maximum stack size
    bool limitReached = false;
    int maxPushed = 0;
    
    try {
        for (int i = 0; i < 100000; ++i) {
            state.push(Value(i));
            maxPushed = i + 1;
        }
    } catch (const std::exception&) {
        limitReached = true;
    }
    
    // Either we reached a limit or pushed many values
    assert(limitReached || maxPushed > 50000);
    
    // Stack should still be functional
    int currentTop = state.getTop();
    assert(currentTop >= 0);
    assert(currentTop <= maxPushed);
}

void StateErrorTest::testLargeGlobalNames() {
    State state;
    
    // Test very long variable name
    std::string longName(10000, 'a');
    
    state.setGlobal(longName, Value(42));
    Value retrieved = state.getGlobal(longName);
    assert(retrieved.asNumber() == 42);
    
    // Test extremely long name
    std::string extremeName(100000, 'b');
    state.setGlobal(extremeName, Value(123));
    Value extremeRetrieved = state.getGlobal(extremeName);
    assert(extremeRetrieved.asNumber() == 123);
}

void StateErrorTest::testEmptyOperations() {
    State state;
    
    // Test operations on empty state
    assert(state.getTop() == 0);
    assert(state.getGlobal("nonexistent").isNil());
    
    // Test empty string operations
    state.setGlobal("", Value(42));
    assert(state.getGlobal("").asNumber() == 42);
    
    // Test doString with empty code
    bool success = state.doString("");
    assert(success);  // Empty code should succeed
}

void StateErrorTest::testNullPointerHandling() {
    State state;
    
    // Test markReferences with null GC
    state.markReferences(nullptr);  // Should not crash
    
    // Test with some data
    state.push(Value(42));
    state.setGlobal("test", Value("string"));
    state.markReferences(nullptr);  // Should not crash
}

void StateErrorTest::testMassiveStackOperations() {
    State state;
    
    const int iterations = 10000;
    
    // Massive push operations
    for (int i = 0; i < iterations; ++i) {
        try {
            state.push(Value(i));
        } catch (const std::exception&) {
            // Stack overflow is acceptable
            break;
        }
    }
    
    int maxTop = state.getTop();
    
    // Massive pop operations
    for (int i = 0; i < maxTop; ++i) {
        try {
            state.pop();
        } catch (const std::exception&) {
            // Should not happen if we don't exceed maxTop
            assert(false);
        }
    }
    
    assert(state.getTop() == 0);
}

void StateErrorTest::testMassiveGlobalOperations() {
    State state;
    
    const int iterations = 1000;
    
    // Set many globals
    for (int i = 0; i < iterations; ++i) {
        std::string name = "var" + std::to_string(i);
        state.setGlobal(name, Value(i));
    }
    
    // Verify all globals
    for (int i = 0; i < iterations; ++i) {
        std::string name = "var" + std::to_string(i);
        Value val = state.getGlobal(name);
        assert(val.asNumber() == i);
    }
    
    // Overwrite all globals
    for (int i = 0; i < iterations; ++i) {
        std::string name = "var" + std::to_string(i);
        state.setGlobal(name, Value(i * 2));
    }
    
    // Verify overwritten values
    for (int i = 0; i < iterations; ++i) {
        std::string name = "var" + std::to_string(i);
        Value val = state.getGlobal(name);
        assert(val.asNumber() == i * 2);
    }
}

void StateErrorTest::testRepeatedErrorConditions() {
    State state;
    
    // Repeatedly cause and handle stack underflow
    for (int i = 0; i < 100; ++i) {
        bool exceptionThrown = false;
        try {
            state.pop();
        } catch (const std::exception&) {
            exceptionThrown = true;
        }
        assert(exceptionThrown);
    }
    
    // State should still be usable
    state.push(Value(42));
    assert(state.getTop() == 1);
    assert(state.get(1).asNumber() == 42);
}

void StateErrorTest::testRecoveryAfterStackError() {
    State state;
    
    // Cause stack underflow
    try {
        state.pop();
    } catch (const std::exception&) {
        // Expected
    }
    
    // State should recover and work normally
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    
    assert(state.getTop() == 3);
    assert(state.get(1).asNumber() == 1);
    assert(state.get(2).asNumber() == 2);
    assert(state.get(3).asNumber() == 3);
    
    Value popped = state.pop();
    assert(popped.asNumber() == 3);
    assert(state.getTop() == 2);
}

void StateErrorTest::testRecoveryAfterFunctionError() {
    State state;
    
    // Cause function call error
    try {
        Vec<Value> args;
        state.call(Value(42), args);
    } catch (const std::exception&) {
        // Expected
    }
    
    // State should recover and work normally
    state.setGlobal("test", Value("recovery"));
    assert(state.getGlobal("test").toString() == "recovery");
    
    state.push(Value(100));
    assert(state.get(1).asNumber() == 100);
}

void StateErrorTest::testStateConsistencyAfterErrors() {
    State state;
    
    // Cause multiple types of errors
    
    // Stack underflow
    try { state.pop(); } catch (...) {}
    
    // Invalid function call
    try {
        Vec<Value> args;
        state.call(Value("not a function"), args);
    } catch (...) {}
    
    // Invalid code execution
    state.doString("invalid syntax @@@");
    
    // After all errors, state should still be consistent
    assert(state.getTop() == 0);
    
    // Should be able to perform all normal operations
    state.push(Value(1));
    state.push(Value(2));
    state.setGlobal("x", Value(42));
    state.setGlobal("y", Value("test"));
    
    assert(state.getTop() == 2);
    assert(state.get(1).asNumber() == 1);
    assert(state.get(2).asNumber() == 2);
    assert(state.getGlobal("x").asNumber() == 42);
    assert(state.getGlobal("y").toString() == "test");
    
    // GC operations should still work
    state.markReferences(nullptr);
    
    usize size = state.getSize() + state.getAdditionalSize();
    assert(size > 0);
}

} // namespace Tests
} // namespace Lua