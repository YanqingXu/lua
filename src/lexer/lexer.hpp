#pragma once

#include "../types.hpp"
#include <string>
#include <vector>

namespace Lua {
    // 标记类型
    enum class TokenType {
        // 关键字
        And, Break, Do, Else, Elseif, End, False, For, Function,
        If, In, Local, Nil, Not, Or, Repeat, Return, Then, True, Until, While,
        // 符号
        Plus, Minus, Star, Slash, Percent, Caret, Hash, Equal, LessEqual,
        GreaterEqual, Less, Greater, NotEqual, Assign, LeftParen, RightParen,
        LeftBrace, RightBrace, LeftBracket, RightBracket, Semicolon, Colon,
        Comma, Dot, DotDot, DotDotDot,
        // 其他
        Number, String, Name, Eof, Error
    };
    
    // 标记结构
    struct Token {
        TokenType type;
        Str lexeme;
        int line;
        int column;
        
        // 值 (用于数字和字符串)
        union {
            LuaNumber number;
            Str* string;
        } value;
        
        Token() : type(TokenType::Error), line(1), column(1) {
            value.number = 0.0;
        }
        
        Token(TokenType type, const Str& lexeme, int line, int column)
            : type(type), lexeme(lexeme), line(line), column(column) {
            value.number = 0.0;
        }
    };
    
    // 词法分析器
    class Lexer {
    private:
        Str source;
        usize current;
        usize start;
        int line;
        int column;
        
    public:
        Lexer(const Str& source);
        
        // 获取下一个标记
        Token nextToken();
        
    private:
        // 辅助方法
        bool isAtEnd() const;
        char advance();
        char peek() const;
        char peekNext() const;
        bool match(char expected);
        
        // 创建标记
        Token makeToken(TokenType type);
        Token errorToken(const Str& message);
        
        // 跳过空白字符和注释
        void skipWhitespace();
        
        // 标识符和关键字
        Token identifier();
        
        // 数字
        Token number();
        
        // 字符串
        Token string();
    };
}
