#ifndef TEST_PARSER_EXPR_HPP
#define TEST_PARSER_EXPR_HPP

#include "../../../test_framework/core/test_macros.hpp"
#include "parser_binary_expr_test.hpp"
#include "parser_unary_expr_test.hpp"
#include "parser_literal_expr_test.hpp"
#include "parser_call_expr_test.hpp"
#include "parser_table_expr_test.hpp"
#include "parser_member_expr_test.hpp"
#include "parser_variable_expr_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Parser Expression Test Suite
 * 
 * Coordinates all parser expression-related tests
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Comprehensive test suite for all expression parsing functionality
 * including literals, variables, operators, function calls, tables, and member access
 */
class ParserExprTestSuite {
public:
    /**
     * @brief Run all parser expression tests
     * 
     * Execute all expression parsing test suites
     * using standardized test macros for consistent output formatting
     */
    static void runAllTests() {
        RUN_TEST_SUITE(Lua::Tests::ParserBinaryExprTest);
        RUN_TEST_SUITE(Lua::Tests::ParserUnaryExprTest);
        RUN_TEST_SUITE(Lua::Tests::ParserCallExprTest);
        RUN_TEST_SUITE(Lua::Tests::ParserTableExprTest);
        RUN_TEST_SUITE(Lua::Tests::ParserMemberExprTest);
        RUN_TEST_SUITE(Lua::Tests::ParserLiteralExprTest);
        RUN_TEST_SUITE(Lua::Tests::ParserVariableExprTest);
    }
};

} // namespace Tests
} // namespace Lua

#endif // TEST_EXPR_HPP