#ifndef STATE_GLOBAL_TEST_HPP
#define STATE_GLOBAL_TEST_HPP

#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../../test_framework/core/test_utils.hpp"
#include <cassert>
#include <string>

namespace Lua {
namespace Tests {

/**
 * @brief Global Variables Test Suite
 * 
 * Tests comprehensive global variable functionality including:
 * - Setting and getting global variables
 * - Different value types as globals
 * - Global variable overwriting
 * - Non-existent global access
 * - Global variable persistence
 * - Memory management of globals
 */
class StateGlobalTestSuite {
public:
    /**
     * @brief Run all global variable tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Global Operations", testBasicGlobalOperations);
        RUN_TEST_GROUP("Global Type Tests", testGlobalTypes);
        RUN_TEST_GROUP("Global Edge Cases", testGlobalEdgeCases);
        RUN_TEST_GROUP("Global Persistence", testGlobalPersistence);
    }

private:
    /**
     * @brief Test basic global variable operations
     */
    static void testBasicGlobalOperations() {
        RUN_TEST(StateGlobalTest, testSetGetGlobal);
        RUN_TEST(StateGlobalTest, testOverwriteGlobal);
        RUN_TEST(StateGlobalTest, testNonExistentGlobal);
    }

    /**
     * @brief Test different types as global variables
     */
    static void testGlobalTypes() {
        RUN_TEST(StateGlobalTest, testNilGlobal);
        RUN_TEST(StateGlobalTest, testBooleanGlobal);
        RUN_TEST(StateGlobalTest, testNumberGlobal);
        RUN_TEST(StateGlobalTest, testStringGlobal);
        RUN_TEST(StateGlobalTest, testComplexGlobals);
    }

    /**
     * @brief Test edge cases and special scenarios
     */
    static void testGlobalEdgeCases() {
        RUN_TEST(StateGlobalTest, testEmptyStringName);
        RUN_TEST(StateGlobalTest, testSpecialCharacterNames);
        RUN_TEST(StateGlobalTest, testLongVariableNames);
        RUN_TEST(StateGlobalTest, testCaseSensitivity);
    }

    /**
     * @brief Test global variable persistence
     */
    static void testGlobalPersistence() {
        RUN_TEST(StateGlobalTest, testGlobalPersistenceAcrossOperations);
        RUN_TEST(StateGlobalTest, testMultipleGlobals);
        RUN_TEST(StateGlobalTest, testGlobalIsolation);
    }
};

/**
 * @brief Individual test class for global variable operations
 */
class StateGlobalTest {
public:
    /**
     * @brief Test basic set and get operations
     */
    static void testSetGetGlobal() {
        State state;
        
        // Set a global variable
        state.setGlobal("test_var", Value(42));
        
        // Get the global variable
        Value retrieved = state.getGlobal("test_var");
        assert(retrieved.isNumber());
        assert(retrieved.asNumber() == 42);
    }

    /**
     * @brief Test overwriting global variables
     */
    static void testOverwriteGlobal() {
        State state;
        
        // Set initial value
        state.setGlobal("var", Value(10));
        assert(state.getGlobal("var").asNumber() == 10);
        
        // Overwrite with different value
        state.setGlobal("var", Value(20));
        assert(state.getGlobal("var").asNumber() == 20);
        
        // Overwrite with different type
        state.setGlobal("var", Value("hello"));
        assert(state.getGlobal("var").isString());
        assert(state.getGlobal("var").toString() == "hello");
    }

    /**
     * @brief Test accessing non-existent global variables
     */
    static void testNonExistentGlobal() {
        State state;
        
        // Access non-existent variable should return nil
        Value nonExistent = state.getGlobal("does_not_exist");
        assert(nonExistent.isNil());
    }

    /**
     * @brief Test nil as global variable value
     */
    static void testNilGlobal() {
        State state;
        
        // Set nil value
        state.setGlobal("nil_var", Value(nullptr));
        
        // Get nil value
        Value retrieved = state.getGlobal("nil_var");
        assert(retrieved.isNil());
    }

    /**
     * @brief Test boolean as global variable value
     */
    static void testBooleanGlobal() {
        State state;
        
        // Test true
        state.setGlobal("bool_true", Value(true));
        Value trueVal = state.getGlobal("bool_true");
        assert(trueVal.isBoolean());
        assert(trueVal.asBoolean() == true);
        
        // Test false
        state.setGlobal("bool_false", Value(false));
        Value falseVal = state.getGlobal("bool_false");
        assert(falseVal.isBoolean());
        assert(falseVal.asBoolean() == false);
    }

    /**
     * @brief Test number as global variable value
     */
    static void testNumberGlobal() {
        State state;
        
        // Test integer
        state.setGlobal("int_var", Value(42));
        Value intVal = state.getGlobal("int_var");
        assert(intVal.isNumber());
        assert(intVal.asNumber() == 42);
        
        // Test floating point
        state.setGlobal("float_var", Value(3.14159));
        Value floatVal = state.getGlobal("float_var");
        assert(floatVal.isNumber());
        assert(floatVal.asNumber() == 3.14159);
        
        // Test negative number
        state.setGlobal("neg_var", Value(-123.45));
        Value negVal = state.getGlobal("neg_var");
        assert(negVal.isNumber());
        assert(negVal.asNumber() == -123.45);
        
        // Test zero
        state.setGlobal("zero_var", Value(0.0));
        Value zeroVal = state.getGlobal("zero_var");
        assert(zeroVal.isNumber());
        assert(zeroVal.asNumber() == 0.0);
    }

    /**
     * @brief Test string as global variable value
     */
    static void testStringGlobal() {
        State state;
        
        // Test regular string
        state.setGlobal("str_var", Value("hello world"));
        Value strVal = state.getGlobal("str_var");
        assert(strVal.isString());
        assert(strVal.toString() == "hello world");
        
        // Test empty string
        state.setGlobal("empty_str", Value(""));
        Value emptyVal = state.getGlobal("empty_str");
        assert(emptyVal.isString());
        assert(emptyVal.toString() == "");
        
        // Test string with special characters
        state.setGlobal("special_str", Value("Hello\nWorld\t!"));
        Value specialVal = state.getGlobal("special_str");
        assert(specialVal.isString());
        assert(specialVal.toString() == "Hello\nWorld\t!");
    }

    /**
     * @brief Test complex global variable scenarios
     */
    static void testComplexGlobals() {
        State state;
        
        // Set multiple globals of different types
        state.setGlobal("num", Value(42));
        state.setGlobal("str", Value("test"));
        state.setGlobal("bool", Value(true));
        state.setGlobal("nil_val", Value(nullptr));
        
        // Verify all are accessible and correct
        assert(state.getGlobal("num").asNumber() == 42);
        assert(state.getGlobal("str").toString() == "test");
        assert(state.getGlobal("bool").asBoolean() == true);
        assert(state.getGlobal("nil_val").isNil());
    }

    /**
     * @brief Test empty string as variable name
     */
    static void testEmptyStringName() {
        State state;
        
        // Empty string should be a valid variable name
        state.setGlobal("", Value(42));
        Value retrieved = state.getGlobal("");
        assert(retrieved.asNumber() == 42);
    }

    /**
     * @brief Test special characters in variable names
     */
    static void testSpecialCharacterNames() {
        State state;
        
        // Test various special characters
        state.setGlobal("var_with_underscore", Value(1));
        state.setGlobal("var123", Value(2));
        state.setGlobal("_leading_underscore", Value(3));
        state.setGlobal("var.with.dots", Value(4));
        state.setGlobal("var-with-dashes", Value(5));
        
        assert(state.getGlobal("var_with_underscore").asNumber() == 1);
        assert(state.getGlobal("var123").asNumber() == 2);
        assert(state.getGlobal("_leading_underscore").asNumber() == 3);
        assert(state.getGlobal("var.with.dots").asNumber() == 4);
        assert(state.getGlobal("var-with-dashes").asNumber() == 5);
    }

    /**
     * @brief Test long variable names
     */
    static void testLongVariableNames() {
        State state;
        
        // Create a very long variable name
        std::string longName = "very_long_variable_name_that_exceeds_normal_expectations_and_continues_for_a_while_to_test_memory_handling";
        
        state.setGlobal(longName, Value(999));
        Value retrieved = state.getGlobal(longName);
        assert(retrieved.asNumber() == 999);
    }

    /**
     * @brief Test case sensitivity of variable names
     */
    static void testCaseSensitivity() {
        State state;
        
        // Set variables with different cases
        state.setGlobal("Variable", Value(1));
        state.setGlobal("variable", Value(2));
        state.setGlobal("VARIABLE", Value(3));
        state.setGlobal("VaRiAbLe", Value(4));
        
        // Verify they are treated as different variables
        assert(state.getGlobal("Variable").asNumber() == 1);
        assert(state.getGlobal("variable").asNumber() == 2);
        assert(state.getGlobal("VARIABLE").asNumber() == 3);
        assert(state.getGlobal("VaRiAbLe").asNumber() == 4);
    }

    /**
     * @brief Test global persistence across stack operations
     */
    static void testGlobalPersistenceAcrossOperations() {
        State state;
        
        // Set global
        state.setGlobal("persistent", Value(100));
        
        // Perform stack operations
        state.push(Value(1));
        state.push(Value(2));
        state.push(Value(3));
        state.pop();
        state.pop();
        state.clearStack();
        
        // Global should still exist
        assert(state.getGlobal("persistent").asNumber() == 100);
    }

    /**
     * @brief Test multiple globals management
     */
    static void testMultipleGlobals() {
        State state;
        
        // Set many globals
        for (int i = 0; i < 100; ++i) {
            std::string name = "var" + std::to_string(i);
            state.setGlobal(name, Value(i * 10));
        }
        
        // Verify all globals
        for (int i = 0; i < 100; ++i) {
            std::string name = "var" + std::to_string(i);
            Value val = state.getGlobal(name);
            assert(val.asNumber() == i * 10);
        }
    }

    /**
     * @brief Test global isolation between states
     */
    static void testGlobalIsolation() {
        State state1;
        State state2;
        
        // Set globals in different states
        state1.setGlobal("shared_name", Value(111));
        state2.setGlobal("shared_name", Value(222));
        
        // Verify isolation
        assert(state1.getGlobal("shared_name").asNumber() == 111);
        assert(state2.getGlobal("shared_name").asNumber() == 222);
        
        // Set unique globals
        state1.setGlobal("unique1", Value(333));
        state2.setGlobal("unique2", Value(444));
        
        // Verify cross-state access returns nil
        assert(state1.getGlobal("unique2").isNil());
        assert(state2.getGlobal("unique1").isNil());
    }
};

} // namespace Tests
} // namespace Lua

#endif // STATE_GLOBAL_TEST_HPP