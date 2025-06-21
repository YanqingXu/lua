#ifndef STATE_FUNCTION_TEST_HPP
#define STATE_FUNCTION_TEST_HPP

#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../../vm/function.hpp"
#include "../../../lib/base_lib.hpp"
#include "../../test_utils.hpp"
#include <cassert>
#include <stdexcept>

namespace Lua {
namespace Tests {

/**
 * @brief Individual test class for function call operations
 */
class StateFunctionTest {
public:
    // Native function tests
    static void testNativeFunctionCall();

    // Lua function tests
    static void testLuaFunctionCall();
    static void testFunctionArguments();
    static void testFunctionReturnValues();
    static void testFunctionErrorHandling();
    static void testNestedFunctionCalls();
    static void testFunctionScope();
    static void testFunctionClosures();

    // Code execution tests
    static void testDoStringBasic();
    static void testDoStringComplex();
    static void testDoStringErrors();
    static void testDoFileBasic();
    static void testCodeExecutionState();
    static void testBuiltinFunctions();
};

/**
 * @brief Function Call Test Suite
 * 
 * Tests comprehensive function call functionality including:
 * - Native function calls
 * - Lua function calls
 * - Function argument passing
 * - Return value handling
 * - Error handling in function calls
 * - Function call with different argument counts
 * - Function call edge cases
 */
class StateFunctionTestSuite {
public:
    /**
     * @brief Run all function call tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Native Function Tests", testNativeFunctions);
        RUN_TEST_GROUP("Function Call Tests", testFunctionCalls);
        RUN_TEST_GROUP("Function Error Handling", testFunctionErrorHandling);
        RUN_TEST_GROUP("Code Execution Tests", testCodeExecution);
    }

private:
    /**
     * @brief Test native function operations
     */
    static void testNativeFunctions() {
        RUN_TEST(StateFunctionTest, testNativeFunctionCall);
    }

    /**
     * @brief Test general function call mechanisms
     */
    static void testFunctionCalls() {
        RUN_TEST(StateFunctionTest, testLuaFunctionCall);
        RUN_TEST(StateFunctionTest, testFunctionArguments);
        RUN_TEST(StateFunctionTest, testFunctionReturnValues);
        RUN_TEST(StateFunctionTest, testNestedFunctionCalls);
        RUN_TEST(StateFunctionTest, testFunctionScope);
        RUN_TEST(StateFunctionTest, testFunctionClosures);
    }

    /**
     * @brief Test function error handling
     */
    static void testFunctionErrorHandling() {
        RUN_TEST(StateFunctionTest, testFunctionErrorHandling);
    }

    /**
     * @brief Test code execution functionality
     */
    static void testCodeExecution() {
        RUN_TEST(StateFunctionTest, testDoStringBasic);
        RUN_TEST(StateFunctionTest, testDoStringComplex);
        RUN_TEST(StateFunctionTest, testDoStringErrors);
        RUN_TEST(StateFunctionTest, testDoFileBasic);
        RUN_TEST(StateFunctionTest, testCodeExecutionState);
        RUN_TEST(StateFunctionTest, testBuiltinFunctions);
    }
};

} // namespace Tests
} // namespace Lua

#endif // STATE_FUNCTION_TEST_HPP