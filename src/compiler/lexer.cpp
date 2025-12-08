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
    : sourceStorage_(source)
    , input_(nullptr)
    , ownedInput_(makeUnique<IO::InputStream>(StrView(sourceStorage_)))
    , currentChar_(-1)
    , nextChar_(-1)
    , hasNextChar_(false)
    , lexemeBuffer_()
    , start_(0)
    , line_(1)
    , column_(1)
    , lookahead_(std::nullopt)
{
    // 使用拥有的 InputStream（基于内部保存的 sourceStorage_）
    input_ = ownedInput_.get();

    // 预填充字符缓存
    currentChar_ = input_->getChar();
    if (currentChar_ != -1) {
        nextChar_ = input_->getChar();
        hasNextChar_ = true;
    }
}

Lexer::Lexer(IO::InputStream& input)
    : input_(&input)
    , ownedInput_(nullptr)
    , currentChar_(-1)
    , nextChar_(-1)
    , hasNextChar_(false)
    , lexemeBuffer_()
    , start_(0)
    , line_(1)
    , column_(1)
    , lookahead_(std::nullopt)
{
    // 预填充字符缓存
    currentChar_ = input_->getChar();
    if (currentChar_ != -1) {
        nextChar_ = input_->getChar();
        hasNextChar_ = true;
    }
}

// =====================================================================
// 字符操作
// =====================================================================

bool Lexer::isAtEnd() const noexcept {
    return currentChar_ == -1;
}

char Lexer::advance() {
    i32 ch = currentChar_;
    if (ch == -1) return '\0';

    // 累积到 lexeme 缓冲区
    lexemeBuffer_ += static_cast<char>(ch);

    // 更新行列号（在前移字符缓存之前）
    if (ch == '\n') {
        line_++;
        column_ = 0;
    } else {
        column_++;
    }

    // 前移字符缓存
    currentChar_ = nextChar_;

    // 读取新的 nextChar_
    if (currentChar_ != -1) {
        nextChar_ = input_->getChar();
        hasNextChar_ = true;
    } else {
        nextChar_ = -1;
        hasNextChar_ = false;
    }

    return static_cast<char>(ch);
}

char Lexer::peek() const noexcept {
    return (currentChar_ == -1) ? '\0' : static_cast<char>(currentChar_);
}

char Lexer::peekNext() const noexcept {
    // 如果 nextChar_ 已经加载，直接返回
    if (hasNextChar_) {
        return (nextChar_ == -1) ? '\0' : static_cast<char>(nextChar_);
    }

    // 否则返回 '\0'（表示没有下一个字符或未加载）
    // 注意：在构造函数中我们已经预加载了 nextChar_，所以这种情况很少发生
    return '\0';
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (peek() != expected) return false;

    advance();
    return true;
}

// =====================================================================
// Token创建
// =====================================================================

Token Lexer::makeToken(TokenType type) {
    // 使用累积的 lexeme 缓冲区
    i32 tokenColumn = column_ - static_cast<i32>(lexemeBuffer_.length());
    Token token(type, lexemeBuffer_, line_, tokenColumn);
    return token;
}

Token Lexer::errorToken(const Str& message) {
    // 使用累积的 lexeme 缓冲区，如果为空则使用错误消息
    Str lexeme = lexemeBuffer_.empty() ? message : lexemeBuffer_;
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
            case '\n':
                advance();
                break;

            case '-':
                // 检查是否为注释
                if (peekNext() == '-') {
                    advance(); // 跳过第一个'-'
                    advance(); // 跳过第二个'-'

                    // 检查长注释 --[[ 或 --[=[
                    if (peek() == '[') {
                        i32 level = skipSeparator();
                        if (level >= 0) {
                            // 长注释（skipSeparator 已经消费了分隔符）
                            skipLongComment(level);
                        } else {
                            // 不是长注释（skipSeparator 已经回退），作为短注释处理
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
        char firstNewline = peek();
        advance();
        // 处理 \r\n 或 \n\r
        if ((peek() == '\n' || peek() == '\r') && peek() != firstNewline) {
            advance();
        }
    }

    // 跳过长注释内容，直到找到匹配的结束符 ]=*]
    while (!isAtEnd()) {
        if (peek() == ']') {
            i32 endLevel = skipSeparator();
            if (endLevel == level) {
                // 找到匹配的结束符（skipSeparator 已经消费了分隔符）
                return;
            }
            // 不匹配（skipSeparator 已经回退），继续查找
        }

        advance();
    }
}

i32 Lexer::skipSeparator() {
    // 跳过 [=*[ 或 ]=*] 形式的分隔符
    // 返回等号的数量，如果不是有效分隔符返回-1
    // 注意：调用此函数时，peek()应该指向第一个'['或']'

    // 保存状态以便回退
    Str savedLexeme = lexemeBuffer_;
    i32 savedLine = line_;
    i32 savedColumn = column_;
    i32 savedCurrentChar = currentChar_;
    i32 savedNextChar = nextChar_;
    bool savedHasNextChar = hasNextChar_;

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

    // 不是有效分隔符，回退状态
    lexemeBuffer_ = savedLexeme;
    line_ = savedLine;
    column_ = savedColumn;
    currentChar_ = savedCurrentChar;
    nextChar_ = savedNextChar;
    hasNextChar_ = savedHasNextChar;

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

    // 使用累积的 lexeme 缓冲区查找关键字（O(1)时间复杂度）
    auto it = keywords.find(lexemeBuffer_);
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
    Token token = makeToken(TokenType::Number);

    char* end = nullptr;
    const char* cstr = lexemeBuffer_.c_str();
    f64 value = std::strtod(cstr, &end);

    bool ok = end != nullptr && static_cast<usize>(end - cstr) == lexemeBuffer_.size();
    if (!ok) {
        // 尝试使用 locale 小数点再解析一次（Lua 5.1 行为）
        std::lconv* lc = std::localeconv();
        char locPoint = (lc && lc->decimal_point && lc->decimal_point[0] != '\0') ? lc->decimal_point[0] : '.';
        if (locPoint != '.') {
            Str localized = lexemeBuffer_;
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
    Token token = makeToken(TokenType::Number);

    char* end;
    f64 value = static_cast<f64>(std::strtoll(lexemeBuffer_.c_str(), &end, 16));
    if (end == nullptr || static_cast<usize>(end - lexemeBuffer_.c_str()) != lexemeBuffer_.size()) {
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
        char firstNewline = peek();
        advance();
        // 处理潜在的 CRLF / LFCR 组合
        if ((peek() == '\n' || peek() == '\r') && peek() != firstNewline) {
            advance();
        }
    }

    // 跳过长字符串内容，直到找到匹配的结束符
    while (!isAtEnd()) {
        if (peek() == ']') {
            // 检查是否为结束符 ]=*]
            // 保存状态以便回退
            Str savedLexeme = lexemeBuffer_;
            i32 savedLine = line_;
            i32 savedColumn = column_;
            i32 savedCurrentChar = currentChar_;
            i32 savedNextChar = nextChar_;
            bool savedHasNextChar = hasNextChar_;

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
            lexemeBuffer_ = savedLexeme;
            line_ = savedLine;
            column_ = savedColumn;
            currentChar_ = savedCurrentChar;
            nextChar_ = savedNextChar;
            hasNextChar_ = savedHasNextChar;
            result += advance();
        } else {
            result += advance();
        }
    }

    return errorToken("Unterminated long string");
}

// =====================================================================
// Token预读机制（支持LL(1)语法分析）
// =====================================================================

Token Lexer::nextToken() {
    // 如果有预读Token，先返回预读Token
    if (lookahead_.has_value()) {
        Token token = lookahead_.value();
        lookahead_ = std::nullopt;  // 清除预读状态
        return token;
    }
    
    // 否则解析新Token
    return scanToken();
}

Token Lexer::peekToken() {
    // 如果已经有预读Token，直接返回
    if (lookahead_.has_value()) {
        return lookahead_.value();
    }
    
    // 否则解析并缓存预读Token
    lookahead_ = scanToken();
    return lookahead_.value();
}

// =====================================================================
// 主要的Token解析函数（内部使用）
// =====================================================================

Token Lexer::scanToken() {
    skipWhitespace();

    // 清空 lexeme 缓冲区，准备扫描新 token
    lexemeBuffer_.clear();

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
        // 保存状态以便回退
        Str savedLexeme = lexemeBuffer_;
        i32 savedLine = line_;
        i32 savedColumn = column_;
        i32 savedCurrentChar = currentChar_;
        i32 savedNextChar = nextChar_;
        bool savedHasNextChar = hasNextChar_;

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
        lexemeBuffer_ = savedLexeme;
        line_ = savedLine;
        column_ = savedColumn;
        currentChar_ = savedCurrentChar;
        nextChar_ = savedNextChar;
        hasNextChar_ = savedHasNextChar;

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


