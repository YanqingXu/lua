#include "state_gc_test.hpp"

namespace Lua {
namespace Tests {

void StateGCTest::testGCObjectBehavior() {
    State state;
    
    // Test that State is a proper GC object
    assert(state.getType() == GCObjectType::STATE);
    
    // Type should remain consistent
    state.push(Value(42));
    assert(state.getType() == GCObjectType::STATE);
    
    state.setGlobal("test", Value("string"));
    assert(state.getType() == GCObjectType::STATE);
    
    state.pop();
    assert(state.getType() == GCObjectType::STATE);
    
    // After various operations, type should still be STATE
    state.clearStack();
    assert(state.getType() == GCObjectType::STATE);
}

void StateGCTest::testGCSizeCalculation() {
    State state;
    
    // Get initial sizes
    usize initialSize = state.getSize();
    usize initialAdditionalSize = state.getAdditionalSize();
    
    assert(initialSize > 0);  // State should have some base size
    
    // Add data to stack and check size changes
    state.push(Value(42));
    usize size1 = state.getSize();
    usize additionalSize1 = state.getAdditionalSize();
    
    // Size should account for the new data
    assert(size1 + additionalSize1 >= initialSize + initialAdditionalSize);
    
    // Add more complex data
    state.push(Value("this is a longer string that takes more memory"));
    state.push(Value(3.14159));
    state.push(Value(true));
    
    usize size2 = state.getSize();
    usize additionalSize2 = state.getAdditionalSize();
    
    // Size should have grown
    assert(size2 + additionalSize2 >= size1 + additionalSize1);
    
    // Add globals
    state.setGlobal("number", Value(123));
    state.setGlobal("string", Value("global string value"));
    state.setGlobal("boolean", Value(false));
    
    usize size3 = state.getSize();
    usize additionalSize3 = state.getAdditionalSize();
    
    // Size should account for globals
    assert(size3 + additionalSize3 >= size2 + additionalSize2);
    
    // Remove some data
    state.pop();
    state.pop();
    
    usize size4 = state.getSize();
    usize additionalSize4 = state.getAdditionalSize();
    
    // Size might not immediately decrease (depends on implementation)
    // But should not increase
    assert(size4 + additionalSize4 <= size3 + additionalSize3 + 100); // Small tolerance
}

void StateGCTest::testGCMarkReferences() {
    State state;
    
    // Test marking with null GC (should not crash)
    state.markReferences(nullptr);
    
    // Add various types of data
    state.push(Value(42));
    state.push(Value("test string"));
    state.push(Value(true));
    state.push(Value(nullptr));
    
    state.setGlobal("global_num", Value(100));
    state.setGlobal("global_str", Value("global string"));
    state.setGlobal("global_bool", Value(false));
    state.setGlobal("global_nil", Value(nullptr));
    
    // Mark references (should not crash)
    state.markReferences(nullptr);
    
    // All data should still be accessible after marking
    assert(state.getTop() == 4);
    assert(state.get(1).asNumber() == 42);
    assert(state.get(2).toString() == "test string");
    assert(state.get(3).asBoolean() == true);
    assert(state.get(4).isNil());
    
    assert(state.getGlobal("global_num").asNumber() == 100);
    assert(state.getGlobal("global_str").toString() == "global string");
    assert(state.getGlobal("global_bool").asBoolean() == false);
    assert(state.getGlobal("global_nil").isNil());
    
    // Test multiple marking calls
    for (int i = 0; i < 10; ++i) {
        state.markReferences(nullptr);
    }
    
    // Data should still be intact
    assert(state.getTop() == 4);
    assert(state.getGlobal("global_num").asNumber() == 100);
}

void StateGCTest::testGCMemoryManagement() {
    State state;
    
    // Test memory management through multiple operations
    usize baseSize = state.getSize() + state.getAdditionalSize();
    
    // Add and remove data multiple times
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Add data
        for (int i = 0; i < 100; ++i) {
            state.push(Value(i));
            state.setGlobal("temp_" + std::to_string(i), Value(i * 2));
        }
        
        usize grownSize = state.getSize() + state.getAdditionalSize();
        assert(grownSize >= baseSize);
        
        // Remove stack data
        for (int i = 0; i < 100; ++i) {
            state.pop();
        }
        
        // Overwrite globals with nil
        for (int i = 0; i < 100; ++i) {
            state.setGlobal("temp_" + std::to_string(i), Value(nullptr));
        }
        
        // Mark references to potentially trigger cleanup
        state.markReferences(nullptr);
        
        usize afterCleanupSize = state.getSize() + state.getAdditionalSize();
        // Size might not immediately shrink, but shouldn't grow indefinitely
    }
    
    // Final state should be usable
    assert(state.getTop() == 0);
    state.push(Value("final test"));
    assert(state.get(1).toString() == "final test");
}

void StateGCTest::testGCWithStackOperations() {
    State state;
    
    // Test GC integration with stack operations
    
    // Fill stack with data
    for (int i = 0; i < 50; ++i) {
        state.push(Value("string_" + std::to_string(i)));
    }
    
    usize stackSize = state.getSize() + state.getAdditionalSize();
    
    // Mark references with full stack
    state.markReferences(nullptr);
    
    // All stack data should still be accessible
    for (int i = 0; i < 50; ++i) {
        assert(state.get(i + 1).toString() == "string_" + std::to_string(i));
    }
    
    // Modify stack and test GC
    for (int i = 0; i < 25; ++i) {
        state.pop();
    }
    
    state.markReferences(nullptr);
    
    // Remaining data should be intact
    assert(state.getTop() == 25);
    for (int i = 0; i < 25; ++i) {
        assert(state.get(i + 1).toString() == "string_" + std::to_string(i));
    }
    
    // Clear stack and test GC
    state.clearStack();
    state.markReferences(nullptr);
    
    assert(state.getTop() == 0);
    
    // Should be able to use stack normally after GC
    state.push(Value(999));
    assert(state.get(1).asNumber() == 999);
}

void StateGCTest::testGCWithGlobalOperations() {
    State state;
    
    // Test GC integration with global variables
    
    // Set many globals
    for (int i = 0; i < 100; ++i) {
        std::string name = "global_" + std::to_string(i);
        state.setGlobal(name, Value("value_" + std::to_string(i)));
    }
    
    usize globalsSize = state.getSize() + state.getAdditionalSize();
    
    // Mark references with many globals
    state.markReferences(nullptr);
    
    // All globals should still be accessible
    for (int i = 0; i < 100; ++i) {
        std::string name = "global_" + std::to_string(i);
        assert(state.getGlobal(name).toString() == "value_" + std::to_string(i));
    }
    
    // Overwrite some globals and test GC
    for (int i = 0; i < 50; ++i) {
        std::string name = "global_" + std::to_string(i);
        state.setGlobal(name, Value(nullptr));
    }
    
    state.markReferences(nullptr);
    
    // Check that overwritten globals are nil
    for (int i = 0; i < 50; ++i) {
        std::string name = "global_" + std::to_string(i);
        assert(state.getGlobal(name).isNil());
    }
    
    // Check that remaining globals are intact
    for (int i = 50; i < 100; ++i) {
        std::string name = "global_" + std::to_string(i);
        assert(state.getGlobal(name).toString() == "value_" + std::to_string(i));
    }
    
    // Should be able to set new globals after GC
    state.setGlobal("new_global", Value("new_value"));
    assert(state.getGlobal("new_global").toString() == "new_value");
}

void StateGCTest::testGCWithMixedOperations() {
    State state;
    
    // Test GC with mixed stack and global operations
    
    // Initial setup
    state.push(Value(1));
    state.push(Value(2));
    state.setGlobal("initial", Value("start"));
    
    usize initialSize = state.getSize() + state.getAdditionalSize();
    state.markReferences(nullptr);
    
    // Complex sequence of operations
    for (int round = 0; round < 10; ++round) {
        // Add stack data
        for (int i = 0; i < 10; ++i) {
            state.push(Value(round * 10 + i));
        }
        
        // Add global data
        for (int i = 0; i < 5; ++i) {
            std::string name = "round_" + std::to_string(round) + "_" + std::to_string(i);
            state.setGlobal(name, Value(round * 100 + i));
        }
        
        // Mark references mid-operation
        state.markReferences(nullptr);
        
        // Remove some stack data
        for (int i = 0; i < 5; ++i) {
            state.pop();
        }
        
        // Verify data integrity
        assert(state.get(1).asNumber() == 1);
        assert(state.get(2).asNumber() == 2);
        assert(state.getGlobal("initial").toString() == "start");
        
        // Mark references again
        state.markReferences(nullptr);
    }
    
    // Final verification
    assert(state.getTop() > 2);  // Should have initial + remaining data
    assert(state.get(1).asNumber() == 1);
    assert(state.get(2).asNumber() == 2);
    assert(state.getGlobal("initial").toString() == "start");
    
    // Check some round globals
    assert(state.getGlobal("round_0_0").asNumber() == 0);
    assert(state.getGlobal("round_9_4").asNumber() == 904);
    
    // Final GC mark
    state.markReferences(nullptr);
    
    // State should still be fully functional
    state.push(Value("final"));
    state.setGlobal("final_test", Value("done"));
    
    assert(state.get(state.getTop()).toString() == "final");
    assert(state.getGlobal("final_test").toString() == "done");
}

void StateGCTest::testGCStressTest() {
    State state;
    
    // Stress test GC with many operations
    
    const int iterations = 100;
    const int dataPerIteration = 20;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Add stack data
        for (int i = 0; i < dataPerIteration; ++i) {
            state.push(Value(iter * dataPerIteration + i));
        }
        
        // Add global data
        for (int i = 0; i < dataPerIteration; ++i) {
            std::string name = "stress_" + std::to_string(iter) + "_" + std::to_string(i);
            state.setGlobal(name, Value("stress_value_" + std::to_string(iter * dataPerIteration + i)));
        }
        
        // Periodically mark references
        if (iter % 10 == 0) {
            state.markReferences(nullptr);
        }
        
        // Periodically clean up some data
        if (iter % 20 == 0 && iter > 0) {
            // Remove some stack data
            int removeCount = std::min(dataPerIteration / 2, state.getTop());
            for (int i = 0; i < removeCount; ++i) {
                state.pop();
            }
            
            // Clear some globals
            for (int i = 0; i < dataPerIteration / 2; ++i) {
                std::string name = "stress_" + std::to_string(iter - 20) + "_" + std::to_string(i);
                state.setGlobal(name, Value(nullptr));
            }
            
            state.markReferences(nullptr);
        }
    }
    
    // Final GC mark
    state.markReferences(nullptr);
    
    // State should still be usable
    assert(state.getTop() >= 0);
    
    state.push(Value("stress_test_complete"));
    state.setGlobal("stress_complete", Value(true));
    
    assert(state.get(state.getTop()).toString() == "stress_test_complete");
    assert(state.getGlobal("stress_complete").asBoolean() == true);
    
    // Verify some data still exists
    bool foundSomeData = false;
    for (int iter = iterations - 10; iter < iterations; ++iter) {
        for (int i = 0; i < dataPerIteration; ++i) {
            std::string name = "stress_" + std::to_string(iter) + "_" + std::to_string(i);
            if (!state.getGlobal(name).isNil()) {
                foundSomeData = true;
                break;
            }
        }
        if (foundSomeData) break;
    }
    
    // Should have found some recent data
    assert(foundSomeData);
}

void StateGCTest::testGCConsistency() {
    State state;
    
    // Test that GC operations maintain data consistency
    
    // Set up known data
    state.push(Value(42));
    state.push(Value("test"));
    state.push(Value(true));
    state.setGlobal("number", Value(123));
    state.setGlobal("string", Value("hello"));
    state.setGlobal("boolean", Value(false));
    
    // Record initial state
    int initialTop = state.getTop();
    Value stackVal1 = state.get(1);
    Value stackVal2 = state.get(2);
    Value stackVal3 = state.get(3);
    Value globalNum = state.getGlobal("number");
    Value globalStr = state.getGlobal("string");
    Value globalBool = state.getGlobal("boolean");
    
    // Perform multiple GC marks
    for (int i = 0; i < 50; ++i) {
        state.markReferences(nullptr);
        
        // Verify consistency after each mark
        assert(state.getTop() == initialTop);
        assert(state.get(1).asNumber() == stackVal1.asNumber());
        assert(state.get(2).toString() == stackVal2.toString());
        assert(state.get(3).asBoolean() == stackVal3.asBoolean());
        assert(state.getGlobal("number").asNumber() == globalNum.asNumber());
        assert(state.getGlobal("string").toString() == globalStr.toString());
        assert(state.getGlobal("boolean").asBoolean() == globalBool.asBoolean());
    }
    
    // Add more data and test consistency
    state.push(Value(999));
    state.setGlobal("new_var", Value("new"));
    
    for (int i = 0; i < 20; ++i) {
        state.markReferences(nullptr);
        
        // All data should remain consistent
        assert(state.getTop() == 4);
        assert(state.get(4).asNumber() == 999);
        assert(state.getGlobal("new_var").toString() == "new");
        
        // Original data should be unchanged
        assert(state.get(1).asNumber() == 42);
        assert(state.getGlobal("number").asNumber() == 123);
    }
}

void StateGCTest::testGCEdgeCases() {
    State state;
    
    // Test GC with empty state
    state.markReferences(nullptr);
    assert(state.getTop() == 0);
    
    // Test GC with only nil values
    state.push(Value(nullptr));
    state.push(Value(nullptr));
    state.setGlobal("nil1", Value(nullptr));
    state.setGlobal("nil2", Value(nullptr));
    
    state.markReferences(nullptr);
    
    assert(state.getTop() == 2);
    assert(state.get(1).isNil());
    assert(state.get(2).isNil());
    assert(state.getGlobal("nil1").isNil());
    assert(state.getGlobal("nil2").isNil());
    
    // Test GC after clearing all data
    state.clearStack();
    state.setGlobal("nil1", Value(nullptr));
    state.setGlobal("nil2", Value(nullptr));
    
    state.markReferences(nullptr);
    
    assert(state.getTop() == 0);
    
    // Test GC with very large strings
    std::string largeString(10000, 'x');
    state.push(Value(largeString));
    state.setGlobal("large", Value(largeString));
    
    state.markReferences(nullptr);
    
    assert(state.get(1).toString() == largeString);
    assert(state.getGlobal("large").toString() == largeString);
    
    // Test GC with rapid allocation/deallocation
    for (int i = 0; i < 100; ++i) {
        state.push(Value(i));
        state.markReferences(nullptr);
        state.pop();
        state.markReferences(nullptr);
    }
    
    // State should be clean
    assert(state.getTop() == 1);  // Only the large string
    assert(state.get(1).toString() == largeString);
}

} // namespace Tests
} // namespace Lua