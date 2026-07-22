#pragma once

/**
 * @file token.hpp
 * @brief Lua词法标记定义
 *
 * 定义语法分析阶段使用的词法单元类型、语义值、词素文本和源代码位置信息。
 */

#include "common/types.hpp"
#include "runtime/lua_allocator.hpp"
#include <variant>

namespace Lua {

/** @brief Lua 词法单元类型。 */
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

using TokenString = LuaOwnedString;
using TokenValue = Var<std::monostate, f64, Str, TokenString>;

inline bool operator==(const TokenString& lhs, const Str& rhs) noexcept {
    return StrView(lhs.data(), lhs.size()) == StrView(rhs);
}

inline bool operator==(const Str& lhs, const TokenString& rhs) noexcept {
    return StrView(lhs) == StrView(rhs.data(), rhs.size());
}

inline bool operator!=(const TokenString& lhs, const Str& rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator!=(const Str& lhs, const TokenString& rhs) noexcept {
    return !(lhs == rhs);
}

/** @brief 带词素与源码位置的词法单元。 */
struct Token {
    TokenType type;
    TokenValue value;
    TokenString lexeme;
    TokenString errorMessage;
    i32 line;
    i32 column;

    Token() : type(TokenType::Eos), value(std::monostate{}), lexeme(), errorMessage(), line(1), column(1) {}

    Token(TokenType t, StrView lex, i32 ln, i32 col, const LuaAllocator* allocator = nullptr)
        : type(t), value(std::monostate{}), lexeme(lex.begin(), lex.end(), LuaSnapshotStdAllocator<char>(allocator)),
          errorMessage(LuaSnapshotStdAllocator<char>(allocator)), line(ln), column(col) {}

    void setStringValue(StrView text) {
        const auto allocator = lexeme.get_allocator();
        if (allocator.getLuaAllocator().isConfigured()) {
            value.emplace<TokenString>(text.begin(), text.end(), allocator);
        } else {
            value.emplace<Str>(text);
        }
    }

    void setErrorMessage(StrView text) {
        errorMessage.assign(text.begin(), text.end());
    }

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
