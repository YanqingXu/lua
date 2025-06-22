#ifndef STATE_BASIC_TEST_HPP
#define STATE_BASIC_TEST_HPP

#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../test_framework/core/test_macros.hpp"
#include <cassert>

namespace Lua {
namespace Tests {

/**
 * @brief Individual test class for basic state functionality
 */
class StateBasicTest {
public:
    /**
     * @brief Test state constructor
     */
    static void testConstructor();

    /**
     * @brief Test state destructor
     */
    static void testDestructor();

    /**
     * @brief Test multiple state instances
     */
    static void testMultipleStates();

    /**
     * @brief Test initial stack size
     */
    static void testInitialStackSize();

    /**
     * @brief Test initial global variables
     */
    static void testInitialGlobals();

    /**
     * @brief Test stack capacity limits
     */
    static void testStackCapacity();

    /**
     * @brief Test GC object type
     */
    static void testGCObjectType();

    /**
     * @brief Test GC size calculation
     */
    static void testGCSize();

    /**
     * @brief Test GC mark references (basic test)
     */
    static void testGCMarkReferences();
};

/**
 * @brief Basic State Test Suite
 * 
 * Tests fundamental State class functionality including:
 * - Constructor and destructor
 * - Basic state initialization
 * - Memory allocation and cleanup
 * - GCObject integration
 */
class StateBasicTestSuite {
public:
    /**
     * @brief Run all basic state tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP("State Construction Tests", testStateConstruction);
        RUN_TEST_GROUP("State Properties Tests", testStateProperties);
        RUN_TEST_GROUP("State GC Integration Tests", testStateGCIntegration);
    }

private:
    /**
     * @brief Test state construction and destruction
     */
    static void testStateConstruction() {
        RUN_TEST(StateBasicTest, testConstructor);
        RUN_TEST(StateBasicTest, testDestructor);
        RUN_TEST(StateBasicTest, testMultipleStates);
    }

    /**
     * @brief Test basic state properties
     */
    static void testStateProperties() {
        RUN_TEST(StateBasicTest, testInitialStackSize);
        RUN_TEST(StateBasicTest, testInitialGlobals);
        RUN_TEST(StateBasicTest, testStackCapacity);
    }

    /**
     * @brief Test garbage collection integration
     */
    static void testStateGCIntegration() {
        RUN_TEST(StateBasicTest, testGCObjectType);
        RUN_TEST(StateBasicTest, testGCSize);
        RUN_TEST(StateBasicTest, testGCMarkReferences);
    }
};

} // namespace Tests
} // namespace Lua

#endif // STATE_BASIC_TEST_HPP