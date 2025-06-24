#pragma once

#include "../../test_framework/core/test_utils.hpp"

namespace Lua::Tests {

/**
 * @brief Test class for conditional statement compilation functionality
 * 
 * Tests if-then-else statements, short-circuit logical operators, and nested conditions
 */
class CompilerConditionalTest {
public:
    /**
     * @brief Run all conditional compilation tests
     * 
     * Execute all test groups in this test suite
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Conditionals", testBasicConditionals);
        RUN_TEST_GROUP("Short Circuit Operations", testShortCircuitOperations);
        RUN_TEST_GROUP("Complex Conditions", testComplexConditions);
    }
    
private:
    /**
     * @brief Test basic conditional statements
     */
    static void testBasicConditionals() {
        RUN_TEST(CompilerConditionalTest, testSimpleIfStatement);
        RUN_TEST(CompilerConditionalTest, testIfElseStatement);
        RUN_TEST(CompilerConditionalTest, testNestedIfStatement);
    }
    
    /**
     * @brief Test short-circuit logical operations
     */
    static void testShortCircuitOperations() {
        RUN_TEST(CompilerConditionalTest, testShortCircuitAnd);
        RUN_TEST(CompilerConditionalTest, testShortCircuitOr);
    }
    
    /**
     * @brief Test complex conditional expressions
     */
    static void testComplexConditions() {
        RUN_TEST(CompilerConditionalTest, testComplexConditionCombinations);
    }
    
    /**
     * @brief Test simple if statement compilation
     */
    static void testSimpleIfStatement();
    
    /**
     * @brief Test if-else statement compilation
     */
    static void testIfElseStatement();
    
    /**
     * @brief Test nested if statement compilation
     */
    static void testNestedIfStatement();
    
    /**
     * @brief Test short-circuit AND operator compilation
     */
    static void testShortCircuitAnd();
    
    /**
     * @brief Test short-circuit OR operator compilation
     */
    static void testShortCircuitOr();
    
    /**
     * @brief Test complex conditional expressions
     */
    static void testComplexConditionCombinations();
};

} // namespace Lua::Tests