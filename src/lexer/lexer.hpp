#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_string.hpp"
#include <string>
#include <vector>

namespace Lua {
    // Token types
    enum class TokenType {
        // Keywords
        And, Break, Do, Else, Elseif, End, False, For, Function,
        If, In, Local, Nil, Not, Or, Repeat, Return, Then, True, Until, While,
        // Symbols
        Plus, Minus, Star, Slash, Percent, Caret, Hash, Equal, LessEqual,
        GreaterEqual, Less, Greater, NotEqual, Assign, LeftParen, RightParen,
        LeftBrace, RightBrace, LeftBracket, RightBracket, Semicolon, Colon,
        Comma, Dot, DotDot, DotDotDot,
        // Others
        Number, String, Name, Eof, Error
    };
    
    // Token structure
    struct Token {
        TokenType type;
        Str lexeme;
        int line;
        int column;
        
        // Value (for numbers and strings)
        union {
            LuaNumber number;
            GCString* string;
        } value;
        
        Token() : type(TokenType::Error), line(1), column(1) {
            value.number = 0.0;
        }
        
        Token(TokenType type, const Str& lexeme, int line, int column)
            : type(type), lexeme(lexeme), line(line), column(column) {
            value.number = 0.0;
        }
    };
    
    // Utility function to convert TokenType to string
    Str tokenTypeToString(TokenType type);
    
    // Lexer
    class Lexer {
    private:
        Str source;
        usize current;
        usize start;
        int line;
        int column;
        
    public:
        Lexer(const Str& source);
        
        // Get next token
        Token nextToken();
        
    private:
        // Helper methods
        bool isAtEnd() const;
        char advance();
        char peek() const;
        char peekNext() const;
        bool match(char expected);
        
        // Create tokens
        Token makeToken(TokenType type);
        Token errorToken(const Str& message);
        
        // Skip whitespace and comments
        void skipWhitespace();
        
        // Identifiers and keywords
        Token identifier();
        
        // Numbers
        Token number();
        
        // Strings
        Token string();
    };
}
