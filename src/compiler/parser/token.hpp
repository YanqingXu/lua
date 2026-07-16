#pragma once

/**
 * @file token.hpp
 * @brief Lua词法标记定义
 *
 * 定义语法分析阶段使用的Token类型、语义值、词素文本和源代码位置信息。
 */

#include "common/types.hpp"
#include <variant>

namespace Lua {

enum class TokenType : i32 {
    And = 257,
    Break,
    Do,
    Else,
    Elseif,
    End,
    False,
    For,
    Function,
    If,
    In,
    Local,
    Nil,
    Not,
    Or,
    Repeat,
    Return,
    Then,
    True,
    Until,
    While,

    Concat,
    Dots,
    Eq,
    Ge,
    Le,
    Ne,

    Number,
    String,
    Name,

    Eos,
    Error
};

using TokenValue = Var<std::monostate, f64, Str>;

struct Token {
    TokenType type;
    TokenValue value;
    Str lexeme;
    Str errorMessage;
    i32 line;
    i32 column;

    Token() noexcept : type(TokenType::Eos), value(std::monostate{}), lexeme(), errorMessage(), line(1), column(1) {}

    Token(TokenType t, StrView lex, i32 ln, i32 col)
        : type(t), value(std::monostate{}), lexeme(lex), errorMessage(), line(ln), column(col) {}

    bool isNumber() const noexcept {
        return type == TokenType::Number;
    }

    bool isString() const noexcept {
        return type == TokenType::String;
    }

    bool isName() const noexcept {
        return type == TokenType::Name;
    }

    bool isKeyword() const noexcept {
        return type >= TokenType::And && type <= TokenType::While;
    }
};

const char* tokenTypeToString(TokenType type);

} // namespace Lua
