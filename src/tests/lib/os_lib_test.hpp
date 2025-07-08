#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/os/os_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief OS Library test suite
 *
 * Complete test suite for os library functionality following hierarchical pattern:
 * SUITE �?GROUP �?INDIVIDUAL
 */
class OSLibTestSuite {
public:
    /**
     * @brief Run all os library tests (SUITE level)
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Time Operations", runTimeOperationsTests);
        RUN_TEST_GROUP("System Operations", runSystemOperationsTests);
        RUN_TEST_GROUP("File Operations", runOSFileOperationsTests);
        RUN_TEST_GROUP("Localization", runLocalizationTests);
        RUN_TEST_GROUP("Error Handling", runOSErrorHandlingTests);
    }

private:
    // GROUP level function declarations
    static void runTimeOperationsTests();
    static void runSystemOperationsTests();
    static void runOSFileOperationsTests();
    static void runLocalizationTests();
    static void runOSErrorHandlingTests();

    // INDIVIDUAL level test function declarations
    // Time Operations Tests
    static void testClock();
    static void testDate();
    static void testTime();
    static void testDifftime();

    // System Operations Tests
    static void testExecute();
    static void testExit();
    static void testGetenv();

    // File Operations Tests
    static void testRemove();
    static void testRename();
    static void testTmpname();

    // Localization Tests
    static void testSetlocale();

    // Error Handling Tests
    static void testErrorHandling();
    static void testNullStateHandling();
};

} // namespace Tests
} // namespace Lua
