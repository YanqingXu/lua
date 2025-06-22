#ifndef TEST_EXPR_HPP
#define TEST_EXPR_HPP

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
namespace Parser {
namespace Expr {

/**
 * @brief Parser Expression Test Suite
 * 
 * Coordinates all parser expression-related tests
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Comprehensive test suite for all expression parsing functionality
 * including literals, variables, operators, function calls, tables, and member access
 */
class ExprTestSuite {
public:
    /**
     * @brief Run all parser expression tests
     * 
     * Execute all expression parsing test suites
     * using standardized test macros for consistent output formatting
     */
    static void runAllTests() {
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::BinaryExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::UnaryExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::CallExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::TableExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::MemberExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::LiteralExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::VariableExprTest);
    }
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // TEST_EXPR_HPP