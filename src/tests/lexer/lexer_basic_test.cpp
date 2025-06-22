#include "lexer_basic_test.hpp"

namespace Lua {
namespace Tests {

// LexerBasicTest implementation
void LexerBasicTest::runAllTests() {
    // Run basic lexing tests
    RUN_TEST_GROUP("Basic Lexing", testBasicLexing);
    
    // Run token recognition tests
    RUN_TEST_GROUP("Token Recognition", testTokenRecognition);
}

void LexerBasicTest::testBasicLexing() {
    TestUtils::printInfo("Testing basic lexical analysis...");
    
    try {
        testLexer("local x = 42 + 3.14");
        TestUtils::printTestResult("Basic lexing", true);
    } catch (const std::exception& e) {
        TestUtils::printTestResult("Basic lexing", false);
        TestUtils::printError("Basic lexing failed: " + std::string(e.what()));
    }
}

void LexerBasicTest::testTokenRecognition() {
    TestUtils::printInfo("Testing token recognition...");
    
    try {
        // Test various token types
        testLexer("if then else end while do for in function local return");
        testLexer("+ - * / % ^ == ~= < <= > >= and or not");
        testLexer("( ) [ ] { } ; , . .. ...");
        testLexer("\"string\" 'string' [[multiline]] 123 3.14 0xFF");
        
        TestUtils::printTestResult("Token recognition", true);
    } catch (const std::exception& e) {
        TestUtils::printTestResult("Token recognition", false);
        TestUtils::printError("Token recognition failed: " + std::string(e.what()));
    }
}

void LexerBasicTest::testLexer(const std::string& source) {
    TestUtils::printInfo("Lexing source: " + source);

    Lexer lexer(source);
    Token token;

    do {
        token = lexer.nextToken();
        // Only print detailed token info in debug mode
        // std::cout << "Token: " << static_cast<int>(token.type) << " Lexeme: " << token.lexeme 
        //     << " Line: " << token.line << " Column: " << token.column << std::endl;
    } while (token.type != TokenType::Eof && token.type != TokenType::Error);
}

// Legacy LexerTest implementation for backward compatibility
void LexerTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Legacy Lexer Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testLexer("local x = 42 + 3.14");
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Legacy Lexer Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void LexerTest::testLexer(const std::string& source) {
    std::cout << "Lexer Test:" << std::endl;
    std::cout << "Source: " << source << std::endl;

    Lexer lexer(source);
    Token token;

    do {
        token = lexer.nextToken();
        std::cout << "Token: " << static_cast<int>(token.type) << " Lexeme: " << token.lexeme 
            << " Line: " << token.line << " Column: " << token.column << std::endl;
    } while (token.type != TokenType::Eof && token.type != TokenType::Error);
}

} // namespace Tests
} // namespace Lua