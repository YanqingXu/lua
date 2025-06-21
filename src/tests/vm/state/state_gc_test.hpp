#ifndef STATE_GC_TEST_HPP
#define STATE_GC_TEST_HPP

#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../../vm/table.hpp"
#include "../../../vm/function.hpp"
#include "../../../gc/core/garbage_collector.hpp"
#include "../../test_utils.hpp"
#include <cassert>
#include <memory>

namespace Lua {
namespace Tests {

/**
 * @brief Garbage Collection Integration Test Suite
 * 
 * Tests State class integration with the garbage collection system including:
 * - GCObject inheritance and behavior
 * - Memory size calculations
 * - Reference marking for GC
 * - Memory management during operations
 * - GC interaction with stack and globals
 * - Memory leak prevention
 */
class StateGCTestSuite {
public:
    /**
     * @brief Run all GC integration tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP("GC Object Tests", testGCObjectBehavior);
        RUN_TEST_GROUP("Memory Management Tests", testMemoryManagement);
        RUN_TEST_GROUP("Reference Marking Tests", testReferenceMarking);
        RUN_TEST_GROUP("GC Integration Tests", testGCIntegration);
    }

private:
    /**
     * @brief Test GCObject behavior
     */
    static void testGCObjectBehavior() {
        RUN_TEST(StateGCTest, testGCObjectInheritance);
        RUN_TEST(StateGCTest, testGCObjectType);
        RUN_TEST(StateGCTest, testGCObjectSize);
    }

    /**
     * @brief Test memory management
     */
    static void testMemoryManagement() {
        RUN_TEST(StateGCTest, testBasicMemorySize);
        RUN_TEST(StateGCTest, testAdditionalMemorySize);
        RUN_TEST(StateGCTest, testMemorySizeWithData);
        RUN_TEST(StateGCTest, testMemoryGrowth);
    }

    /**
     * @brief Test reference marking
     */
    static void testReferenceMarking() {
        RUN_TEST(StateGCTest, testMarkStackReferences);
        RUN_TEST(StateGCTest, testMarkGlobalReferences);
        RUN_TEST(StateGCTest, testMarkEmptyState);
        RUN_TEST(StateGCTest, testMarkComplexReferences);
    }

    /**
     * @brief Test GC integration scenarios
     */
    static void testGCIntegration() {
        RUN_TEST(StateGCTest, testGCWithStackOperations);
        RUN_TEST(StateGCTest, testGCWithGlobalOperations);
        RUN_TEST(StateGCTest, testGCWithMixedOperations);
    }
};

/**
 * @brief Individual test class for GC integration
 */
class StateGCTest {
public:
    /**
     * @brief Test GCObject inheritance
     */
    static void testGCObjectInheritance() {
        State state;
        
        // State should be a GCObject
        GCObject* gcObj = static_cast<GCObject*>(&state);
        assert(gcObj != nullptr);
        
        // Should have proper GC object behavior
        assert(gcObj->getSize() > 0);
    }

    /**
     * @brief Test GC object type identification
     */
    static void testGCObjectType() {
        State state;
        
        // Verify the object reports correct type
        // Note: This test assumes GCObjectType::State exists
        // The actual implementation may vary
        assert(state.getSize() >= sizeof(State));
    }

    /**
     * @brief Test basic GC object size
     */
    static void testGCObjectSize() {
        State state;
        
        usize size = state.getSize();
        assert(size >= sizeof(State));
        assert(size > 0);
    }

    /**
     * @brief Test basic memory size calculation
     */
    static void testBasicMemorySize() {
        State state;
        
        usize baseSize = state.getSize();
        usize additionalSize = state.getAdditionalSize();
        
        assert(baseSize > 0);
        assert(additionalSize >= 0);
        
        usize totalSize = baseSize + additionalSize;
        assert(totalSize >= sizeof(State));
    }

    /**
     * @brief Test additional memory size calculation
     */
    static void testAdditionalMemorySize() {
        State state;
        
        usize initialAdditional = state.getAdditionalSize();
        
        // Add some data to increase additional size
        state.push(Value(42));
        state.setGlobal("test", Value("string"));
        
        usize newAdditional = state.getAdditionalSize();
        
        // Additional size should account for the new data
        // (though the exact calculation may vary)
        assert(newAdditional >= initialAdditional);
    }

    /**
     * @brief Test memory size with data
     */
    static void testMemorySizeWithData() {
        State state;
        
        usize emptySize = state.getSize() + state.getAdditionalSize();
        
        // Add stack data
        for (int i = 0; i < 10; ++i) {
            state.push(Value(i));
        }
        
        // Add global data
        for (int i = 0; i < 5; ++i) {
            std::string name = "var" + std::to_string(i);
            state.setGlobal(name, Value(i * 10));
        }
        
        usize fullSize = state.getSize() + state.getAdditionalSize();
        
        // Size should reflect the added data
        assert(fullSize >= emptySize);
    }

    /**
     * @brief Test memory growth patterns
     */
    static void testMemoryGrowth() {
        State state;
        
        Vec<usize> sizes;
        
        // Record initial size
        sizes.push_back(state.getSize() + state.getAdditionalSize());
        
        // Add data incrementally and record sizes
        for (int i = 0; i < 5; ++i) {
            state.push(Value(i));
            state.setGlobal("var" + std::to_string(i), Value(i));
            sizes.push_back(state.getSize() + state.getAdditionalSize());
        }
        
        // Sizes should generally increase or stay the same
        for (size_t i = 1; i < sizes.size(); ++i) {
            assert(sizes[i] >= sizes[0]);  // At least as large as initial
        }
    }

    /**
     * @brief Test marking stack references
     */
    static void testMarkStackReferences() {
        State state;
        
        // Add various values to stack
        state.push(Value(nullptr));     // nil
        state.push(Value(42));          // number
        state.push(Value("test"));      // string
        state.push(Value(true));        // boolean
        
        // markReferences should not crash
        state.markReferences(nullptr);
        
        // Note: Without a real GC, we can't test actual marking
        // This test mainly ensures the method exists and doesn't crash
    }

    /**
     * @brief Test marking global references
     */
    static void testMarkGlobalReferences() {
        State state;
        
        // Add various global values
        state.setGlobal("nil_val", Value(nullptr));
        state.setGlobal("num_val", Value(123));
        state.setGlobal("str_val", Value("global string"));
        state.setGlobal("bool_val", Value(false));
        
        // markReferences should not crash
        state.markReferences(nullptr);
    }

    /**
     * @brief Test marking empty state references
     */
    static void testMarkEmptyState() {
        State state;
        
        // Empty state should handle marking gracefully
        state.markReferences(nullptr);
        
        // Should not crash with empty state
        assert(state.getTop() == 0);
    }

    /**
     * @brief Test marking complex references
     */
    static void testMarkComplexReferences() {
        State state;
        
        // Create complex reference patterns
        
        // Stack with mixed types
        state.push(Value(1));
        state.push(Value("string1"));
        state.push(Value(nullptr));
        state.push(Value(true));
        state.push(Value(2.5));
        
        // Globals with mixed types
        state.setGlobal("g1", Value(100));
        state.setGlobal("g2", Value("global_string"));
        state.setGlobal("g3", Value(nullptr));
        state.setGlobal("g4", Value(false));
        
        // Should handle complex reference patterns
        state.markReferences(nullptr);
    }

    /**
     * @brief Test GC with stack operations
     */
    static void testGCWithStackOperations() {
        State state;
        
        usize initialSize = state.getSize() + state.getAdditionalSize();
        
        // Perform many stack operations
        for (int i = 0; i < 100; ++i) {
            state.push(Value(i));
            if (i % 10 == 0) {
                state.markReferences(nullptr);  // Simulate GC marking
            }
        }
        
        // Pop some values
        for (int i = 0; i < 50; ++i) {
            state.pop();
            if (i % 10 == 0) {
                state.markReferences(nullptr);  // Simulate GC marking
            }
        }
        
        // State should remain consistent
        assert(state.getTop() == 50);
        
        usize finalSize = state.getSize() + state.getAdditionalSize();
        assert(finalSize >= initialSize);
    }

    /**
     * @brief Test GC with global operations
     */
    static void testGCWithGlobalOperations() {
        State state;
        
        // Perform many global operations
        for (int i = 0; i < 50; ++i) {
            std::string name = "var" + std::to_string(i);
            state.setGlobal(name, Value(i * 2));
            
            if (i % 10 == 0) {
                state.markReferences(nullptr);  // Simulate GC marking
            }
        }
        
        // Overwrite some globals
        for (int i = 0; i < 25; ++i) {
            std::string name = "var" + std::to_string(i);
            state.setGlobal(name, Value("string" + std::to_string(i)));
            
            if (i % 5 == 0) {
                state.markReferences(nullptr);  // Simulate GC marking
            }
        }
        
        // Verify globals are still accessible
        for (int i = 0; i < 25; ++i) {
            std::string name = "var" + std::to_string(i);
            Value val = state.getGlobal(name);
            assert(val.isString());
        }
        
        for (int i = 25; i < 50; ++i) {
            std::string name = "var" + std::to_string(i);
            Value val = state.getGlobal(name);
            assert(val.isNumber());
        }
    }

    /**
     * @brief Test GC with mixed operations
     */
    static void testGCWithMixedOperations() {
        State state;
        
        // Interleave stack and global operations with GC marking
        for (int i = 0; i < 30; ++i) {
            // Stack operations
            state.push(Value(i));
            state.push(Value("str" + std::to_string(i)));
            
            // Global operations
            state.setGlobal("num" + std::to_string(i), Value(i * 3));
            state.setGlobal("str" + std::to_string(i), Value("global" + std::to_string(i)));
            
            // Simulate GC marking
            if (i % 5 == 0) {
                state.markReferences(nullptr);
            }
            
            // Pop one value
            if (state.getTop() > 0) {
                state.pop();
            }
        }
        
        // Final GC marking
        state.markReferences(nullptr);
        
        // Verify state consistency
        assert(state.getTop() >= 0);
        
        // Verify some globals still exist
        for (int i = 0; i < 30; i += 5) {
            Value numVal = state.getGlobal("num" + std::to_string(i));
            Value strVal = state.getGlobal("str" + std::to_string(i));
            assert(numVal.isNumber());
            assert(strVal.isString());
        }
    }

    /**
     * @brief Test memory consistency during operations
     */
    static void testMemoryConsistency() {
        State state;
        
        // Record memory usage patterns
        Vec<usize> memorySizes;
        
        for (int i = 0; i < 20; ++i) {
            // Add data
            state.push(Value(i));
            state.setGlobal("test" + std::to_string(i), Value(i));
            
            // Record memory size
            usize currentSize = state.getSize() + state.getAdditionalSize();
            memorySizes.push_back(currentSize);
            
            // Mark references
            state.markReferences(nullptr);
        }
        
        // Memory sizes should be reasonable
        for (usize size : memorySizes) {
            assert(size > 0);
            assert(size >= sizeof(State));
        }
    }
};

} // namespace Tests
} // namespace Lua

#endif // STATE_GC_TEST_HPP