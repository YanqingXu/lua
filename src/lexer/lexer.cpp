#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace Lua {
    // Keyword mapping table
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"and", TokenType::And},
        {"break", TokenType::Break},
        {"do", TokenType::Do},
        {"else", TokenType::Else},
        {"elseif", TokenType::Elseif},
        {"end", TokenType::End},
        {"false", TokenType::False},
        {"for", TokenType::For},
        {"function", TokenType::Function},
        {"if", TokenType::If},
        {"in", TokenType::In},
        {"local", TokenType::Local},
        {"nil", TokenType::Nil},
        {"not", TokenType::Not},
        {"or", TokenType::Or},
        {"repeat", TokenType::Repeat},
        {"return", TokenType::Return},
        {"then", TokenType::Then},
        {"true", TokenType::True},
        {"until", TokenType::Until},
        {"while", TokenType::While}
    };
    
    Lexer::Lexer(const Str& source)
        : source(source), current(0), start(0), line(1), column(1) {
    }
    
    Token Lexer::nextToken() {
        skipWhitespace();
        
        // Mark start position
        start = current;
        
        if (isAtEnd()) return makeToken(TokenType::Eof);
        
        char c = advance();
        
        // Identifier
        if (isalpha(c) || c == '_') return identifier();
        
        // Number
        if (isdigit(c)) return number();
        
        switch (c) {
            // Single character tokens
            case '(': return makeToken(TokenType::LeftParen);
            case ')': return makeToken(TokenType::RightParen);
            case '{': return makeToken(TokenType::LeftBrace);
            case '}': return makeToken(TokenType::RightBrace);
            case '[': return makeToken(TokenType::LeftBracket);
            case ']': return makeToken(TokenType::RightBracket);
            case ';': return makeToken(TokenType::Semicolon);
            case ':': return makeToken(TokenType::Colon);
            case ',': return makeToken(TokenType::Comma);
            case '+': return makeToken(TokenType::Plus);
            case '-': return makeToken(TokenType::Minus);
            case '*': return makeToken(TokenType::Star);
            case '/': return makeToken(TokenType::Slash);
            case '%': return makeToken(TokenType::Percent);
            case '^': return makeToken(TokenType::Caret);
            case '#': return makeToken(TokenType::Hash);
            
            // Possible double character tokens
            case '=':
                return makeToken(match('=') ? TokenType::Equal : TokenType::Assign);
            case '<':
                return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
            case '>':
                return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
            case '~':
                return makeToken(match('=') ? TokenType::NotEqual : TokenType::Error);
                
            // Dot operator (., .., ...)
            case '.':
                if (match('.')) {
                    if (match('.')) {
                        return makeToken(TokenType::DotDotDot);
                    } else {
                        return makeToken(TokenType::DotDot);
                    }
                } else {
                    return makeToken(TokenType::Dot);
                }
                
            // String
            case '"': return string();
            case '\'': return string();
                
            default:
                return errorToken("Unexpected character.");
        }
    }
    
    bool Lexer::isAtEnd() const {
        return current >= source.length();
    }
    
    char Lexer::advance() {
        column++;
        return source[current++];
    }
    
    char Lexer::peek() const {
        if (isAtEnd()) return '\0';
        return source[current];
    }
    
    char Lexer::peekNext() const {
        if (current + 1 >= source.length()) return '\0';
        return source[current + 1];
    }
    
    bool Lexer::match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        
        current++;
        column++;
        return true;
    }
    
    Token Lexer::makeToken(TokenType type) {
        Token token(type, source.substr(start, current - start), line, column - static_cast<int>(current - start));
        return token;
    }
    
    Token Lexer::errorToken(const Str& message) {
        Token token(TokenType::Error, message, line, column);
        return token;
    }
    
    void Lexer::skipWhitespace() {
        while (true) {
            char c = peek();
            
            switch (c) {
                case ' ':
                case '\r':
                case '\t':
                    advance();
                    break;
                case '\n':
                    line++;
                    column = 0;
                    advance();
                    break;
                case '-':
                    // Check if it's a comment (--...)
                    if (peekNext() == '-') {
                        // Comment until end of line
                        while (peek() != '\n' && !isAtEnd()) {
                            advance();
                        }
                    } else {
                        return;
                    }
                    break;
                default:
                    return;
            }
        }
    }
    
    Token Lexer::identifier() {
        while (isalnum(peek()) || peek() == '_') {
            advance();
        }
        
        // Check if it's a keyword
        std::string text = source.substr(start, current - start);
        auto it = keywords.find(text);
        TokenType type = (it != keywords.end()) ? it->second : TokenType::Name;
        
        return makeToken(type);
    }
    
    Token Lexer::number() {
        // Integer part
        while (isdigit(peek())) {
            advance();
        }
        
        // Decimal part
        if (peek() == '.' && isdigit(peekNext())) {
            // Consume '.'
            advance();
            
            // Consume decimal part
            while (isdigit(peek())) {
                advance();
            }
        }
        
        // Scientific notation
        if (peek() == 'e' || peek() == 'E') {
            advance();
            
            // Optional sign
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            
            // Exponent
            if (!isdigit(peek())) {
                return errorToken("Invalid number: expected digits in exponent");
            }
            
            while (isdigit(peek())) {
                advance();
            }
        }
        
        // Parse number
        Token token = makeToken(TokenType::Number);
        token.value.number = std::stod(token.lexeme);
        return token;
    }
    
    Token Lexer::string() {
        char delimiter = source[start]; // Save string delimiter (' or ")
        
        // Consume until matching delimiter
        while (peek() != delimiter && !isAtEnd()) {
            if (peek() == '\n') {
                line++;
                column = 0;
            }
            
            // Handle escape sequences
            if (peek() == '\\') {
                advance(); // Consume backslash
                
                // Handle other escape characters...
                if (peek() == 'n' || peek() == 't' || peek() == 'r' || peek() == '\\' || 
                    peek() == '\'' || peek() == '\"') {
                    advance();
                } else {
                    // Unhandled escape sequence
                }
            } else {
                advance();
            }
        }
        
        if (isAtEnd()) {
            return errorToken("Unterminated string.");
        }
        
        // Closing delimiter
        advance();
        
        // Extract string content (excluding quotes)
        Str value = source.substr(start + 1, current - start - 2);
        Token token = makeToken(TokenType::String);
        token.value.string = GCString::create(value);
        return token;
    }
}
