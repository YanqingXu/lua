/**
 * @file lexer.cpp
 * @brief Lua词法分析器实现
 */

#include "lexer.hpp"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <locale>
#include <unordered_map>

namespace Lua {

// =====================================================================
// 关键字哈希表（静态初始化，O(1)查找）
// =====================================================================

static const HashMap<Str, TokenType> keywords = {
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

// =====================================================================
// TokenType转字符串（用于调试）
// =====================================================================

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        // 关键字
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
        
        // 多字符运算符
        case TokenType::Concat: return "..";
        case TokenType::Dots: return "...";
        case TokenType::Eq: return "==";
        case TokenType::Ge: return ">=";
        case TokenType::Le: return "<=";
        case TokenType::Ne: return "~=";
        
        // 字面量和标识符
        case TokenType::Number: return "<number>";
        case TokenType::String: return "<string>";
        case TokenType::Name: return "<name>";
        
        // 特殊
        case TokenType::Eos: return "<eof>";
        case TokenType::Error: return "<error>";
        
        default: return "<unknown>";
    }
}

// =====================================================================
// Lexer构造函数
// =====================================================================

Lexer::Lexer(const Str& source)
    : source_(source)
    , current_(0)
    , start_(0)
    , line_(1)
    , column_(1)
{
}

// =====================================================================
// 字符操作
// =====================================================================

bool Lexer::isAtEnd() const noexcept {
    return current_ >= source_.length();
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    column_++;
    return source_[current_++];
}

char Lexer::peek() const noexcept {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const noexcept {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    
    current_++;
    column_++;
    return true;
}

// =====================================================================
// Token创建
// =====================================================================

Token Lexer::makeToken(TokenType type) {
    Str lexeme;
    
    // 安全的子串提取
    if (start_ < source_.length() && current_ > start_ && current_ <= source_.length()) {
        lexeme = source_.substr(start_, current_ - start_);
    }
    
    Token token(type, lexeme, line_, column_ - static_cast<i32>(current_ - start_));
    return token;
}

Token Lexer::errorToken(const Str& message) {
    Str lexeme;
    if (start_ < source_.length() && current_ > start_ && current_ <= source_.length()) {
        lexeme = source_.substr(start_, current_ - start_);
    }
    if (lexeme.empty()) {
        lexeme = message; // 回退到消息，至少提供可读信息
    }
    return Token(TokenType::Error, lexeme, line_, column_);
}

// =====================================================================
// 跳过空白和注释
// =====================================================================

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
                
            case '\n':
                line_++;
                column_ = 0;
                advance();
                break;

            case '-':
                // 检查是否为注释
                if (peekNext() == '-') {
                    advance(); // 跳过第一个'-'
                    advance(); // 跳过第二个'-'

                    // 检查长注释 --[[ 或 --[=[
                    if (peek() == '[') {
                        usize savePos = current_;
                        i32 saveCol = column_;

                        i32 level = skipSeparator();
                        if (level >= 0) {
                            // 长注释
                            skipLongComment(level);
                        } else {
                            // 不是长注释，回退并作为短注释处理
                            current_ = savePos;
                            column_ = saveCol;
                            skipLineComment();
                        }
                    } else {
                        // 短注释
                        skipLineComment();
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

void Lexer::skipLineComment() {
    // 跳过直到行尾
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
}

void Lexer::skipLongComment(i32 level) {
    // 如果起始分隔符后立刻是换行，则跳过（与长字符串行为一致）
    if (peek() == '\n' || peek() == '\r') {
        advance();
        line_++;
        column_ = 0;
        if ((peek() == '\n' || peek() == '\r') && peek() != source_[current_ - 1]) {
            advance();
        }
    }

    // 跳过长注释内容，直到找到匹配的结束符 ]=*]
    while (!isAtEnd()) {
        if (peek() == ']') {
            usize savePos = current_;
            i32 saveCol = column_;
            i32 endLevel = skipSeparator();
            if (endLevel == level) {
                // 找到匹配的结束符
                return;
            }

            // 不匹配，回退
            current_ = savePos;
            column_ = saveCol;
        }

        if (peek() == '\n') {
            line_++;
            column_ = 0;
        }
        advance();
    }
}

i32 Lexer::skipSeparator() {
    // 跳过 [=*[ 或 ]=*] 形式的分隔符
    // 返回等号的数量，如果不是有效分隔符返回-1
    // 注意：调用此函数时，current_应该指向第一个'['或']'

    i32 count = 0;
    char s = peek();

    // 必须是'['或']'
    if (s != '[' && s != ']') {
        return -1;
    }

    advance(); // 跳过第一个'['或']'

    // 计算等号数量
    while (peek() == '=') {
        count++;
        advance();
    }

    // 检查结束符
    if (peek() == s) {
        advance(); // 跳过结束的'['或']'
        return count;
    }

    // 不是有效分隔符
    return -1;
}

// =====================================================================
// 标识符和关键字识别
// =====================================================================

Token Lexer::identifier() {
    // 标识符：[a-zA-Z_][a-zA-Z0-9_]*
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    // 获取词素
    Str lexeme = source_.substr(start_, current_ - start_);

    // 使用哈希表查找关键字（O(1)时间复杂度）
    auto it = keywords.find(lexeme);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::Name;

    return makeToken(type);
}

// =====================================================================
// 数字识别
// =====================================================================

Token Lexer::number() {
    // 整数部分
    while (std::isdigit(peek())) {
        advance();
    }

    // 小数部分
    if (peek() == '.' && std::isdigit(peekNext())) {
        advance(); // 跳过'.'
        while (std::isdigit(peek())) {
            advance();
        }
    }

    // 指数部分 (e或E)
    if (peek() == 'e' || peek() == 'E') {
        advance(); // 跳过'e'或'E'

        // 可选的符号
        if (peek() == '+' || peek() == '-') {
            advance();
        }

        // 指数数字
        if (!std::isdigit(peek())) {
            return errorToken("Invalid number: expected digits after exponent");
        }

        while (std::isdigit(peek())) {
            advance();
        }
    }

    // 读取尾随的字母/下划线以捕获 Lua 5.1 定义的非法数字形式（如 123abc）
    if (std::isalpha(peek()) || peek() == '_') {
        while (std::isalnum(peek()) || peek() == '_') {
            advance();
        }
    }

    // 转换为数字并校验是否完全消费，若未完全消费则视为 malformed number
    Str lexeme = source_.substr(start_, current_ - start_);
    Token token = makeToken(TokenType::Number);

    char* end = nullptr;
    const char* cstr = lexeme.c_str();
    f64 value = std::strtod(cstr, &end);

    bool ok = end != nullptr && static_cast<usize>(end - cstr) == lexeme.size();
    if (!ok) {
        // 尝试使用 locale 小数点再解析一次（Lua 5.1 行为）
        std::lconv* lc = std::localeconv();
        char locPoint = (lc && lc->decimal_point && lc->decimal_point[0] != '\0') ? lc->decimal_point[0] : '.';
        if (locPoint != '.') {
            Str localized = lexeme;
            for (char& ch : localized) {
                if (ch == '.') ch = locPoint;
            }
            end = nullptr;
            value = std::strtod(localized.c_str(), &end);
            ok = end != nullptr && static_cast<usize>(end - localized.c_str()) == localized.size();
        }
    }

    if (!ok) {
        return errorToken("Malformed number");
    }

    token.value = value;
    return token;
}

Token Lexer::hexNumber() {
    // 跳过'0x'或'0X'
    advance(); // '0'
    advance(); // 'x'或'X'

    if (!std::isxdigit(peek())) {
        return errorToken("Invalid hexadecimal number: expected hex digits after 0x");
    }

    // 如果后续紧跟字母/下划线，按照 Lua 5.1 语义视为格式错误
    if (std::isalpha(peek()) || peek() == '_') {
        while (std::isalnum(peek()) || peek() == '_') {
            advance();
        }
        return errorToken("Malformed hexadecimal number");
    }

    // 十六进制数字
    while (std::isxdigit(peek())) {
        advance();
    }

    // 转换为数字
    Str lexeme = source_.substr(start_, current_ - start_);
    Token token = makeToken(TokenType::Number);

    char* end;
    f64 value = static_cast<f64>(std::strtoll(lexeme.c_str(), &end, 16));
    if (end == nullptr || static_cast<usize>(end - lexeme.c_str()) != lexeme.size()) {
        return errorToken("Malformed hexadecimal number");
    }
    token.value = value;

    return token;
}

// =====================================================================
// 字符串识别
// =====================================================================

Token Lexer::string(char quote) {
    Str result;

    while (peek() != quote && !isAtEnd()) {
        if (peek() == '\n') {
            return errorToken("Unterminated string");
        }

        if (peek() == '\\') {
            advance(); // 跳过'\'

            if (isAtEnd()) {
                return errorToken("Unterminated string");
            }

            // 转义字符
            char c = advance();
            switch (c) {
                case 'a': result += '\a'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'v': result += '\v'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case '\'': result += '\''; break;
                case '\n':
                    line_++;
                    column_ = 0;
                    result += '\n';
                    break;
                default:
                    // 数字转义 \ddd
                    if (std::isdigit(c)) {
                        i32 value = c - '0';
                        i32 count = 1;

                        while (count < 3 && std::isdigit(peek())) {
                            value = value * 10 + (peek() - '0');
                            advance();
                            count++;
                        }

                        if (value > 255) {
                            return errorToken("Decimal escape too large");
                        }

                        result += static_cast<char>(value);
                    } else {
                        result += c;
                    }
                    break;
            }
        } else {
            if (peek() == '\n') {
                line_++;
                column_ = 0;
            }
            result += advance();
        }
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string");
    }

    // 跳过结束引号
    advance();

    Token token = makeToken(TokenType::String);
    token.value = result;

    return token;
}

Token Lexer::longString(i32 level) {
    Str result;

    // 如果长字符串起始分隔符后立刻是换行，则丢弃该换行（Lua 5.1 行为）
    if (peek() == '\n' || peek() == '\r') {
        advance();
        line_++;
        column_ = 0;
        // 处理潜在的 CRLF / LFCR 组合
        if ((peek() == '\n' || peek() == '\r') && peek() != source_[current_ - 1]) {
            advance();
        }
    }

    // 跳过长字符串内容，直到找到匹配的结束符
    while (!isAtEnd()) {
        if (peek() == ']') {
            // 检查是否为结束符 ]=*]
            usize savePos = current_;
            i32 saveCol = column_;
            advance(); // 跳过']'

            // 计算等号数量
            i32 endLevel = 0;
            while (peek() == '=') {
                endLevel++;
                advance();
            }

            // 检查是否匹配
            if (peek() == ']' && endLevel == level) {
                advance(); // 跳过最后的']'
                // 找到匹配的结束符
                Token token = makeToken(TokenType::String);
                token.value = result;
                return token;
            }

            // 不匹配，回退并添加到结果
            current_ = savePos;
            column_ = saveCol;
            result += advance();
        } else {
            if (peek() == '\n') {
                line_++;
                column_ = 0;
            }
            result += advance();
        }
    }

    return errorToken("Unterminated long string");
}

// =====================================================================
// 主要的Token获取函数
// =====================================================================

Token Lexer::nextToken() {
    skipWhitespace();

    start_ = current_;

    if (isAtEnd()) {
        return makeToken(TokenType::Eos);
    }

    char c = advance();

    // 标识符或关键字
    if (std::isalpha(c) || c == '_') {
        return identifier();
    }

    // 数字
    if (std::isdigit(c)) {
        // 检查十六进制
        if (c == '0' && (peek() == 'x' || peek() == 'X')) {
            return hexNumber();
        }
        return number();
    }

    // 字符串
    if (c == '"' || c == '\'') {
        return string(c);
    }

    // 长字符串 [[ 或 [=[
    if (c == '[') {
        // 检查是否为长字符串
        // 计算等号数量
        i32 level = 0;
        while (peek() == '=') {
            level++;
            advance();
        }

        // 检查是否为长字符串开始符 [=*[
        if (peek() == '[') {
            advance(); // 跳过第二个'['
            return longString(level);
        }

        // 不是长字符串，回退等号
        for (i32 i = 0; i < level; ++i) {
            current_--;
            column_--;
        }

        // 返回单字符'['
        return makeToken(static_cast<TokenType>('['));
    }

    // 运算符和分隔符
    switch (c) {
        case '+': return makeToken(static_cast<TokenType>('+'));
        case '-': return makeToken(static_cast<TokenType>('-'));
        case '*': return makeToken(static_cast<TokenType>('*'));
        case '/': return makeToken(static_cast<TokenType>('/'));
        case '%': return makeToken(static_cast<TokenType>('%'));
        case '^': return makeToken(static_cast<TokenType>('^'));
        case '#': return makeToken(static_cast<TokenType>('#'));
        case '(': return makeToken(static_cast<TokenType>('('));
        case ')': return makeToken(static_cast<TokenType>(')'));
        case '{': return makeToken(static_cast<TokenType>('{'));
        case '}': return makeToken(static_cast<TokenType>('}'));
        case ']': return makeToken(static_cast<TokenType>(']'));
        case ';': return makeToken(static_cast<TokenType>(';'));
        case ',': return makeToken(static_cast<TokenType>(','));

        case '=':
            return match('=') ? makeToken(TokenType::Eq) : makeToken(static_cast<TokenType>('='));

        case '<':
            return match('=') ? makeToken(TokenType::Le) : makeToken(static_cast<TokenType>('<'));

        case '>':
            return match('=') ? makeToken(TokenType::Ge) : makeToken(static_cast<TokenType>('>'));

        case '~':
            if (match('=')) {
                return makeToken(TokenType::Ne);
            }
            return errorToken("Unexpected character '~'");

        case ':':
            return makeToken(static_cast<TokenType>(':'));

        case '.':
            if (match('.')) {
                if (match('.')) {
                    return makeToken(TokenType::Dots);  // ...
                }
                return makeToken(TokenType::Concat);    // ..
            }
            // 检查是否为数字 .123
            if (std::isdigit(peek())) {
                return number();
            }
            return makeToken(static_cast<TokenType>('.'));

        default:
            break;
    }

    return errorToken("Unexpected character");
}

} // namespace Lua


