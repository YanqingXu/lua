#include "lexer_test.hpp"

namespace Lua {
namespace Tests {

void testLexer(const std::string& source) {
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