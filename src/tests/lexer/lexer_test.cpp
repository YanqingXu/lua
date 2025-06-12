#include "lexer_test.hpp"

namespace Lua {
namespace Tests {

void LexerTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Lexer Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testLexer("local x = 42 + 3.14");
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Lexer Tests Completed" << std::endl;
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