#include "state_stack_test.hpp"

namespace Lua {
namespace Tests {

void StateStackTest::testPushPop() {
    State state;
    
    // Test push
    state.push(Value(42));
    assert(state.getTop() == 1);
    
    // Test pop
    Value popped = state.pop();
    assert(popped.asNumber() == 42);
    assert(state.getTop() == 0);
}

void StateStackTest::testPushMultipleTypes() {
    State state;
    
    state.push(Value(nullptr));        // nil
    state.push(Value(true));           // boolean
    state.push(Value(3.14));           // number
    state.push(Value("hello"));        // string
    
    assert(state.getTop() == 4);
    assert(state.get(1).isNil());
    assert(state.get(2).isBoolean());
    assert(state.get(3).isNumber());
    assert(state.get(4).isString());
}

void StateStackTest::testStackTop() {
    State state;
    
    assert(state.getTop() == 0);
    
    for (int i = 1; i <= 5; ++i) {
        state.push(Value(i));
        assert(state.getTop() == i);
    }
    
    for (int i = 4; i >= 0; --i) {
        state.pop();
        assert(state.getTop() == i);
    }
}

void StateStackTest::testPositiveIndexing() {
    State state;
    
    state.push(Value(10));
    state.push(Value(20));
    state.push(Value(30));
    
    assert(state.get(1).asNumber() == 10);
    assert(state.get(2).asNumber() == 20);
    assert(state.get(3).asNumber() == 30);
}

void StateStackTest::testNegativeIndexing() {
    State state;
    
    state.push(Value(10));
    state.push(Value(20));
    state.push(Value(30));
    
    assert(state.get(-1).asNumber() == 30);  // top
    assert(state.get(-2).asNumber() == 20);  // second from top
    assert(state.get(-3).asNumber() == 10);  // bottom
}

void StateStackTest::testInvalidIndexing() {
    State state;
    
    state.push(Value(42));
    
    // Out of bounds indices should return nil
    assert(state.get(2).isNil());   // beyond top
    assert(state.get(-2).isNil());  // beyond bottom
    assert(state.get(100).isNil()); // way out of bounds
}

void StateStackTest::testZeroIndex() {
    State state;
    
    state.push(Value(42));
    
    // Index 0 should return nil
    assert(state.get(0).isNil());
}

void StateStackTest::testIsNil() {
    State state;
    
    state.push(Value(nullptr));
    state.push(Value(42));
    
    assert(state.isNil(1) == true);
    assert(state.isNil(2) == false);
    assert(state.isNil(3) == true);  // out of bounds
}

void StateStackTest::testIsBoolean() {
    State state;
    
    state.push(Value(true));
    state.push(Value(42));
    
    assert(state.isBoolean(1) == true);
    assert(state.isBoolean(2) == false);
}

void StateStackTest::testIsNumber() {
    State state;
    
    state.push(Value(3.14));
    state.push(Value("hello"));
    
    assert(state.isNumber(1) == true);
    assert(state.isNumber(2) == false);
}

void StateStackTest::testIsString() {
    State state;
    
    state.push(Value("hello"));
    state.push(Value(42));
    
    assert(state.isString(1) == true);
    assert(state.isString(2) == false);
}

void StateStackTest::testIsTable() {
    State state;
    
    // Note: This test assumes Table creation is available
    state.push(Value(42));
    
    assert(state.isTable(1) == false);
    // TODO: Add actual table test when table creation is available
}

void StateStackTest::testIsFunction() {
    State state;
    
    state.push(Value(42));
    
    assert(state.isFunction(1) == false);
    // TODO: Add actual function test when function creation is available
}

void StateStackTest::testToBoolean() {
    State state;
    
    state.push(Value(true));
    state.push(Value(false));
    state.push(Value(42));
    state.push(Value(nullptr));
    
    assert(state.toBoolean(1) == true);
    assert(state.toBoolean(2) == false);
    assert(state.toBoolean(3) == true);   // non-zero number
    assert(state.toBoolean(4) == false);  // nil
}

void StateStackTest::testToNumber() {
    State state;
    
    state.push(Value(3.14));
    state.push(Value("42"));
    state.push(Value("hello"));
    
    assert(state.toNumber(1) == 3.14);
    assert(state.toNumber(2) == 42.0);  // string to number conversion
    assert(state.toNumber(3) == 0.0);   // invalid conversion
}

void StateStackTest::testToString() {
    State state;
    
    state.push(Value("hello"));
    state.push(Value(42));
    state.push(Value(true));
    
    assert(state.toString(1) == "hello");
    assert(state.toString(2) == "42");    // number to string
    assert(state.toString(3) == "true");  // boolean to string
}

void StateStackTest::testToTable() {
    State state;
    
    state.push(Value(42));
    
    // TODO: Implement when table support is available
    // For now, just test that it doesn't crash
    // auto table = state.toTable(1);
}

void StateStackTest::testToFunction() {
    State state;
    
    state.push(Value(42));
    
    // TODO: Implement when function support is available
    // For now, just test that it doesn't crash
    // auto func = state.toFunction(1);
}

void StateStackTest::testStackOverflow() {
    State state;
    
    // Push many values to test overflow handling
    for (int i = 0; i < 1000; ++i) {
        state.push(Value(i));
    }
    
    assert(state.getTop() == 1000);
}

void StateStackTest::testStackUnderflow() {
    State state;
    
    // Try to pop from empty stack - should throw exception
    bool exceptionThrown = false;
    try {
        Value result = state.pop();
    } catch (const std::exception& e) {
        exceptionThrown = true;
        // Verify error message contains relevant keywords
        std::string msg = e.what();
        assert(msg.find("stack") != std::string::npos || 
               msg.find("underflow") != std::string::npos);
    }
    assert(exceptionThrown);
    assert(state.getTop() == 0);
}

void StateStackTest::testOutOfBoundsAccess() {
    State state;
    
    state.push(Value(42));
    
    // Test various out of bounds indices
    assert(state.get(0).isNil());    // zero index
    assert(state.get(2).isNil());    // beyond top
    assert(state.get(-2).isNil());   // beyond bottom
    assert(state.get(100).isNil());  // way out of bounds
    assert(state.get(-100).isNil()); // way out of bounds negative
}

void StateStackTest::testSetTop() {
    State state;
    
    // Push some values
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    assert(state.getTop() == 3);
    
    // Reduce stack size
    state.setTop(1);
    assert(state.getTop() == 1);
    assert(state.get(1).asNumber() == 1);
    
    // Increase stack size (should fill with nil)
    state.setTop(3);
    assert(state.getTop() == 3);
    assert(state.get(1).asNumber() == 1);
    assert(state.get(2).isNil());
    assert(state.get(3).isNil());
}

void StateStackTest::testClearStack() {
    State state;
    
    // Push some values
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    assert(state.getTop() == 3);
    
    // Clear stack
    state.setTop(0);
    assert(state.getTop() == 0);
}

void StateStackTest::testSetValue() {
    State state;
    
    // Push some values
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    
    // Set value at index 2
    state.set(2, Value(42));
    assert(state.get(2).asNumber() == 42);
    
    // Other values should remain unchanged
    assert(state.get(1).asNumber() == 1);
    assert(state.get(3).asNumber() == 3);
}

void StateStackTest::testStackExtension() {
    State state;
    
    // Push a reasonable number of values to test stack growth
    const int count = 500;  // Less than LUAI_MAXSTACK (1000)
    for (int i = 0; i < count; ++i) {
        state.push(Value(i));
    }
    
    assert(state.getTop() == count);
    
    // Verify values are correct
    for (int i = 0; i < count; ++i) {
        assert(state.get(i + 1).asNumber() == i);
    }
    
    // Pop all values
    for (int i = count - 1; i >= 0; --i) {
        Value val = state.pop();
        assert(val.asNumber() == i);
    }
    
    assert(state.getTop() == 0);
}

} // namespace Tests
} // namespace Lua