#ifndef LUA_TEST_CLOSURE_HPP
#define LUA_TEST_CLOSURE_HPP

#include <iostream>
#include <string>

#include "closure_basic_test.hpp"
#include "closure_advanced_test.hpp"
#include "closure_memory_test.hpp"
#include "closure_performance_test.hpp"
#include "closure_error_test.hpp"

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
     */
    static void runAllTests();

private:
    static void setupTestEnvironment();
    static void cleanupTestEnvironment();
};

} // namespace Tests
} // namespace Lua

#endif // LUA_TEST_CLOSURE_HPP