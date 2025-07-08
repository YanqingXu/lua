#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/io/io_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief IO Library test suite
 *
 * Complete test suite for io library functionality following hierarchical pattern:
 * SUITE �?GROUP �?INDIVIDUAL
 */
class IOLibTestSuite {
public:
    /**
     * @brief Run all io library tests (SUITE level)
     */
    static void runAllTests() {
        RUN_TEST_GROUP("File Operations", runFileOperationsTests);
        RUN_TEST_GROUP("Stream Operations", runStreamOperationsTests);
        RUN_TEST_GROUP("Error Handling", runIOErrorHandlingTests);
    }

private:
    // GROUP level function declarations
    static void runFileOperationsTests();
    static void runStreamOperationsTests();
    static void runIOErrorHandlingTests();

    // INDIVIDUAL level test function declarations
    // File Operations Tests
    static void testOpen();
    static void testClose();
    static void testRead();
    static void testWrite();

    // Stream Operations Tests
    static void testFlush();
    static void testLines();
    static void testInput();
    static void testOutput();
    static void testType();

    // Error Handling Tests
    static void testErrorHandling();
    static void testNullStateHandling();
};

} // namespace Tests
} // namespace Lua
