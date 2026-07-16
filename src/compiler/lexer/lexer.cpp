/**
 * @file lexer.cpp
 * @brief Lua词法分析器实现
 */

#include "lexer.hpp"
#include <array>
#include <cctype>
#include <cstdlib>
#include <locale>
#include <unordered_map>

namespace Lua {

// =====================================================================
// 关键字哈希表（静态初始化，O(1)查找）
// =====================================================================

static const HashMap<Str, TokenType> keywords = {
    {"and", TokenType::And},       {"break", TokenType::Break},   {"do", TokenType::Do},
    {"else", TokenType::Else},     {"elseif", TokenType::Elseif}, {"end", TokenType::End},
    {"false", TokenType::False},   {"for", TokenType::For},       {"function", TokenType::Function},
    {"if", TokenType::If},         {"in", TokenType::In},         {"local", TokenType::Local},
    {"nil", TokenType::Nil},       {"not", TokenType::Not},       {"or", TokenType::Or},
    {"repeat", TokenType::Repeat}, {"return", TokenType::Return}, {"then", TokenType::Then},
    {"true", TokenType::True},     {"until", TokenType::Until},   {"while", TokenType::While}};

namespace {

struct SimpleEscape {
    char escaped;
    char value;
};

constexpr std::array<SimpleEscape, 10> kSimpleEscapes{{{'a', '\a'},
                                                       {'b', '\b'},
                                                       {'f', '\f'},
                                                       {'n', '\n'},
                                                       {'r', '\r'},
                                                       {'t', '\t'},
                                                       {'v', '\v'},
                                                       {'\\', '\\'},
                                                       {'"', '"'},
                                                       {'\'', '\''}}};

Opt<char> decodeSimpleEscape(char c) noexcept {
    for (const SimpleEscape& escape : kSimpleEscapes) {
        if (escape.escaped == c) {
            return escape.value;
        }
    }

    return std::nullopt;
}

bool isSingleCharToken(char c) noexcept {
    if (c == '\0') {
        return false;
    }

    constexpr StrView kSingleCharTokens = "+-*/%^#(){}];,:";
    return kSingleCharTokens.find(c) != StrView::npos;
}

} // namespace

// =====================================================================
// TokenType转字符串（用于调试）
// =====================================================================

const char* tokenTypeToString(TokenType type) {
    switch (type) {
    // 关键字
    case TokenType::And:
        return "and";
    case TokenType::Break:
        return "break";
    case TokenType::Do:
        return "do";
    case TokenType::Else:
        return "else";
    case TokenType::Elseif:
        return "elseif";
    case TokenType::End:
        return "end";
    case TokenType::False:
        return "false";
    case TokenType::For:
        return "for";
    case TokenType::Function:
        return "function";
    case TokenType::If:
        return "if";
    case TokenType::In:
        return "in";
    case TokenType::Local:
        return "local";
    case TokenType::Nil:
        return "nil";
    case TokenType::Not:
        return "not";
    case TokenType::Or:
        return "or";
    case TokenType::Repeat:
        return "repeat";
    case TokenType::Return:
        return "return";
    case TokenType::Then:
        return "then";
    case TokenType::True:
        return "true";
    case TokenType::Until:
        return "until";
    case TokenType::While:
        return "while";

    // 多字符运算符
    case TokenType::Concat:
        return "..";
    case TokenType::Dots:
        return "...";
    case TokenType::Eq:
        return "==";
    case TokenType::Ge:
        return ">=";
    case TokenType::Le:
        return "<=";
    case TokenType::Ne:
        return "~=";

    // 字面量和标识符
    case TokenType::Number:
        return "<number>";
    case TokenType::String:
        return "<string>";
    case TokenType::Name:
        return "<name>";

    // 特殊
    case TokenType::Eos:
        return "<eof>";
    case TokenType::Error:
        return "<error>";

    default:
        return "<unknown>";
    }
}

// =====================================================================
// Lexer构造函数
// =====================================================================

Lexer::Lexer(const Str& source) : Lexer(source, nullptr) {}

Lexer::Lexer(const Str& source, LuaAllocator* allocator)
    : sourceStorage_(source), ownedInput_(makeUnique<IO::InputStream>(StrView(sourceStorage_))),
      inputCursor_(*ownedInput_, allocator), lexemeBuffer_(), tokenStartLine_(1), tokenStartColumn_(1),
      lookahead_(std::nullopt) {}

Lexer::Lexer(IO::InputStream& input) : Lexer(input, nullptr) {}

Lexer::Lexer(IO::InputStream& input, LuaAllocator* allocator)
    : ownedInput_(nullptr), inputCursor_(input, allocator), lexemeBuffer_(), tokenStartLine_(1), tokenStartColumn_(1),
      lookahead_(std::nullopt) {}

// =====================================================================
// 字符操作
// =====================================================================

bool Lexer::isAtEnd() const noexcept {
    return inputCursor_.isAtEnd();
}

char Lexer::advance() {
    if (inputCursor_.isAtEnd()) {
        return '\0';
    }
    char ch = inputCursor_.advance();
    lexemeBuffer_ += ch;
    return ch;
}

char Lexer::peek() const noexcept {
    return inputCursor_.peek();
}

char Lexer::peekNext() const noexcept {
    return inputCursor_.peek(1);
}

bool Lexer::match(char expected) {
    if (isAtEnd())
        return false;
    if (peek() != expected)
        return false;

    advance();
    return true;
}

bool Lexer::isNewline(char c) noexcept {
    return c == '\n' || c == '\r';
}

bool Lexer::isAlpha(char c) noexcept {
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool Lexer::isAlphaNum(char c) noexcept {
    return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

bool Lexer::isDigit(char c) noexcept {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool Lexer::isHexDigit(char c) noexcept {
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

void Lexer::beginToken() {
    tokenStartLine_ = inputCursor_.line();
    tokenStartColumn_ = inputCursor_.column();
}

void Lexer::consumeNewlineSequence() {
    if (!isNewline(peek())) {
        return;
    }

    char firstNewline = advance();
    consumeNewlinePairRemainder(firstNewline);
}

void Lexer::consumeNewlinePairRemainder(char firstNewline) {
    if (isNewline(peek()) && peek() != firstNewline) {
        advance();
    }
}

// =====================================================================
// Token创建
// =====================================================================

Token Lexer::makeToken(TokenType type) {
    Token token(type, lexemeBuffer_, tokenStartLine_, tokenStartColumn_);
    return token;
}

Token Lexer::errorToken(const Str& message) {
    // 使用累积的 lexeme 缓冲区，如果为空则使用错误消息
    Str lexeme = lexemeBuffer_.empty() ? message : lexemeBuffer_;
    Token token(TokenType::Error, lexeme, tokenStartLine_, tokenStartColumn_);
    token.errorMessage = message;
    return token;
}

// =====================================================================
// 跳过空白和注释
// =====================================================================

Opt<Token> Lexer::skipComment() {
    lexemeBuffer_.clear();
    beginToken();

    // 此函数假设已经检测到 '--'，需要跳过这两个字符
    advance(); // 跳过第一个'-'
    advance(); // 跳过第二个'-'

    if (peek() != '[') {
        // 短注释
        skipLineComment();
        return std::nullopt;
    }

    // 可能是长注释
    Opt<i32> level = readLongBracketDelimiter();
    if (level.has_value()) {
        return skipLongComment(level.value());
    } else {
        skipLineComment();
    }

    return std::nullopt;
}

Opt<Token> Lexer::skipWhitespace() {
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
                if (Opt<Token> error = skipComment()) {
                    return error;
                }
            } else {
                return std::nullopt;
            }
            break;

        case '#':
            if (inputCursor_.line() == 1 && inputCursor_.column() == 1) {
                skipLineComment();
            } else {
                return std::nullopt;
            }
            break;

        default:
            return std::nullopt;
        }
    }

    return std::nullopt;
}

void Lexer::skipLineComment() {
    while (!isNewline(peek()) && !isAtEnd()) {
        advance();
    }
}

Opt<Token> Lexer::skipLongComment(i32 level) {
    skipInitialLongLiteralNewline();

    // 跳过长注释内容，直到找到匹配的结束符 ]=*]
    while (!isAtEnd()) {
        if (peek() == ']') {
            LexerState savedState = saveState();
            Opt<i32> endLevel = readLongBracketDelimiter();
            if (endLevel.has_value() && endLevel.value() == level) {
                return std::nullopt;
            }
            restoreState(savedState);
        }

        advance();
    }

    return errorToken("Unterminated long comment");
}

Opt<i32> Lexer::readLongBracketDelimiter() {
    // 跳过 [=*[ 或 ]=*] 形式的分隔符
    // 返回等号的数量，如果不是有效分隔符返回 std::nullopt
    // 注意：调用此函数时，peek()应该指向第一个'['或']'

    // 保存状态以便回退
    LexerState savedState = saveState();

    i32 count = 0;
    char s = peek();

    // 必须是'['或']'
    if (s != '[' && s != ']') {
        return std::nullopt;
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
    restoreState(savedState);

    return std::nullopt;
}

// =====================================================================
// 标识符和关键字识别
// =====================================================================

Token Lexer::identifier() {
    // 标识符：[a-zA-Z_][a-zA-Z0-9_]*
    while (isAlphaNum(peek()) || peek() == '_') {
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

Token Lexer::decimalNumber() {
    // 整数部分
    consumeDecimalDigits();

    // 小数部分
    if (peek() == '.' && peekNext() != '.') {
        advance();
        consumeDecimalDigits();
    }

    // 指数部分 (e或E)
    if (peek() == 'e' || peek() == 'E') {
        advance();

        // 可选的符号
        if (peek() == '+' || peek() == '-') {
            advance();
        }

        // 指数数字
        if (!isDigit(peek())) {
            return errorToken("Invalid number: expected digits after exponent");
        }

        consumeDecimalDigits();
    }

    if (peek() == '.' && peekNext() != '.') {
        advance();
        consumeDecimalDigits();
        return errorToken("Malformed number");
    }

    // 读取尾随的字母/下划线以捕获 Lua 5.1 定义的非法数字形式（如 123abc）
    consumeMalformedNumberSuffix();

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
                if (ch == '.')
                    ch = locPoint;
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

Token Lexer::hexadecimalNumber() {
    // scanToken 已经消费了前导 '0'，这里消费 'x' 或 'X'
    advance();

    if (!isHexDigit(peek())) {
        return errorToken("Invalid hexadecimal number: expected hex digits after 0x");
    }

    // 十六进制数字
    consumeHexDigits();

    // 如果后续紧跟字母/下划线，按照 Lua 5.1 语义视为格式错误
    if (isAlpha(peek()) || peek() == '_') {
        consumeMalformedNumberSuffix();
        return errorToken("Malformed hexadecimal number");
    }

    // 转换为数字
    Token token = makeToken(TokenType::Number);

    char* end = nullptr;
    f64 value = static_cast<f64>(std::strtoll(lexemeBuffer_.c_str(), &end, 16));

    if (nullptr == end) {
        return errorToken("Malformed hexadecimal number");
    }

    auto bufferSize = lexemeBuffer_.size();
    auto lexemeLength = static_cast<usize>(end - lexemeBuffer_.c_str());

    if (lexemeLength != bufferSize) {
        return errorToken("Malformed hexadecimal number");
    }

    token.value = value;
    return token;
}

void Lexer::consumeDecimalDigits() {
    while (isDigit(peek())) {
        advance();
    }
}

void Lexer::consumeHexDigits() {
    while (isHexDigit(peek())) {
        advance();
    }
}

void Lexer::consumeMalformedNumberSuffix() {
    if (!isAlpha(peek()) && peek() != '_') {
        return;
    }

    while (isAlphaNum(peek()) || peek() == '_') {
        advance();
    }
}

// =====================================================================
// 字符串识别
// =====================================================================

Token Lexer::shortString(char quote) {
    Str result;

    while (peek() != quote && !isAtEnd()) {
        if (isNewline(peek())) {
            return errorToken("Unterminated string");
        }

        if (peek() == '\\') {
            if (Opt<Token> error = appendShortStringEscape(result)) {
                return error.value();
            }
        } else {
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

Opt<Token> Lexer::appendShortStringEscape(Str& result) {
    advance(); // 跳过'\'

    if (isAtEnd()) {
        return errorToken("Unterminated string");
    }

    char c = advance();

    if (Opt<char> simpleEscape = decodeSimpleEscape(c)) {
        result += simpleEscape.value();
        return std::nullopt;
    }

    if (isNewline(c)) {
        consumeNewlinePairRemainder(c);
        result += '\n';
        return std::nullopt;
    }

    if (isDigit(c)) {
        return appendDecimalEscape(c, result);
    }

    result += c;
    return std::nullopt;
}

Opt<Token> Lexer::appendDecimalEscape(char firstDigit, Str& result) {
    i32 value = firstDigit - '0';
    i32 count = 1;

    while (count < 3 && isDigit(peek())) {
        value = value * 10 + (peek() - '0');
        advance();
        count++;
    }

    if (value > 255) {
        return errorToken("Decimal escape too large");
    }

    result += static_cast<char>(value);
    return std::nullopt;
}

Token Lexer::longString(i32 level) {
    Str result;

    skipInitialLongLiteralNewline();

    // 跳过长字符串内容，直到找到匹配的结束符
    while (!isAtEnd()) {
        if (peek() == ']') {
            LexerState savedState = saveState();
            Opt<i32> endLevel = readLongBracketDelimiter();
            if (endLevel.has_value() && endLevel.value() == level) {
                Token token = makeToken(TokenType::String);
                token.value = result;
                return token;
            }
            restoreState(savedState);
        }

        appendLongStringChar(result);
    }

    return errorToken("Unterminated long string");
}

void Lexer::skipInitialLongLiteralNewline() {
    if (isNewline(peek())) {
        consumeNewlineSequence();
    }
}

void Lexer::appendLongStringChar(Str& result) {
    if (isNewline(peek())) {
        consumeNewlineSequence();
        result += '\n';
        return;
    }

    result += advance();
}

// =====================================================================
// 词法分析器状态管理
// =====================================================================

Lexer::LexerState Lexer::saveState() const {
    return LexerState{lexemeBuffer_.size(), inputCursor_.save(), tokenStartLine_, tokenStartColumn_};
}

void Lexer::restoreState(const LexerState& state) {
    lexemeBuffer_.resize(state.lexemeLength);
    inputCursor_.restore(state.input);
    tokenStartLine_ = state.tokenStartLine;
    tokenStartColumn_ = state.tokenStartColumn;
}

// =====================================================================
// Token预读机制（支持LL(1)语法分析）
// =====================================================================

Token Lexer::nextToken() {
    // 如果有预读Token，先返回预读Token
    if (lookahead_.has_value()) {
        Token token = lookahead_.value();
        lookahead_ = std::nullopt;
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
// 长字符串检测辅助方法
// =====================================================================

Opt<Token> Lexer::tryLongString() {
    Opt<i32> level = tryReadLongBracketStart();
    if (!level.has_value()) {
        return std::nullopt;
    }

    return longString(level.value());
}

Opt<i32> Lexer::tryReadLongBracketStart() {
    LexerState savedState = saveState();

    // 计算等号数量
    i32 level = 0;
    while (peek() == '=') {
        level++;
        advance();
    }

    // 检查是否为长字符串开始符 [=*[
    if (peek() == '[') {
        advance();
        return level;
    }

    // 不是长字符串，恢复状态
    restoreState(savedState);
    return std::nullopt;
}

// =====================================================================
// 运算符和分隔符处理
// =====================================================================

Token Lexer::handleEqualsSuffix(TokenType singleType, TokenType compoundType) {
    return match('=') ? makeToken(compoundType) : makeToken(singleType);
}

Token Lexer::handleTildeOperator() {
    if (match('=')) {
        return makeToken(TokenType::Ne);
    }

    return errorToken("Unexpected character '~'");
}

Token Lexer::handleDotOperator() {
    if (match('.')) {
        if (match('.')) {
            return makeToken(TokenType::Dots);
        }
        return makeToken(TokenType::Concat);
    }

    if (isDigit(peek())) {
        return decimalNumber();
    }

    return makeToken(static_cast<TokenType>('.'));
}

Token Lexer::handleOperator(char c) {
    if (isSingleCharToken(c)) {
        return makeToken(static_cast<TokenType>(c));
    }

    if (c == '=')
        return handleEqualsSuffix(static_cast<TokenType>('='), TokenType::Eq);
    if (c == '<')
        return handleEqualsSuffix(static_cast<TokenType>('<'), TokenType::Le);
    if (c == '>')
        return handleEqualsSuffix(static_cast<TokenType>('>'), TokenType::Ge);
    if (c == '~')
        return handleTildeOperator();
    if (c == '.')
        return handleDotOperator();

    return errorToken("Unexpected character");
}

// =====================================================================
// 主要的Token解析函数（内部使用）
// =====================================================================

Token Lexer::scanToken() {
    if (Opt<Token> error = skipWhitespace()) {
        return error.value();
    }

    // 清空 lexeme 缓冲区，准备扫描新 token
    lexemeBuffer_.clear();
    beginToken();

    // 检查是否到达文件末尾
    if (isAtEnd()) {
        return makeToken(TokenType::Eos);
    }

    char c = advance();

    // 1. 标识符或关键字：[a-zA-Z_][a-zA-Z0-9_]*
    if (isAlpha(c) || c == '_') {
        return identifier();
    }

    // 2. 数字：十进制或十六进制（0x 或 0X）
    if (isDigit(c)) {
        if (c == '0' && (peek() == 'x' || peek() == 'X')) {
            return hexadecimalNumber();
        }
        return decimalNumber();
    }

    // 3. 字符串：单引号或双引号
    if (c == '"' || c == '\'') {
        return shortString(c);
    }

    // 4. 长字符串：[[ 或 [=[... 或单字符 '['
    if (c == '[') {
        Opt<Token> longStr = tryLongString();
        if (longStr.has_value()) {
            return longStr.value();
        }
        // 不是长字符串，返回单字符'['
        return makeToken(static_cast<TokenType>('['));
    }

    // 5. 运算符和分隔符
    return handleOperator(c);
}

} // namespace Lua
