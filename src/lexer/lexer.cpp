#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace Lua {
    // 关键字映射表
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
        
        // 标记开始位置
        start = current;
        
        if (isAtEnd()) return makeToken(TokenType::Eof);
        
        char c = advance();
        
        // 标识符
        if (isalpha(c) || c == '_') return identifier();
        
        // 数字
        if (isdigit(c)) return number();
        
        switch (c) {
            // 单字符标记
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
            
            // 可能是双字符标记
            case '=':
                return makeToken(match('=') ? TokenType::Equal : TokenType::Assign);
            case '<':
                return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
            case '>':
                return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
            case '~':
                return makeToken(match('=') ? TokenType::NotEqual : TokenType::Error);
                
            // 点操作符 (., .., ...)
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
                
            // 字符串
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
        Token token(type, source.substr(start, current - start), line, column - (current - start));
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
                    // 检查是否是注释 (--...)
                    if (peekNext() == '-') {
                        // 注释一直到行尾
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
        
        // 查找是否是关键字
        std::string text = source.substr(start, current - start);
        auto it = keywords.find(text);
        TokenType type = (it != keywords.end()) ? it->second : TokenType::Name;
        
        return makeToken(type);
    }
    
    Token Lexer::number() {
        // 整数部分
        while (isdigit(peek())) {
            advance();
        }
        
        // 小数部分
        if (peek() == '.' && isdigit(peekNext())) {
            // 消费 '.'
            advance();
            
            // 消费小数部分
            while (isdigit(peek())) {
                advance();
            }
        }
        
        // 科学计数法
        if (peek() == 'e' || peek() == 'E') {
            advance();
            
            // 可选符号
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            
            // 指数
            if (!isdigit(peek())) {
                return errorToken("Invalid number: expected digits in exponent");
            }
            
            while (isdigit(peek())) {
                advance();
            }
        }
        
        // 解析数字
        Token token = makeToken(TokenType::Number);
        token.value.number = std::stod(token.lexeme);
        return token;
    }
    
    Token Lexer::string() {
        char delimiter = source[start]; // 保存字符串定界符 (' 或 ")
        
        // 消费直到匹配的定界符
        while (peek() != delimiter && !isAtEnd()) {
            if (peek() == '\n') {
                line++;
                column = 0;
            }
            
            // 处理转义序列
            if (peek() == '\\') {
                advance(); // 消费反斜杠
                
                // 处理其他转义字符...
                if (peek() == 'n' || peek() == 't' || peek() == 'r' || peek() == '\\' || 
                    peek() == '\'' || peek() == '\"') {
                    advance();
                } else {
                    // 未处理的转义序列
                }
            } else {
                advance();
            }
        }
        
        if (isAtEnd()) {
            return errorToken("Unterminated string.");
        }
        
        // 结束定界符
        advance();
        
        // 提取字符串内容 (不包括引号)
        Str value = source.substr(start + 1, current - start - 2);
        Token token = makeToken(TokenType::String);
        token.value.string = new Str(value);
        return token;
    }
}
