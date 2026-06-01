/**
 * @file parser.cpp
 * @brief Lua语法分析器核心实现
 *
 * 实现解析入口、Token管理、错误诊断发布与语法错误恢复策略。
 */

#include "parser_impl.hpp"
#include <sstream>

namespace Lua {

static Str getTokenText(const Token& token) {
    if (token.type == TokenType::Eos) {
        return "<eof>";
    }

    if (!token.lexeme.empty()) {
        return token.lexeme;
    }

    return tokenTypeToString(token.type);
}

Str Parser::Impl::errorWithNear(const Str& message, const Token& token) {
    const Str& diagnostic = (token.type == TokenType::Error && !token.errorMessage.empty())
        ? token.errorMessage
        : message;
    return diagnostic + " near '" + getTokenText(token) + "'";
}

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

const Token& Parser::Impl::previous() const {
    return tokenStream_.previous();
}

void Parser::Impl::advance() {
    tokenStream_.advance();
}

Token Parser::Impl::peek() {
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

[[noreturn]] void Parser::Impl::error(const Str& message) {
    errorAt(current(), message);
}

[[noreturn]] void Parser::Impl::errorAt(const Token& token, const Str& message) {
    Str fullMessage = errorWithNear(message, token);
    ParseError parseError(fullMessage, token.line, token.column);
    publishDiagnostic(parseError);
    throw parseError;
}

void Parser::Impl::reportError(const Str& message) {
    reportErrorAt(current(), message);
}

void Parser::Impl::reportErrorAt(const Token& token, const Str& message) {
    Str fullMessage = errorWithNear(message, token);
    publishDiagnostic(ParseError(fullMessage, token.line, token.column));
}

void Parser::Impl::publishDiagnostic(const ParseError& error) {
    for (ParseDiagnosticObserver* observer : diagnosticObservers_) {
        observer->onParseDiagnostic(error);
    }
}

void Parser::Impl::synchronize() {
    while (!check(TokenType::Eos)) {
        if (match(static_cast<TokenType>(';'))) {
            return;
        }

        if (check(TokenType::End) ||
            check(TokenType::Else) ||
            check(TokenType::Elseif) ||
            check(TokenType::Until)) {
            return;
        }

        if (check(TokenType::Local) ||
            check(TokenType::Function) ||
            check(TokenType::If) ||
            check(TokenType::While) ||
            check(TokenType::For) ||
            check(TokenType::Repeat) ||
            check(TokenType::Return) ||
            check(TokenType::Break)) {
            return;
        }

        advance();
    }
}

bool Parser::Impl::canRecoverFrom(const ParseError& error) const {
    return recoveryStrategy_->canRecover(*this, error);
}

void Parser::Impl::recoverAfterError() {
    recoveryStrategy_->recover(*this);
}

void Parser::Impl::enterFunctionSyntaxScope(i32 line, const Vec<Str>& params) {
    FunctionSyntaxScope scope;
    scope.line = line;
    for (const Str& param : params) {
        if (param != "...") {
            scope.locals.push_back(param);
        }
    }
    functionScopes_.push_back(std::move(scope));
}

void Parser::Impl::leaveFunctionSyntaxScope() {
    if (!functionScopes_.empty()) {
        functionScopes_.pop_back();
    }
}

void Parser::Impl::declareLocalName(const Str& name, const Token& token) {
    if (functionScopes_.empty() || containsName(functionScopes_.back().locals, name)) {
        return;
    }

    FunctionSyntaxScope& scope = functionScopes_.back();
    if (scope.locals.size() >= MAX_LOCAL_VARIABLES) {
        throw ParseError(
            "function at line " + std::to_string(scope.line) + " has more than 200 local variables",
            token.line,
            token.column
        );
    }
    scope.locals.push_back(name);
}

void Parser::Impl::noteNameUse(const Str& name, const Token& token) {
    if (functionScopes_.empty()) {
        return;
    }

    i32 owner = -1;
    for (i32 i = static_cast<i32>(functionScopes_.size()) - 1; i >= 0; --i) {
        if (containsName(functionScopes_[static_cast<usize>(i)].locals, name)) {
            owner = i;
            break;
        }
    }
    if (owner < 0 || owner == static_cast<i32>(functionScopes_.size()) - 1) {
        return;
    }

    for (i32 i = static_cast<i32>(functionScopes_.size()) - 1; i > owner; --i) {
        FunctionSyntaxScope& scope = functionScopes_[static_cast<usize>(i)];
        if (containsName(scope.upvalues, name)) {
            continue;
        }
        if (scope.upvalues.size() >= MAX_UPVALUES_PER_FUNCTION) {
            throw ParseError(
                "function at line " + std::to_string(scope.line) + " has more than 60 upvalues",
                token.line,
                token.column
            );
        }
        scope.upvalues.push_back(name);
    }
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

std::expected<Chunk, ParseError> Parser::Impl::parse() {
    diagnosticCollector_.clear();
    functionScopes_.clear();
    enterFunctionSyntaxScope(1);

    try {
        Chunk chunk;

        chunk.statements = parseBlock();

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

}
