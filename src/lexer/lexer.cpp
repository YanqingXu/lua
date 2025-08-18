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
        
        // Number (including hex)
        if (isdigit(c)) {
            if (c == '0' && (peek() == 'x' || peek() == 'X')) {
                return hexNumber();
            }
            return number();
        }

        switch (c) {
            // Single character tokens
            case '(': return makeToken(TokenType::LeftParen);
            case ')': return makeToken(TokenType::RightParen);
            case '{': return makeToken(TokenType::LeftBrace);
            case '}': return makeToken(TokenType::RightBrace);
            case '[': {
                // Check for long string
                usize savePos = current;
                int level = skipSeparator();
                if (level >= 0) {
                    return longString(level);
                } else {
                    current = savePos;
                    return makeToken(TokenType::LeftBracket);
                }
            }
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
                // Advance to include the unexpected character in the token
                advance();
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
        Str lexeme;

        // Safe substr with boundary checking
        if (start < source.length() && current > start && current <= source.length()) {
            lexeme = source.substr(start, current - start);
        }

        Token token(type, lexeme, line, column - static_cast<int>(current - start));
        return token;
    }
    
    Token Lexer::errorToken(const Str& message) {
        // For Lua 5.1 compatibility, use the actual character(s) as lexeme
        Str actualLexeme;

        // Safe substr with boundary checking
        if (start < source.length() && current > start && current <= source.length()) {
            actualLexeme = source.substr(start, current - start);
        }

        if (actualLexeme.empty() && !isAtEnd() && current < source.length()) {
            actualLexeme = source.substr(current, 1);
        }

        // Fallback to a safe default if we still have no lexeme
        if (actualLexeme.empty()) {
            actualLexeme = "@"; // Use @ as a safe fallback for error tokens
        }

        Token token(TokenType::Error, actualLexeme, line, column);
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
                        advance(); // Skip first '-'
                        advance(); // Skip second '-'

                        // Check for long comment --[[ or --[=[
                        if (peek() == '[') {
                            usize savePos = current;
                            advance(); // Skip '['
                            int level = skipSeparator();
                            if (level >= 0) {
                                // Long comment
                                skipLongComment(level);
                            } else {
                                // Not a long comment, reset and treat as short comment
                                current = savePos;
                                // Short comment until end of line
                                while (peek() != '\n' && !isAtEnd()) {
                                    advance();
                                }
                            }
                        } else {
                            // Short comment until end of line
                            while (peek() != '\n' && !isAtEnd()) {
                                advance();
                            }
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

            // Check for malformed number (multiple dots)
            if (peek() == '.' && isdigit(peekNext())) {
                // This is a malformed number like 123.45.67
                // Consume the rest to include in error token
                advance(); // consume second dot
                while (isdigit(peek())) {
                    advance();
                }
                Token token(TokenType::InvalidNumber, source.substr(start, current - start), line, column - static_cast<int>(current - start));
                return token;
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

        // Save the starting position for error reporting
        int startLine = line;
        int startColumn = column;

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
            // Create a token with the unfinished string content for better error reporting
            // Use the position where the string started
            // For Lua 5.1 compatibility, limit the token content to avoid truncation
            Str tokenContent = source.substr(start, current - start);
            // Remove any trailing newlines to match Lua 5.1 behavior
            while (!tokenContent.empty() && (tokenContent.back() == '\n' || tokenContent.back() == '\r')) {
                tokenContent.pop_back();
            }
            Token token(TokenType::UnterminatedString, tokenContent, startLine, startColumn);
            return token;
        }
        
        // Closing delimiter
        advance();
        
        // Extract string content (excluding quotes)
        Str value = source.substr(start + 1, current - start - 2);
        Token token = makeToken(TokenType::String);
        token.value.string = GCString::create(value);
        return token;
    }

    int Lexer::skipSeparator() {
        int count = 0;
        usize s = current;
        while (current < source.length() && source[current] == '=') {
            current++;
            count++;
        }
        if (current < source.length() && source[current] == '[') {
            current++;
            return count;
        } else {
            current = s; // Reset if not a valid separator
            return -1;
        }
    }

    Token Lexer::longString(int level) {
        // Skip the opening bracket sequence
        while (!isAtEnd()) {
            if (peek() == ']') {
                usize savePos = current;
                advance(); // Skip ']'
                int closeLevel = skipSeparator();
                if (closeLevel == level) {
                    // Found matching close bracket
                    Str value = source.substr(start + level + 2, savePos - start - level - 2);
                    Token token = makeToken(TokenType::String);
                    token.value.string = GCString::create(value);
                    return token;
                } else {
                    // Not a matching close, continue
                    current = savePos + 1;
                }
            } else {
                if (peek() == '\n') {
                    line++;
                    column = 0;
                } else {
                    column++;
                }
                advance();
            }
        }
        return errorToken("Unterminated long string.");
    }

    void Lexer::skipLongComment(int level) {
        while (!isAtEnd()) {
            if (peek() == ']') {
                usize savePos = current;
                advance(); // Skip ']'
                int closeLevel = skipSeparator();
                if (closeLevel == level) {
                    // Found matching close bracket, comment ends
                    return;
                } else {
                    // Not a matching close, continue
                    current = savePos + 1;
                }
            } else {
                if (peek() == '\n') {
                    line++;
                    column = 0;
                } else {
                    column++;
                }
                advance();
            }
        }
        // If we reach here, the long comment is unterminated
        // In Lua 5.1, this is typically handled as an error
    }

    Token Lexer::hexNumber() {
        // Skip '0x' or '0X'
        advance(); // Skip 'x' or 'X'

        if (!std::isxdigit(peek())) {
            return errorToken("Invalid hexadecimal number.");
        }

        while (std::isxdigit(peek())) {
            advance();
        }

        // Convert hex string to number
        Str hexStr = source.substr(start + 2, current - start - 2); // Skip "0x"
        LuaNumber value = static_cast<LuaNumber>(std::stoll(hexStr, nullptr, 16));

        Token token = makeToken(TokenType::Number);
        token.value.number = value;
        return token;
    }

    Str tokenTypeToString(TokenType type) {
        switch (type) {
            case TokenType::And: return "and";
            case TokenType::Break: return "break";
            case TokenType::Do: return "do";
            case TokenType::Else: return "else";
            case TokenType::Elseif: return "elseif";
            case TokenType::End: return "end";
            case TokenType::False: return "false";
            case TokenType::For: return "for";
            case TokenType::Function: return "function";
            case TokenType::If: return "if";
            case TokenType::In: return "in";
            case TokenType::Local: return "local";
            case TokenType::Nil: return "nil";
            case TokenType::Not: return "not";
            case TokenType::Or: return "or";
            case TokenType::Repeat: return "repeat";
            case TokenType::Return: return "return";
            case TokenType::Then: return "then";
            case TokenType::True: return "true";
            case TokenType::Until: return "until";
            case TokenType::While: return "while";
            case TokenType::Plus: return "+";
            case TokenType::Minus: return "-";
            case TokenType::Star: return "*";
            case TokenType::Slash: return "/";
            case TokenType::Percent: return "%";
            case TokenType::Caret: return "^";
            case TokenType::Hash: return "#";
            case TokenType::Equal: return "==";
            case TokenType::LessEqual: return "<=";
            case TokenType::GreaterEqual: return ">=";
            case TokenType::Less: return "<";
            case TokenType::Greater: return ">";
            case TokenType::NotEqual: return "~=";
            case TokenType::Assign: return "=";
            case TokenType::LeftParen: return "(";
            case TokenType::RightParen: return ")";
            case TokenType::LeftBrace: return "{";
            case TokenType::RightBrace: return "}";
            case TokenType::LeftBracket: return "[";
            case TokenType::RightBracket: return "]";
            case TokenType::Semicolon: return ";";
            case TokenType::Colon: return ":";
            case TokenType::Comma: return ",";
            case TokenType::Dot: return ".";
            case TokenType::DotDot: return "..";
            case TokenType::DotDotDot: return "...";
            case TokenType::Number: return "number";
            case TokenType::String: return "string";
            case TokenType::Name: return "identifier";
            case TokenType::Eof: return "end of file";
            case TokenType::Error: return "error";
            case TokenType::UnterminatedString: return "unterminated string";
            case TokenType::InvalidNumber: return "invalid number";
            default: return "unknown";
        }
    }
}
