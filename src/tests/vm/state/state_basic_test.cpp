#include "state_basic_test.hpp"
#include <cassert>
#include <string>
#include <climits>

namespace Lua {
namespace Tests {

// StateBasicTest implementation

void StateBasicTest::testConstructor() {
    State* state = new State();
    assert(state != nullptr);
    assert(state->getTop() == 0);
    delete state;
}

void StateBasicTest::testDestructor() {
    State* state = new State();
    // Add some data to test cleanup
    state->push(Value(42));
    state->setGlobal("test", Value("cleanup"));
    delete state; // Should not crash
}

void StateBasicTest::testMultipleStates() {
    State state1;
    State state2;
    
    state1.push(Value(1));
    state2.push(Value(2));
    
    assert(state1.getTop() == 1);
    assert(state2.getTop() == 1);
    assert(state1.get(1).asNumber() == 1);
    assert(state2.get(1).asNumber() == 2);
}

void StateBasicTest::testInitialStackSize() {
    State state;
    assert(state.getTop() == 0);
}

void StateBasicTest::testInitialGlobals() {
    State state;
    Value nonExistent = state.getGlobal("nonexistent");
    assert(nonExistent.isNil());
}

void StateBasicTest::testStackCapacity() {
    State state;
    // Test that we can push at least some values
    for (int i = 0; i < 100; ++i) {
        state.push(Value(i));
    }
    assert(state.getTop() == 100);
}

void StateBasicTest::testGCObjectType() {
    State state;
    // State should be a GCObject
    assert(state.getSize() > 0);
}

void StateBasicTest::testGCSize() {
    State state;
    usize baseSize = state.getSize();
    usize additionalSize = state.getAdditionalSize();
    
    assert(baseSize >= sizeof(State));
    assert(additionalSize >= 0);
}

void StateBasicTest::testGCMarkReferences() {
    State state;
    state.push(Value(42));
    state.setGlobal("test", Value("string"));
    
    // markReferences should not crash
    // Note: We can't easily test the actual marking without a GC instance
    // This test mainly ensures the method exists and doesn't crash
    state.markReferences(nullptr);
}

} // namespace Tests
} // namespace Lua