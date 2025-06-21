#ifndef STATE_STACK_TEST_HPP
#define STATE_STACK_TEST_HPP

#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../../vm/table.hpp"
#include "../../../vm/function.hpp"
#include "../../test_utils.hpp"
#include <cassert>
#include <stdexcept>

namespace Lua {
namespace Tests {

/**
 * @brief Stack Operations Test Suite
 * 
 * Tests comprehensive stack functionality including:
 * - Push and pop operations
 * - Stack indexing (positive and negative)
 * - Type checking and conversion
 * - Stack overflow and underflow handling
 * - Stack manipulation operations
 */
class StateStackTestSuite {
public:
    /**
     * @brief Run all stack operation tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Stack Operations", testBasicStackOperations);
        RUN_TEST_GROUP("Stack Indexing Tests", testStackIndexing);
        RUN_TEST_GROUP("Type Checking Tests", testTypeChecking);
        RUN_TEST_GROUP("Type Conversion Tests", testTypeConversion);
        RUN_TEST_GROUP("Stack Error Handling", testStackErrorHandling);
        RUN_TEST_GROUP("Stack Manipulation", testStackManipulation);
    }

private:
    /**
     * @brief Test basic push and pop operations
     */
    static void testBasicStackOperations();

    /**
     * @brief Test stack indexing
     */
    static void testStackIndexing();

    /**
     * @brief Test type checking functions
     */
    static void testTypeChecking();

    /**
     * @brief Test type conversion functions
     */
    static void testTypeConversion();

    /**
     * @brief Test stack error conditions
     */
    static void testStackErrorHandling();

    /**
     * @brief Test stack manipulation operations
     */
    static void testStackManipulation();
};

/**
 * @brief Individual test class for stack operations
 */
class StateStackTest {
public:
    /**
     * @brief Test basic push and pop operations
     */
    static void testPushPop();

    /**
     * @brief Test pushing multiple types
     */
    static void testPushMultipleTypes();

    /**
     * @brief Test stack top tracking
     */
    static void testStackTop();

    /**
     * @brief Test positive indexing (1-based)
     */
    static void testPositiveIndexing();

    /**
     * @brief Test negative indexing (from top)
     */
    static void testNegativeIndexing();

    /**
     * @brief Test invalid indexing
     */
    static void testInvalidIndexing();

    /**
     * @brief Test zero index (invalid in Lua)
     */
    static void testZeroIndex();

    /**
     * @brief Test isNil function
     */
    static void testIsNil();

    /**
     * @brief Test isBoolean function
     */
    static void testIsBoolean();

    /**
     * @brief Test isNumber function
     */
    static void testIsNumber();

    /**
     * @brief Test isString function
     */
    static void testIsString();

    /**
     * @brief Test isTable function
     */
    static void testIsTable();

    /**
     * @brief Test isFunction function
     */
    static void testIsFunction();

    /**
     * @brief Test toBoolean conversion
     */
    static void testToBoolean();

    /**
     * @brief Test toNumber conversion
     */
    static void testToNumber();

    /**
     * @brief Test toString conversion
     */
    static void testToString();

    /**
     * @brief Test toTable conversion
     */
    static void testToTable();

    /**
     * @brief Test toFunction conversion
     */
    static void testToFunction();

    /**
     * @brief Test stack overflow handling
     */
    static void testStackOverflow();

    /**
     * @brief Test stack underflow handling
     */
    static void testStackUnderflow();

    /**
     * @brief Test out of bounds access
     */
    static void testOutOfBoundsAccess();

    /**
     * @brief Test setTop operation
     */
    static void testSetTop();

    /**
     * @brief Test clearStack operation
     */
    static void testClearStack();

    /**
     * @brief Test set value operation
     */
    static void testSetValue();

    /**
     * @brief Test stack extension via set
     */
    static void testStackExtension();
};

/**
 * @brief Test basic push and pop operations implementation
 */
inline void StateStackTestSuite::testBasicStackOperations() {
    RUN_TEST(StateStackTest, testPushPop);
    RUN_TEST(StateStackTest, testPushMultipleTypes);
    RUN_TEST(StateStackTest, testStackTop);
}

/**
 * @brief Test stack indexing implementation
 */
inline void StateStackTestSuite::testStackIndexing() {
    RUN_TEST(StateStackTest, testPositiveIndexing);
    RUN_TEST(StateStackTest, testNegativeIndexing);
    RUN_TEST(StateStackTest, testInvalidIndexing);
    RUN_TEST(StateStackTest, testZeroIndex);
}

/**
 * @brief Test type checking functions implementation
 */
inline void StateStackTestSuite::testTypeChecking() {
    RUN_TEST(StateStackTest, testIsNil);
    RUN_TEST(StateStackTest, testIsBoolean);
    RUN_TEST(StateStackTest, testIsNumber);
    RUN_TEST(StateStackTest, testIsString);
    RUN_TEST(StateStackTest, testIsTable);
    RUN_TEST(StateStackTest, testIsFunction);
}

/**
 * @brief Test type conversion functions implementation
 */
inline void StateStackTestSuite::testTypeConversion() {
    RUN_TEST(StateStackTest, testToBoolean);
    RUN_TEST(StateStackTest, testToNumber);
    RUN_TEST(StateStackTest, testToString);
    RUN_TEST(StateStackTest, testToTable);
    RUN_TEST(StateStackTest, testToFunction);
}

/**
 * @brief Test stack error conditions implementation
 */
inline void StateStackTestSuite::testStackErrorHandling() {
    RUN_TEST(StateStackTest, testStackOverflow);
    RUN_TEST(StateStackTest, testStackUnderflow);
    RUN_TEST(StateStackTest, testOutOfBoundsAccess);
}

/**
 * @brief Test stack manipulation operations implementation
 */
inline void StateStackTestSuite::testStackManipulation() {
    RUN_TEST(StateStackTest, testSetTop);
    RUN_TEST(StateStackTest, testClearStack);
    RUN_TEST(StateStackTest, testSetValue);
    RUN_TEST(StateStackTest, testStackExtension);
}

} // namespace Tests
} // namespace Lua

#endif // STATE_STACK_TEST_HPP