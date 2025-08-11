#pragma once

#include <iostream>
#include "gc_integration_test.hpp"
#include "gc_string_pool_test.hpp"
#include "gc_basic_test.hpp"
#include "gc_error_test.hpp"

namespace Lua::Tests {

/**
 * @brief GC module test suite
 * 
 * Coordinates all garbage collector related tests
 */
class GCTestSuite {
public:
    /**
     * @brief Run all GC module tests
     * 
     * Execute all test suites in this module
     */
    static void runAllTests() {
        // Note: Test framework removed - convert to simple function calls if needed
        // GCStringPoolTestSuite::runAllTests();
        // GCIntegrationTestSuite::runAllTests();
        // GCBasicTestSuite::runAllTests();
        // GCErrorTestSuite::runAllTests();

        std::cout << "GC tests disabled - test framework removed\n";
    }
};

} // namespace Lua::Tests
