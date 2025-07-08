#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/debug/debug_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief Debug Library test suite
 *
 * Complete test suite for debug library functionality following hierarchical pattern:
 * SUITE �?GROUP �?INDIVIDUAL
 */
class DebugLibTestSuite {
public:
    /**
     * @brief Run all debug library tests (SUITE level)
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Debug Functions", runDebugFunctionsTests);
        RUN_TEST_GROUP("Environment", runEnvironmentTests);
        RUN_TEST_GROUP("Hooks", runHookTests);
        RUN_TEST_GROUP("Variables", runVariableTests);
        RUN_TEST_GROUP("Metatable", runDebugMetatableTests);
        RUN_TEST_GROUP("Error Handling", runDebugErrorHandlingTests);
    }

private:
    // GROUP level function declarations
    static void runDebugFunctionsTests();
    static void runEnvironmentTests();
    static void runHookTests();
    static void runVariableTests();
    static void runDebugMetatableTests();
    static void runDebugErrorHandlingTests();

    // INDIVIDUAL level test function declarations
    // Debug Functions Tests
    static void testDebug();
    static void testTraceback();
    static void testGetinfo();

    // Environment Tests
    static void testGetfenv();
    static void testSetfenv();

    // Hook Tests
    static void testGethook();
    static void testSethook();

    // Variable Tests
    static void testGetlocal();
    static void testSetlocal();
    static void testGetupvalue();
    static void testSetupvalue();

    // Metatable Tests
    static void testGetmetatable();
    static void testSetmetatable();
    static void testGetregistry();

    // Error Handling Tests
    static void testErrorHandling();
    static void testNullStateHandling();
};

} // namespace Tests
} // namespace Lua
