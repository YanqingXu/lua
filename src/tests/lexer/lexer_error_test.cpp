#include "lexer_error_test.hpp"

namespace Lua {
namespace Tests {

void LexerErrorTest::runAllTests() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::GROUP, "Lexer Error Handling Tests", 
                               "Testing lexer error detection and handling");
    
    // Run test groups
    RUN_TEST_GROUP("Invalid Characters", testInvalidCharacters);
    RUN_TEST_GROUP("String Errors", testStringErrors);
    RUN_TEST_GROUP("Number Errors", testNumberErrors);
    RUN_TEST_GROUP("Escape Sequence Errors", testEscapeSequenceErrors);
    RUN_TEST_GROUP("Edge Cases", testEdgeCases);
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::GROUP, "Lexer Error Handling Tests completed");
}

void LexerErrorTest::testInvalidCharacters() {
    SAFE_RUN_TEST(LexerErrorTest, testInvalidSymbols);
    SAFE_RUN_TEST(LexerErrorTest, testInvalidUnicodeCharacters);
    SAFE_RUN_TEST(LexerErrorTest, testControlCharacters);
}

void LexerErrorTest::testStringErrors() {
    SAFE_RUN_TEST(LexerErrorTest, testUnterminatedString);
    SAFE_RUN_TEST(LexerErrorTest, testUnterminatedMultilineString);
    SAFE_RUN_TEST(LexerErrorTest, testInvalidStringEscapes);
}

void LexerErrorTest::testNumberErrors() {
    SAFE_RUN_TEST(LexerErrorTest, testMalformedNumbers);
    SAFE_RUN_TEST(LexerErrorTest, testInvalidHexNumbers);
    SAFE_RUN_TEST(LexerErrorTest, testNumberOverflow);
}

void LexerErrorTest::testEscapeSequenceErrors() {
    SAFE_RUN_TEST(LexerErrorTest, testInvalidEscapeSequences);
    SAFE_RUN_TEST(LexerErrorTest, testIncompleteEscapeSequences);
}

void LexerErrorTest::testEdgeCases() {
    SAFE_RUN_TEST(LexerErrorTest, testEmptyInput);
    SAFE_RUN_TEST(LexerErrorTest, testOnlyWhitespace);
    SAFE_RUN_TEST(LexerErrorTest, testVeryLongTokens);
    SAFE_RUN_TEST(LexerErrorTest, testMixedValidInvalidTokens);
}

// Individual test implementations
void LexerErrorTest::testInvalidSymbols() {
    std::string source = "@#$%^&*";
    bool hasError = containsErrorToken(source);
    printTestResult("Invalid symbols detection", hasError);
}

void LexerErrorTest::testInvalidUnicodeCharacters() {
    std::string source = "local x = \u0001\u0002";
    bool hasError = containsErrorToken(source);
    printTestResult("Invalid unicode characters", hasError);
}

void LexerErrorTest::testControlCharacters() {
    std::string source = "local x\x01\x02 = 1";
    bool hasError = containsErrorToken(source);
    printTestResult("Control characters detection", hasError);
}

void LexerErrorTest::testUnterminatedString() {
    std::string source = R"(local x = "unterminated string)";
    bool hasError = containsErrorToken(source);
    printTestResult("Unterminated string detection", hasError);
}

void LexerErrorTest::testUnterminatedMultilineString() {
    std::string source = R"(local x = [[unterminated
multiline string)";
    bool hasError = containsErrorToken(source);
    printTestResult("Unterminated multiline string", hasError);
}

void LexerErrorTest::testInvalidStringEscapes() {
    std::string source = R"(local x = "invalid \q escape")";
    bool hasError = containsErrorToken(source);
    printTestResult("Invalid string escapes", hasError);
}

void LexerErrorTest::testMalformedNumbers() {
    std::string source = "local x = 123.456.789";
    bool hasError = containsErrorToken(source);
    printTestResult("Malformed numbers detection", hasError);
}

void LexerErrorTest::testInvalidHexNumbers() {
    std::string source = "local x = 0xGHI";
    bool hasError = containsErrorToken(source);
    printTestResult("Invalid hex numbers", hasError);
}

void LexerErrorTest::testNumberOverflow() {
    std::string source = "local x = 999999999999999999999999999999999999999";
    bool hasError = lexAndCheckError(source);
    printTestResult("Number overflow detection", hasError);
}

void LexerErrorTest::testInvalidEscapeSequences() {
    std::string source = R"(local x = "\z invalid")";
    bool hasError = containsErrorToken(source);
    printTestResult("Invalid escape sequences", hasError);
}

void LexerErrorTest::testIncompleteEscapeSequences() {
    std::string source = R"(local x = "incomplete \)";
    bool hasError = containsErrorToken(source);
    printTestResult("Incomplete escape sequences", hasError);
}

void LexerErrorTest::testEmptyInput() {
    std::string source = "";
    
    Lexer lexer(source);
    Token token = lexer.nextToken();
    bool isEOF = (token.type == TokenType::Eof);
    printTestResult("Empty input handling", isEOF);
}

void LexerErrorTest::testOnlyWhitespace() {
    std::string source = "   \t\n\r   ";
    
    Lexer lexer(source);
    Token token = lexer.nextToken();
    bool isEOF = (token.type == TokenType::Eof);
    printTestResult("Whitespace-only input", isEOF);
}

void LexerErrorTest::testVeryLongTokens() {
    std::string source = "local ";
    // Create a very long identifier
    for (int i = 0; i < 10000; ++i) {
        source += "a";
    }
    source += " = 1";
    
    Lexer lexer(source);
    Token token;
    bool foundLongToken = false;

    // Scan through tokens
    do {
        token = lexer.nextToken();
        if (token.type == TokenType::Name && token.lexeme.length() > 1000) {
            foundLongToken = true;
            break;
        }
    } while (token.type != TokenType::Eof);

    printTestResult("Very long tokens handling", foundLongToken);
}

void LexerErrorTest::testMixedValidInvalidTokens() {
    std::string source = "local x = 123 @ invalid $ more @ errors";
    
    int errorCount = countErrorTokens(source);
    bool hasMultipleErrors = errorCount >= 2;
    printTestResult("Mixed valid/invalid tokens", hasMultipleErrors);
}

// Helper method implementations
void LexerErrorTest::printTestResult(const std::string& testName, bool passed) {
    TestUtils::printTestResult(testName, passed);
}

bool LexerErrorTest::lexAndCheckError(const std::string& source, TokenType expectedErrorType) {
    Lexer lexer(source);
    Token token;

    // Scan through all tokens
    do {
        token = lexer.nextToken();
        if (token.type == expectedErrorType) {
            return true;
        }
    } while (token.type != TokenType::Eof);

    return false;
}

bool LexerErrorTest::containsErrorToken(const std::string& source) {
    return lexAndCheckError(source, TokenType::Error);
}

int LexerErrorTest::countErrorTokens(const std::string& source) {
    Lexer lexer(source);
    Token token;
    int errorCount = 0;

    // Scan through all tokens and count errors
    do {
        token = lexer.nextToken();
        if (token.type == TokenType::Error) {
            errorCount++;
        }
    } while (token.type != TokenType::Eof);

    return errorCount;
}

} // namespace Tests
} // namespace Lua