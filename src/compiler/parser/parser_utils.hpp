#pragma once

/**
 * @file parser_utils.hpp
 * @brief Lua语法分析器通用辅助函数
 *
 * 提供不依赖Parser对象状态的Token语义值访问等共享解析辅助能力。
 */

#include "token.hpp"

#include <variant>

namespace Lua::ParserUtils {

[[nodiscard]] inline StrView tokenString(const Token& token) noexcept {
    if (std::holds_alternative<TokenString>(token.value)) {
        const TokenString& value = std::get<TokenString>(token.value);
        return StrView(value.data(), value.size());
    }
    if (std::holds_alternative<Str>(token.value)) {
        return std::get<Str>(token.value);
    }
    return StrView(token.lexeme.data(), token.lexeme.size());
}

} // namespace Lua::ParserUtils
