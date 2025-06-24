#pragma once

#include "../../common/opcodes.hpp"
#include "../../parser/ast/expressions.hpp"
#include "../../lexer/lexer.hpp"
#include "../../test_framework/core/test_utils.hpp"

namespace Lua::Tests {

/**
 * @brief Test suite for binary expression compilation
 * 
 * This test suite validates the compilation of binary expressions
 * including arithmetic, comparison, logical operations, and
 * operator precedence handling.
 */
class CompilerBinaryExpressionTest {
public:
    /**
     * @brief Run all binary expression tests
     */
    static void runAllTests() {
        RUN_TEST_GROUP("Arithmetic Operations", testArithmeticOperations);
        RUN_TEST_GROUP("Comparison Operations", testComparisonOperations);
        RUN_TEST_GROUP("Logical Operations", testLogicalOperations);
        RUN_TEST_GROUP("Advanced Features", testAdvancedFeatures);
        RUN_TEST_GROUP("Error Handling", testErrorHandling);
    }
    
    /**
     * @brief Test arithmetic operations compilation
     */
    static void testArithmeticOperations();
    
    /**
     * @brief Test comparison operations compilation
     */
    static void testComparisonOperations();
    
    /**
     * @brief Test logical operations compilation
     */
    static void testLogicalOperations();
    
    /**
     * @brief Test advanced features like precedence and nesting
     */
    static void testAdvancedFeatures();
    
    /**
     * @brief Test error handling in binary expression compilation
     */
    static void testErrorHandling();
    
private:
    // Helper methods for testing specific operations
    static void testArithmeticOp(TokenType op, OpCode expectedOpCode);
    static void testComparisonOp(TokenType op, OpCode expectedOpCode);
    static void testStringConcatenation();
    static void testOperatorPrecedence();
    static void testNestedExpressions();
};

} // namespace Lua::Tests