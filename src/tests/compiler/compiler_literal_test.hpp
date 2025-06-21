#pragma once

#include "../../common/types.hpp"
#include "../test_utils.hpp"

namespace Lua::Tests {

/**
 * @brief Test suite for literal compiler functionality
 * 
 * This test suite validates the compilation of various literal types
 * including nil, boolean, number, and string literals, as well as
 * constant table management and instruction generation.
 */
class CompilerLiteralTest {
public:
    /**
     * @brief Run all literal compiler tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP(CompilerLiteralTest, testBasicLiterals);
        RUN_TEST_GROUP(CompilerLiteralTest, testConstantManagement);
        RUN_TEST_GROUP(CompilerLiteralTest, testInstructionGeneration);
        RUN_TEST_GROUP(CompilerLiteralTest, testErrorHandling);
    }
    
    /**
     * @brief Test basic literal types compilation
     */
    static void testBasicLiterals();
    
    /**
     * @brief Test constant table management
     */
    static void testConstantManagement();
    
    /**
     * @brief Test instruction generation for literals
     */
    static void testInstructionGeneration();
    
    /**
     * @brief Test error handling in literal compilation
     */
    static void testErrorHandling();
    
private:
    // Legacy test methods - to be refactored
    static void testNilLiteral();
    static void testBooleanLiterals();
    static void testNumberLiterals();
    static void testStringLiterals();
    static void testComplexLiterals();
    static void testLiteralConstantTable();
    static void testRegisterAllocation();
};

} // namespace Lua::Tests