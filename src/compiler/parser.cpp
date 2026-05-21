/**
 * @file parser.cpp
 * @brief Lua语法分析器实现
 */

#include "parser.hpp"
#include <sstream>

namespace Lua {

// =====================================================================
// 辅助函数：生成官方 Lua 风格的错误消息
// =====================================================================

/**
 * @brief 获取 Token 的可读字符串表示（用于错误消息）
 *
 * 参考官方 Lua 5.1.5 的 txtToken() 函数：
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
 * 参考官方 Lua 5.1.5 的 luaX_lexerror() 函数：
 * 错误消息格式：message near 'token'
 */
Str Parser::errorWithNear(const Str& message, const Token& token) {
    return message + " near '" + getTokenText(token) + "'";
}

// =====================================================================
// 构造函数和基本Token管理
// =====================================================================

Parser::Parser(const Str& source)
    : lexer_(source)
    , current_(lexer_.nextToken()) {
}

Parser::Parser(const Str& source, RuntimeServices& services)
    : lexer_(source)
    , current_(lexer_.nextToken())
    , services_(&services) {
}

const Token& Parser::current() const {
    return current_;
}

void Parser::advance() {
    current_ = lexer_.nextToken();
}

Token Parser::peek() {
    // 使用Lexer的peekToken()方法实现高效的Token预读
    // 支持LL(1)语法分析，避免复制整个Lexer状态
    return lexer_.peekToken();
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const Str& message) {
    if (!match(type)) {
        error(message);
    }
}

void Parser::error(const Str& message) {
    // 生成官方 Lua 风格的错误消息：message near 'token'
    // 参考官方 Lua 5.1.5 的 luaX_syntaxerror() 函数
    Str fullMessage = errorWithNear(message, current_);
    throw ParseError(fullMessage, current_.line, current_.column);
}

void Parser::reportError(const Str& message) {
    // 生成官方 Lua 风格的错误消息
    Str fullMessage = errorWithNear(message, current_);
    errors_.emplace_back(fullMessage, current_.line, current_.column);

    // 设置 panic 模式（为将来的错误恢复机制预留）
    panicMode_ = true;
}

void Parser::synchronize() {
    // 重置 panic 模式
    panicMode_ = false;

    // 跳过 token 直到找到语句边界
    while (!check(TokenType::Eos)) {
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

// =====================================================================
// 主解析函数
// =====================================================================

std::expected<Chunk, ParseError> Parser::parse() {
    try {
        Chunk chunk;

        // 解析语句块
        chunk.statements = parseBlock();

        // 确保到达文件末尾
        if (!check(TokenType::Eos)) {
            error("Expected end of file");
        }

        return chunk;
    } catch (const ParseError& error) {
        return std::unexpected(error);
    }
}

// Grammar productions are implemented in the parser_*.cpp shards.

} // namespace Lua


