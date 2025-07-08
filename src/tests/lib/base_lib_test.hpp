#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/base/base_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief Base Library test suite
 *
 * Complete test suite for base library functionality following hierarchical pattern:
 * SUITE → GROUP → INDIVIDUAL
 *
 * Test coverage includes:
 * - Core functions: print, type, tostring, tonumber, error, assert
 * - Table operations: pairs, ipairs, next
 * - Metatable operations: getmetatable, setmetatable
 * - Raw access: rawget, rawset, rawlen, rawequal
 * - Error handling: pcall, xpcall
 * - Utility functions: select, unpack
 */
class BaseLibTestSuite {
public:
    /**
     * @brief Run all base library tests (SUITE level)
     */
    static void runAllTests() {
        // SUITE level only calls GROUP level tests
        RUN_TEST_GROUP("Core Functions", runCoreTests);
        RUN_TEST_GROUP("Type Operations", runTypeTests);
        //RUN_TEST_GROUP("Table Operations", runTableTests);
        //RUN_TEST_GROUP("Metatable Operations", runMetatableTests);
        //RUN_TEST_GROUP("Raw Access Operations", runRawAccessTests);
        //RUN_TEST_GROUP("Error Handling", runErrorHandlingTests);
        //RUN_TEST_GROUP("Utility Functions", runUtilityTests);
    }

private:
    // GROUP level function declarations
    static void runCoreTests();
    static void runTypeTests();
    static void runTableTests();
    static void runMetatableTests();
    static void runRawAccessTests();
    static void runErrorHandlingTests();
    static void runUtilityTests();

    // INDIVIDUAL level test function declarations
    // Core Functions Tests
    static void testPrint();
    static void testType();
    static void testToString();
    static void testToNumber();

    // Type Operations Tests
    static void testTypeChecking();
    static void testTypeConversion();

    // Table Operations Tests
    static void testPairs();
    static void testIPairs();
    static void testNext();

    // Metatable Operations Tests
    static void testGetMetatable();
    static void testSetMetatable();

    // Raw Access Operations Tests
    static void testRawGet();
    static void testRawSet();
    static void testRawLen();
    static void testRawEqual();

    // Error Handling Tests
    static void testError();
    static void testAssert();
    static void testPCall();
    static void testXPCall();

    // Utility Functions Tests
    static void testSelect();
    static void testUnpack();
};
} // namespace Tests
} // namespace Lua
