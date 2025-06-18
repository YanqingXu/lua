#pragma once

// Include all parser test headers
#include "expr/test_expr.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Parser Test Suite
 * 
 * This class provides a unified interface to run all parser-related tests.
 * It includes tests for basic parsing, function definitions, control structures,
 * and various statement types.
 */
class ParserTestSuite {
public:
    /**
     * @brief Run all parser tests
     * 
     * Executes all parser-related test suites in a logical order.
     * Tests are run from basic parsing to complex language constructs.
     */
    static void runAllTests() {
        RUN_TEST_SUITE(ExprTestSuite);
    }
};

} // namespace Tests
} // namespace Lua
