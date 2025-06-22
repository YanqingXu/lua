#pragma once

#include "../test_utils.hpp"
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
        RUN_TEST_SUITE(GCStringPoolTestSuite);
        RUN_TEST_SUITE(GCIntegrationTestSuite);
        RUN_TEST_SUITE(GCBasicTestSuite);
        RUN_TEST_SUITE(GCErrorTestSuite);
        
        // TODO: Add other test suites here when available
    }
};

} // namespace Lua::Tests
