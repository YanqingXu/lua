/**
 * @file repl_compatibility_test.hpp
 * @brief Lua 5.1 REPL compatibility test suite
 * 
 * This file contains comprehensive tests to verify that our REPL implementation
 * matches the behavior of the official Lua 5.1 REPL as closely as possible.
 */

#pragma once

#include "../../common/types.hpp"
#include "../../vm/lua_state.hpp"
#include "../../vm/global_state.hpp"
#include <iostream>
#include <sstream>
#include <memory>

namespace Lua {
namespace Test {

/**
 * @brief REPL compatibility test suite
 * 
 * Tests various aspects of REPL functionality including:
 * - Command line argument processing
 * - =expression syntax sugar
 * - Multi-line input handling
 * - Error reporting
 * - Signal handling
 * - Environment variable support
 */
class REPLCompatibilityTest {
public:
    /**
     * @brief Run all REPL compatibility tests
     */
    static void runAllTests();

private:
    /**
     * @brief Test command line argument parsing
     */
    static void testCommandLineArguments();

    /**
     * @brief Test =expression syntax sugar
     */
    static void testExpressionSyntaxSugar();

    /**
     * @brief Test incomplete input detection
     */
    static void testIncompleteInputDetection();

    /**
     * @brief Test error reporting format
     */
    static void testErrorReporting();

    /**
     * @brief Test prompt customization
     */
    static void testPromptCustomization();

    /**
     * @brief Test environment variable handling
     */
    static void testEnvironmentVariables();

    /**
     * @brief Test arg global table setup
     */
    static void testArgGlobalTable();

    /**
     * @brief Test input length limits
     */
    static void testInputLengthLimits();

    /**
     * @brief Helper function to simulate REPL input
     */
    static Str simulateREPLInput(const Str& input);

    /**
     * @brief Helper function to check if input is incomplete
     */
    static bool checkIncompleteInput(const Str& code);

    /**
     * @brief Print test result
     */
    static void printTestResult(const Str& testName, bool passed, const Str& details = "");
};

} // namespace Test
} // namespace Lua
