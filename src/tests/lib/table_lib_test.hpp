#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/table/table_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief Table Library test suite
 *
 * Complete test suite for table library functionality following hierarchical pattern:
 * SUITE �?GROUP �?INDIVIDUAL
 */
class TableLibTestSuite {
public:
    /**
     * @brief Run all table library tests (SUITE level)
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Table Operations", runTableOperationsTests);
        RUN_TEST_GROUP("Length Operations", runLengthTests);
        RUN_TEST_GROUP("Error Handling", runTableErrorHandlingTests);
    }

private:
    // GROUP level function declarations
    static void runTableOperationsTests();
    static void runLengthTests();
    static void runTableErrorHandlingTests();

    // INDIVIDUAL level test function declarations
    static void testInsert();
    static void testRemove();
    static void testSort();
    static void testConcat();
    static void testGetn();
    static void testMaxn();
    static void testErrorHandling();
    static void testEdgeCases();
};

} // namespace Tests
} // namespace Lua
