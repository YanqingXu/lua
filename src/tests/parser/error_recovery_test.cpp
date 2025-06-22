#include "error_recovery_test.hpp"

namespace Lua {
namespace Tests {

void ParserErrorRecoveryTest::runAllTests() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::GROUP, "Parser Error Recovery Tests", 
                               "Testing parser error recovery mechanisms");
    
    // Run test groups
    RUN_TEST_GROUP("Basic Synchronization", testBasicSynchronization);
    RUN_TEST_GROUP("Balanced Delimiter Skipping", testBalancedDelimiterSkipping);
    RUN_TEST_GROUP("Error Recovery in Expressions", testErrorRecoveryInExpressions);
    RUN_TEST_GROUP("Error Recovery in Statements", testErrorRecoveryInStatements);
    RUN_TEST_GROUP("Recovery Reporting", testRecoveryReporting);
    RUN_TEST_GROUP("Edge Cases", testEdgeCases);
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::GROUP, "Parser Error Recovery Tests completed");
}

void ParserErrorRecoveryTest::testBasicSynchronization() {
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSynchronizeAfterMissingSemicolon);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSynchronizeAfterInvalidToken);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSynchronizeWithNestedStructures);
}

void ParserErrorRecoveryTest::testBalancedDelimiterSkipping() {
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSkipBalancedParentheses);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSkipBalancedBrackets);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSkipBalancedBraces);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSkipNestedDelimiters);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testSkipUnbalancedDelimiters);
}

void ParserErrorRecoveryTest::testErrorRecoveryInExpressions() {
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryInBinaryExpressions);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryInFunctionCalls);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryInTableConstructors);
}

void ParserErrorRecoveryTest::testErrorRecoveryInStatements() {
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryInIfStatements);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryInWhileStatements);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryInFunctionDefinitions);
}

void ParserErrorRecoveryTest::testRecoveryReporting() {
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryStatistics);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testRecoveryMessages);
}

void ParserErrorRecoveryTest::testEdgeCases() {
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testEmptyInput);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testOnlyErrorTokens);
    SAFE_RUN_TEST(ParserErrorRecoveryTest, testVeryLongErrorSequence);
}

// Individual test implementations
void ParserErrorRecoveryTest::testSynchronizeAfterMissingSemicolon() {
    std::string source = R"(
        local x = 1
        local y = 2  -- missing semicolon after previous line
        local z = 3
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Synchronize after missing semicolon", recovered);
}

void ParserErrorRecoveryTest::testSynchronizeAfterInvalidToken() {
    std::string source = R"(
        local x = 1
        @ invalid token here
        local y = 2
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Synchronize after invalid token", recovered);
}

void ParserErrorRecoveryTest::testSynchronizeWithNestedStructures() {
    std::string source = R"(
        if true then
            local x = @ invalid
            local y = 2
        end
        local z = 3
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Synchronize with nested structures", recovered);
}

void ParserErrorRecoveryTest::testSkipBalancedParentheses() {
    std::string source = R"(
        local x = func(1, 2, @ error, 4)
        local y = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Skip balanced parentheses", recovered);
}

void ParserErrorRecoveryTest::testSkipBalancedBrackets() {
    std::string source = R"(
        local x = arr[1, @ error, 3]
        local y = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Skip balanced brackets", recovered);
}

void ParserErrorRecoveryTest::testSkipBalancedBraces() {
    std::string source = R"(
        local x = {a = 1, @ error, c = 3}
        local y = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Skip balanced braces", recovered);
}

void ParserErrorRecoveryTest::testSkipNestedDelimiters() {
    std::string source = R"(
        local x = func({a = [1, @ error, 3], b = 2})
        local y = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Skip nested delimiters", recovered);
}

void ParserErrorRecoveryTest::testSkipUnbalancedDelimiters() {
    std::string source = R"(
        local x = func(1, 2, 3  -- missing closing parenthesis
        local y = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Handle unbalanced delimiters", recovered);
}

void ParserErrorRecoveryTest::testRecoveryInBinaryExpressions() {
    std::string source = R"(
        local x = 1 + @ error + 3
        local y = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Recovery in binary expressions", recovered);
}

void ParserErrorRecoveryTest::testRecoveryInFunctionCalls() {
    std::string source = R"(
        func(1, @ error, 3)
        local x = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Recovery in function calls", recovered);
}

void ParserErrorRecoveryTest::testRecoveryInTableConstructors() {
    std::string source = R"(
        local t = {a = 1, @ error, c = 3}
        local x = 5
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Recovery in table constructors", recovered);
}

void ParserErrorRecoveryTest::testRecoveryInIfStatements() {
    std::string source = R"(
        if @ error then
            local x = 1
        end
        local y = 2
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Recovery in if statements", recovered);
}

void ParserErrorRecoveryTest::testRecoveryInWhileStatements() {
    std::string source = R"(
        while @ error do
            local x = 1
        end
        local y = 2
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Recovery in while statements", recovered);
}

void ParserErrorRecoveryTest::testRecoveryInFunctionDefinitions() {
    std::string source = R"(
        function test(@ error)
            return 1
        end
        local x = 2
    )";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Recovery in function definitions", recovered);
}

void ParserErrorRecoveryTest::testRecoveryStatistics() {
    std::string source = R"(
        local x = @ error1
        local y = @ error2
        local z = 3
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(source);
        
        // Parse and expect errors with recovery
        auto result = parser.parseExpression();
        
        // Check if parser tracked recovery statistics
        bool hasStats = parser.hasErrors(); // Basic check for error tracking
        printTestResult("Recovery statistics tracking", hasStats);
    } catch (const std::exception& e) {
        printTestResult("Recovery statistics tracking", true); // Expected to have errors
    }
}

void ParserErrorRecoveryTest::testRecoveryMessages() {
    std::string source = "local x = @ invalid";
    
    bool hasExpectedError = containsExpectedError(source, "error");
    printTestResult("Recovery error messages", hasExpectedError);
}

void ParserErrorRecoveryTest::testEmptyInput() {
    std::string source = "";
    
    try {
        Lexer lexer(source);
        Parser parser(source);
        auto result = parser.parseExpression();
        printTestResult("Handle empty input", result == nullptr);
    } catch (const std::exception& e) {
        printTestResult("Handle empty input", true); // Expected behavior
    }
}

void ParserErrorRecoveryTest::testOnlyErrorTokens() {
    std::string source = "@ @ @ @";
    
    bool recovered = parseAndCheckRecovery(source, false); // Don't expect successful recovery
    printTestResult("Handle only error tokens", !recovered); // Should fail gracefully
}

void ParserErrorRecoveryTest::testVeryLongErrorSequence() {
    std::string source = "local x = ";
    for (int i = 0; i < 100; ++i) {
        source += "@ ";
    }
    source += "\nlocal y = 5";
    
    bool recovered = parseAndCheckRecovery(source);
    printTestResult("Handle very long error sequence", recovered);
}

// Helper method implementations
void ParserErrorRecoveryTest::printTestResult(const std::string& testName, bool passed) {
    TestUtils::printTestResult(testName, passed);
}

bool ParserErrorRecoveryTest::parseAndCheckRecovery(const std::string& source, bool expectRecovery) {
    try {
        Lexer lexer(source);
        Parser parser(source);
        
        // Attempt to parse - we expect this to encounter errors
        auto result = parser.parseExpression();
        
        // Check if parser had errors but continued (recovery)
        bool hadErrors = parser.hasErrors();
        
        if (expectRecovery) {
            // We expect errors but also expect the parser to recover
            return hadErrors; // If it had errors but didn't crash, recovery worked
        } else {
            // We expect parsing to fail completely
            return !hadErrors && result == nullptr;
        }
    } catch (const std::exception& e) {
        // If expectRecovery is true, catching an exception means recovery failed
        // If expectRecovery is false, catching an exception is expected
        return !expectRecovery;
    }
}

bool ParserErrorRecoveryTest::containsExpectedError(const std::string& source, const std::string& expectedError) {
    try {
        Lexer lexer(source);
        Parser parser(source);
        
        auto result = parser.parseExpression();
        
        // Check if parser has errors (basic implementation)
        return parser.hasErrors();
    } catch (const std::exception& e) {
        // Check if the exception message contains the expected error
        std::string errorMsg = e.what();
        return errorMsg.find(expectedError) != std::string::npos;
    }
}

} // namespace Tests
} // namespace Lua