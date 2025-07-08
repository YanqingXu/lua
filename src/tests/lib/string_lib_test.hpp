#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/string/string_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief String Library test suite
 *
 * Complete test suite for string library functionality following hierarchical pattern:
 * SUITE → GROUP → INDIVIDUAL
 *
 * Test coverage includes:
 * - Basic functions: len, sub, upper, lower, reverse
 * - Pattern matching: find, match, gmatch, gsub
 * - Formatting: format, rep
 * - Character operations: byte, char
 */
class StringLibTestSuite {
public:
    /**
     * @brief Run all string library tests (SUITE level)
     */
    static void runAllTests() {
        // SUITE level only calls GROUP level tests
        RUN_TEST_GROUP("Basic Functions", runBasicStringTests);
        RUN_TEST_GROUP("Pattern Matching", runPatternTests);
        RUN_TEST_GROUP("Formatting Functions", runFormattingTests);
        RUN_TEST_GROUP("Character Operations", runCharacterTests);
        RUN_TEST_GROUP("Error Handling", runStringErrorHandlingTests);
    }

private:
    // GROUP level function declarations
    static void runBasicStringTests();
    static void runPatternTests();
    static void runFormattingTests();
    static void runCharacterTests();
    static void runStringErrorHandlingTests();

    // INDIVIDUAL level test function declarations
    // Basic Functions Tests
    static void testLen();
    static void testSub();
    static void testUpper();
    static void testLower();
    static void testReverse();

    // Pattern Matching Tests
    static void testFind();
    static void testMatch();
    static void testGmatch();
    static void testGsub();

    // Formatting Tests
    static void testFormat();
    static void testRep();

    // Character Operations Tests
    static void testByte();
    static void testChar();

    // Error Handling Tests
    static void testErrorHandling();
    static void testEdgeCases();
};

} // namespace Tests
} // namespace Lua