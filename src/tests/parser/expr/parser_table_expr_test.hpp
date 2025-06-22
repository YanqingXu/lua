#ifndef TABLE_EXPR_TEST_HPP
#define TABLE_EXPR_TEST_HPP

#include <iostream>
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"`

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

/**
 * @brief Table Expression Parser Test Class
 * 
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Tests parsing of table constructor expressions including:
 * - Empty tables
 * - Array-style initialization
 * - Hash-style initialization
 * - Mixed initialization patterns
 * - Nested table structures
 */
class TableExprTest {
public:
    /**
     * @brief Run all table expression tests
     * 
     * Executes all test cases for table expression parsing
     */
    static void runAllTests();
    
private:
    // Basic table tests
    static void testEmptyTables();
    static void testArrayTables();
    static void testHashTables();
    static void testMixedTables();
    
    // Array-style table tests
    static void testArrayWithLiterals();
    static void testArrayWithVariables();
    static void testArrayWithExpressions();
    static void testArrayWithMixedElements();
    
    // Hash-style table tests
    static void testHashWithStringKeys();
    static void testHashWithIdentifierKeys();
    static void testHashWithExpressionKeys();
    static void testHashWithMixedKeys();
    
    // Value type tests
    static void testTablesWithLiteralValues();
    static void testTablesWithVariableValues();
    static void testTablesWithExpressionValues();
    static void testTablesWithFunctionCallValues();
    
    // Complex table tests
    static void testNestedTables();
    static void testTablesWithComplexExpressions();
    static void testTablesWithUnaryExpressions();
    static void testTablesWithBinaryExpressions();
    
    // Special syntax tests
    static void testTablesWithTrailingCommas();
    static void testTablesWithSemicolons();
    static void testTablesWithMixedSeparators();
    static void testTablesWithWhitespace();
    
    // Edge case tests
    static void testLargeTables();
    static void testTablesWithComplexKeys();
    static void testTablesWithDuplicateKeys();
    static void testTablesInExpressions();
    
    // Error handling tests
    static void testInvalidTableSyntax();
    static void testMalformedTableElements();
    static void testUnterminatedTables();
    static void testInvalidKeySyntax();
    
    // Helper methods
    static void testTableParsing(const std::string& input, int expectedElementCount, const std::string& testName);
    static void testTableParsingError(const std::string& input, const std::string& testName);
    //static bool verifyTableExpression(const Expr* expr, int expectedElementCount);
    static void printTableExpressionInfo(const TableExpr* tableExpr);
    static std::string getTableElementType(const TableField& element);
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // TABLE_EXPR_TEST_HPP