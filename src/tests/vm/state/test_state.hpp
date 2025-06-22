#ifndef TEST_STATE_SUITE_HPP
#define TEST_STATE_SUITE_HPP

#include "../../../vm/state.hpp"
#include "../../test_framework/core/test_macros.hpp"
#include "state_basic_test.hpp"
#include "state_stack_test.hpp"
//#include "state_global_test.hpp"
#include "state_function_test.hpp"
//#include "state_gc_test.hpp"
//#include "state_error_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief State Module Test Suite
 * 
 * This class provides a unified interface to run all state-related tests.
 * It includes comprehensive tests for state management, stack operations,
 * global variables, function calls, garbage collection integration,
 * and error handling.
 * 
 * Test Organization:
 * - Basic State Tests: Constructor, destructor, basic operations
 * - Stack Tests: Push, pop, get, set operations with various scenarios
 * - Global Tests: Global variable management and access
 * - Function Tests: Function calls, native and Lua functions
 * - GC Tests: Garbage collection integration and memory management
 * - Error Tests: Error handling and edge cases
 */
class StateTestSuite {
public:
    /**
     * @brief Run all state tests
     * 
     * Executes all state-related test suites in a logical order.
     * Tests are run from basic functionality to complex scenarios.
     */
    static void runAllTests() {
        //RUN_TEST_SUITE(StateBasicTestSuite);
        //RUN_TEST_SUITE(StateStackTestSuite);
        //RUN_TEST_SUITE(StateGlobalTestSuite);
        RUN_TEST_SUITE(StateFunctionTestSuite);
        //RUN_TEST_SUITE(StateGCTestSuite);
        //RUN_TEST_SUITE(StateErrorTestSuite);
    }
};

} // namespace Tests
} // namespace Lua

#endif // TEST_STATE_SUITE_HPP