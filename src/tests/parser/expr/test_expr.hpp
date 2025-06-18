#ifndef TEST_EXPR_HPP
#define TEST_EXPR_HPP

#include "literal_expr_test.hpp"
#include "variable_expr_test.hpp"
#include "unary_expr_test.hpp"
#include "binary_expr_test.hpp"
#include "call_expr_test.hpp"
#include "table_expr_test.hpp"
#include "member_expr_test.hpp"
#include "../../test_utils.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Expression Parser Test Suite
 * 
 * Comprehensive test suite for all expression parsing functionality
 * including literals, variables, operators, function calls, tables, and member access
 */
class ExprTestSuite {
public:
    /**
     * @brief Run all expression parsing tests
     * 
     * Executes all expression-related test cases in a structured manner
     * using standardized test macros for consistent output formatting
     */
    static void runAllTests();
};

} // namespace Tests
} // namespace Lua

#endif // TEST_EXPR_HPP