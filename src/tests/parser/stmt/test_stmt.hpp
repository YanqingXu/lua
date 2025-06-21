#pragma once

#include "return_stmt_test.hpp"
#include "../../test_utils.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Statement Test Suite
 * 
 * This class serves as the SUITE-level test coordinator for all statement parsing functionality.
 * It organizes and runs various test groups that cover different types of statements.
 * 
 * Test Hierarchy within Statement Suite:
 * SUITE (StmtTestSuite) -> GROUP (ReturnStmtTest, IfStmtTest, etc.) -> INDIVIDUAL
 * 
 * The statement suite includes:
 * - Return statement parsing tests (ReturnStmtTest)
 * - Future: If statement tests, While statement tests, etc.
 */
class StmtTestSuite {
public:
    /**
     * @brief Run all statement parsing tests
     * 
     * Executes all statement-related test groups in a logical order.
     * This method coordinates the execution of various test groups within the statement suite.
     * 
     * Current test groups:
     * - ReturnStmtTest: Tests for return statement parsing
     * 
     * Future test groups (to be implemented):
     * - IfStmtTest: Tests for if statement parsing
     * - WhileStmtTest: Tests for while statement parsing
     * - ForStmtTest: Tests for for statement parsing
     */
    static void runAllTests() {
        // Run return statement test group
        RUN_TEST_GROUP("Return Statement Tests", ReturnStmtTest);
        
        // TODO: Add other statement test groups as they are implemented
        // RUN_TEST_GROUP("If Statement Tests", IfStmtTest);
        // RUN_TEST_GROUP("While Statement Tests", WhileStmtTest);
        // RUN_TEST_GROUP("For Statement Tests", ForStmtTest);
    }
};

} // namespace Tests
} // namespace Lua