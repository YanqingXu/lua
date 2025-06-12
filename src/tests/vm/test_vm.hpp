#pragma once

// Include all VM test headers
#include "value_test.hpp"
#include "state_test.hpp"

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
    static void runAllTests();
    
private:
    /**
     * @brief Print section header for test organization
     * @param sectionName Name of the test section
     */
    static void printSectionHeader(const std::string& sectionName);
    
    /**
     * @brief Print section footer
     */
    static void printSectionFooter();
};

} // namespace Tests
} // namespace Lua
