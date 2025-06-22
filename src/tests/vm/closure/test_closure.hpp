#ifndef LUA_TEST_CLOSURE_HPP
#define LUA_TEST_CLOSURE_HPP

#include <iostream>
#include <string>

#include "../../../test_framework/core/test_macros.hpp"
#include "closure_basic_test.hpp"
#include "closure_advanced_test.hpp"
#include "closure_memory_test.hpp"
#include "closure_performance_test.hpp"
#include "closure_error_test.hpp"
#include "closure_boundary_test.hpp"



namespace Lua {
namespace Tests {

/**
 * Main Closure Test Suite
 * 
 * This class serves as the main entry point for all closure-related tests.
 * It coordinates and runs various specialized test suites for different
 * aspects of closure functionality.
 */
class ClosureTestSuite {
public:
    /**
     * @brief Run all closure tests
     * 
     * Executes the complete suite of closure tests by coordinating
     * the execution of all closure test modules including basic tests,
     * advanced tests, memory tests, performance tests, and error tests.
     */    static void runAllTests() {
        // Run basic functionality tests
        RUN_TEST_SUITE(ClosureBasicTest);

        // Run advanced functionality tests
        RUN_TEST_SUITE(ClosureAdvancedTest);

        // Run memory and lifecycle tests
        RUN_TEST_SUITE(ClosureMemoryTest);

        // Run performance tests
        RUN_TEST_SUITE(ClosurePerformanceTest);

        // Run error handling tests
        RUN_TEST_SUITE(ClosureErrorTest);

        // Run boundary condition tests
        RUN_TEST_SUITE(ClosureBoundaryTest);
    }
};

} // namespace Tests
} // namespace Lua

#endif // LUA_TEST_CLOSURE_HPP