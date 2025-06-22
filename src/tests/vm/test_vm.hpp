#pragma once

// Include all VM test headers
#include "value_test.hpp"
#include "state_test.hpp"
#include "closure/test_closure.hpp"
#include "../test_utils.hpp"
//#include "closure/test_closure.hpp"
#include "state/test_state.hpp"
#include "vm_error_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Virtual Machine Test Suite
 * 
 * This class provides a unified interface to run all virtual machine-related tests.
 * It includes tests for value system, state management, stack operations,
 * and instruction execution.
 */
class VMTestSuite {
public:
    /**
     * @brief Run all VM tests
     * 
     * Executes all virtual machine-related test suites in a logical order.
     * Tests are run from basic value types to complex state management.
     */
    static void runAllTests() {
      RUN_TEST_MODULE("State Module", StateTestSuite);
      //RUN_TEST_MODULE("Closure Module", ClosureTestSuite);
      RUN_TEST_MODULE("VM Error Handling Module", VMErrorTest);
    }
};

} // namespace Tests
} // namespace Lua
