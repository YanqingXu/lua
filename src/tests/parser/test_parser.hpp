#pragma once

// Include all parser test headers
#include "expr/test_parser_expr.hpp"
#include "stmt/test_stmt.hpp"
#include "error_recovery_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Parser Test Module
 * 
 * This class serves as the MODULE-level test coordinator for all parser-related functionality.
 * It organizes and runs various test suites that cover different aspects of parsing.
 * 
 * Test Hierarchy within Parser Module:
 * MODULE (ParserTestSuite) -> SUITE (ExprTestSuite, StmtTestSuite, etc.) -> GROUP -> INDIVIDUAL
 * 
 * The parser module includes:
 * - Expression parsing tests (ExprTestSuite)
 * - Statement parsing tests (future: StmtTestSuite)
 * - Function parsing tests (future: FuncTestSuite)
 * - Control structure parsing tests (future: ControlTestSuite)
 */
class ParserTestSuite {
public:
    /**
     * @brief Run all parser module tests
     * 
     * Executes all parser-related test suites in a logical order.
     * This method coordinates the execution of various test suites within the parser module.
     * 
     * Current test suites:
     * - ExprTestSuite: Tests for expression parsing
     * 
     * Future test suites (to be implemented):
     * - StmtTestSuite: Tests for statement parsing
     * - FuncTestSuite: Tests for function definition parsing
     * - ControlTestSuite: Tests for control structure parsing
     */
    static void runAllTests() {
        // Note: Test framework removed - tests disabled
        std::cout << "Parser tests disabled - test framework removed\n";

        // Individual test methods can be called directly if needed:
        // ParserExprTestSuite::runAllTests();
        // StmtTestSuite::runAllTests();
        // ParserErrorRecoveryTest::runAllTests();
        // FuncTestSuite::runAllTests();
        // ControlTestSuite::runAllTests();
    }
};

} // namespace Tests
} // namespace Lua
