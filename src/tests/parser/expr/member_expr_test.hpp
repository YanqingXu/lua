#ifndef MEMBER_EXPR_TEST_HPP
#define MEMBER_EXPR_TEST_HPP

#include <iostream>
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../test_utils.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Member Expression Parser Test Class
 * 
 * Tests parsing of member access expressions including:
 * - Dot notation (obj.property)
 * - Bracket notation (obj[key])
 * - Chained member access
 * - Member access with complex expressions
 * - Method access patterns
 */
class MemberExprTest {
public:
    /**
     * @brief Run all member expression tests
     * 
     * Executes all test cases for member expression parsing
     */
    static void runAllTests();
    
private:
    // Basic member access tests
    static void testDotNotation();
    static void testBracketNotation();
    static void testSimpleMemberAccess();
    
    // Dot notation tests
    static void testDotWithIdentifiers();
    static void testDotWithComplexObjects();
    static void testDotWithReservedWords();
    static void testDotWithUnderscoreNames();
    
    // Bracket notation tests
    static void testBracketWithStringKeys();
    static void testBracketWithNumericKeys();
    static void testBracketWithVariableKeys();
    static void testBracketWithExpressionKeys();
    
    // Chained member access tests
    static void testChainedDotAccess();
    static void testChainedBracketAccess();
    static void testMixedChainedAccess();
    static void testDeepChainedAccess();
    
    // Complex object tests
    static void testMemberAccessOnFunctionCalls();
    static void testMemberAccessOnTableConstructors();
    static void testMemberAccessOnParenthesizedExpressions();
    static void testMemberAccessOnComplexExpressions();
    
    // Key expression tests
    static void testBracketWithArithmeticKeys();
    static void testBracketWithLogicalKeys();
    static void testBracketWithComparisonKeys();
    static void testBracketWithUnaryKeys();
    
    // Member access in expressions tests
    static void testMemberAccessInBinaryExpressions();
    static void testMemberAccessInUnaryExpressions();
    static void testMemberAccessInFunctionCalls();
    static void testMemberAccessInTableConstructors();
    
    // Special cases tests
    static void testMemberAccessWithWhitespace();
    static void testMemberAccessWithComments();
    static void testMemberAccessWithNewlines();
    static void testMemberAccessPrecedence();
    
    // Error handling tests
    static void testInvalidDotNotation();
    static void testInvalidBracketNotation();
    static void testMalformedMemberAccess();
    static void testUnterminatedBracketAccess();
    
    // Helper methods
    static void testMemberParsing(const std::string& input, const std::string& expectedObject, const std::string& expectedMember, const std::string& testName);
    static void testBracketMemberParsing(const std::string& input, const std::string& expectedObject, const std::string& testName);
    static void testMemberParsingError(const std::string& input, const std::string& testName);
    static bool verifyMemberExpression(const Expr* expr, const std::string& expectedObject, const std::string& expectedMember);
    static bool verifyBracketMemberExpression(const Expr* expr, const std::string& expectedObject);
    static void printMemberExpressionInfo(const MemberExpr* memberExpr);
    static std::string extractObjectName(const Expr* expr);
};

} // namespace Tests
} // namespace Lua

#endif // MEMBER_EXPR_TEST_HPP