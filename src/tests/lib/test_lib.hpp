#pragma once

// System headers
#include <iostream>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"

// Test module headers
#include "base_lib_test.hpp"
#include "string_lib_test.hpp"
#include "math_lib_test.hpp"
#include "table_lib_test.hpp"
#include "io_lib_test.hpp"
#include "os_lib_test.hpp"
#include "debug_lib_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Standard library test functions (MODULE level)
 *
 * Coordinates all standard library related tests, following the hierarchical calling pattern:
 * MODULE (runLibTests) → SUITE (individual library test functions)
 *
 * This MODULE level test only calls SUITE level test functions.
 *
 * Test coverage includes:
 * - BaseLib: Basic library tests
 * - StringLib: String library tests
 * - TableLib: Table library tests
 * - MathLib: Math library tests
 * - IOLib: IO library tests
 * - OSLib: OS library tests
 * - DebugLib: Debug library tests
 */

/**
 * @brief Standard library test module (MODULE level)
 *
 * Execute all test suites in the standard library module.
 * Following the hierarchical calling pattern: MODULE → SUITE
 *
 * @note Only calls SUITE level test classes
 * @note Does not call GROUP or INDIVIDUAL level tests directly
 */
class LibTestModule {
public:
    /**
     * @brief Run all standard library tests
     */
    static void runAllTests() {
        // MODULE level only calls SUITE level test classes
        RUN_TEST_SUITE(BaseLibTestSuite);
        RUN_TEST_SUITE(StringLibTestSuite);
        RUN_TEST_SUITE(MathLibTestSuite);
        RUN_TEST_SUITE(TableLibTestSuite);
        RUN_TEST_SUITE(IOLibTestSuite);
        RUN_TEST_SUITE(OSLibTestSuite);
        RUN_TEST_SUITE(DebugLibTestSuite);
    }
};

} // namespace Tests
} // namespace Lua
