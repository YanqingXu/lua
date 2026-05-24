/**
 * @file parser.cpp
 * @brief Lua语法分析器实现
 */

#include "parser_impl.hpp"
#include <sstream>

namespace Lua {

// =====================================================================
// 辅助函数：生成官方 Lua 风格的错误消息
// =====================================================================

/**
 * @brief 获取 Token 的可读字符串表示（用于错误消息）
 *
 * Lua 风格的 Token 文本格式：
 * - 对于标识符、字符串、数字：返回实际的词素内容
 * - 对于 EOF：返回 "<eof>"
 * - 对于其他 token：返回词素内容
 */
static Str getTokenText(const Token& token) {
    if (token.type == TokenType::Eos) {
        return "<eof>";
    }
    // 对于大多数 token，直接返回其词素
    if (!token.lexeme.empty()) {
        return token.lexeme;
    }
    // 如果没有词素，返回 token 类型的字符串表示
    return tokenTypeToString(token.type);
}

/**
 * @brief 生成带有 "near 'X'" 后缀的错误消息
 *
 * 错误消息格式：message near 'token'
 */
Str Parser::Impl::errorWithNear(const Str& message, const Token& token) {
    const Str& diagnostic = (token.type == TokenType::Error && !token.errorMessage.empty())
        ? token.errorMessage
        : message;
    return diagnostic + " near '" + getTokenText(token) + "'";
}

// =====================================================================
// 构造函数和基本Token管理
// =====================================================================

Parser::Parser(const Str& source)
    : Parser(source, ParserOptions{}) {
}

Parser::Parser(const Str& source, ParserOptions options)
    : impl_(makeUnique<Impl>(source, options)) {
}

Parser::Parser(const Str& source, RuntimeServices& services)
    : Parser(source, services, ParserOptions{}) {
}

Parser::Parser(const Str& source, RuntimeServices& services, ParserOptions options)
    : impl_(makeUnique<Impl>(source, services, options)) {
}

Parser::~Parser() = default;
Parser::Parser(Parser&&) noexcept = default;
Parser& Parser::operator=(Parser&&) noexcept = default;

std::expected<Chunk, ParseError> Parser::parse() {
    return impl_->parse();
}

const Vec<ParseError>& Parser::diagnostics() const noexcept {
    return impl_->diagnostics();
}

Parser::Impl::Impl(const Str& source, ParserOptions options)
    : tokenStream_(source)
    , recoveryStrategy_(makeRecoveryStrategy(options.recoveryMode)) {
    diagnosticObservers_.push_back(&diagnosticCollector_);
}

Parser::Impl::Impl(const Str& source, RuntimeServices& services, ParserOptions options)
    : tokenStream_(source)
    , services_(&services)
    , recoveryStrategy_(makeRecoveryStrategy(options.recoveryMode)) {
    diagnosticObservers_.push_back(&diagnosticCollector_);
}

const Token& Parser::Impl::current() const {
    return tokenStream_.current();
}

void Parser::Impl::advance() {
    tokenStream_.advance();
}

Token Parser::Impl::peek() {
    // 使用Lexer的peekToken()方法实现高效的Token预读
    // 支持LL(1)语法分析，避免复制整个Lexer状态
    return tokenStream_.peek();
}

bool Parser::Impl::check(TokenType type) const {
    return tokenStream_.check(type);
}

bool Parser::Impl::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::Impl::expect(TokenType type, const Str& message) {
    if (!match(type)) {
        error(message);
    }
}

void Parser::Impl::error(const Str& message) {
    errorAt(current(), message);
}

void Parser::Impl::errorAt(const Token& token, const Str& message) {
    // 生成 Lua 风格的错误消息：message near 'token'
    Str fullMessage = errorWithNear(message, token);
    ParseError parseError(fullMessage, token.line, token.column);
    publishDiagnostic(parseError);
    throw parseError;
}

void Parser::Impl::reportError(const Str& message) {
    reportErrorAt(current(), message);
}

void Parser::Impl::reportErrorAt(const Token& token, const Str& message) {
    // 生成官方 Lua 风格的错误消息
    Str fullMessage = errorWithNear(message, token);
    publishDiagnostic(ParseError(fullMessage, token.line, token.column));
}

void Parser::Impl::publishDiagnostic(const ParseError& error) {
    for (ParseDiagnosticObserver* observer : diagnosticObservers_) {
        observer->onParseDiagnostic(error);
    }
}

void Parser::Impl::synchronize() {
    // 跳过 token 直到找到语句边界
    while (!check(TokenType::Eos)) {
        if (match(static_cast<TokenType>(';'))) {
            return;
        }

        // 检查是否到达块结束符
        if (check(TokenType::End) ||
            check(TokenType::Else) ||
            check(TokenType::Elseif) ||
            check(TokenType::Until)) {
            return;  // 不消费这些 token，让调用者处理
        }

        // 检查是否到达语句开始符
        if (check(TokenType::Local) ||
            check(TokenType::Function) ||
            check(TokenType::If) ||
            check(TokenType::While) ||
            check(TokenType::For) ||
            check(TokenType::Repeat) ||
            check(TokenType::Return) ||
            check(TokenType::Break)) {
            return;  // 不消费这些 token，让调用者处理
        }

        // 继续跳过当前 token
        advance();
    }
}

bool Parser::Impl::canRecoverFrom(const ParseError& error) const {
    return recoveryStrategy_->canRecover(*this, error);
}

void Parser::Impl::recoverAfterError() {
    recoveryStrategy_->recover(*this);
}

UPtr<Parser::Impl::ErrorRecoveryStrategy> Parser::Impl::makeRecoveryStrategy(ParseRecoveryMode mode) {
    switch (mode) {
        case ParseRecoveryMode::StatementBoundary:
            return makeUnique<StatementBoundaryRecoveryStrategy>();
        case ParseRecoveryMode::FailFast:
        default:
            return makeUnique<FailFastRecoveryStrategy>();
    }
}

// =====================================================================
// 主解析函数
// =====================================================================

std::expected<Chunk, ParseError> Parser::Impl::parse() {
    diagnosticCollector_.clear();

    try {
        Chunk chunk;

        // 解析语句块
        chunk.statements = parseBlock();

        // 确保到达文件末尾
        if (!check(TokenType::Eos)) {
            error("Expected end of file");
        }

        if (!diagnosticCollector_.empty()) {
            return std::unexpected(diagnosticCollector_.first());
        }

        return chunk;
    } catch (const ParseError& error) {
        return std::unexpected(error);
    }
}

const Vec<ParseError>& Parser::Impl::diagnostics() const noexcept {
    return diagnosticCollector_.diagnostics();
}

// Grammar productions are implemented in the parser_*.cpp shards.

} // namespace Lua


