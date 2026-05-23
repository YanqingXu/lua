#pragma once

/**
 * @file parser_utils.hpp
 * @brief Shared parser helpers that do not need Parser object state.
 */

#include "token.hpp"

#include <variant>

namespace Lua::ParserUtils {

/**
 * @brief Borrow the semantic string carried by a token.
 *
 * String tokens prefer the decoded TokenValue storage; all other textual
 * tokens fall back to the original lexeme. Callers that store the result past
 * the current token lifetime must copy it into Str explicitly.
 */
[[nodiscard]] inline StrView tokenString(const Token& token) noexcept {
    if (std::holds_alternative<Str>(token.value)) {
        return std::get<Str>(token.value);
    }
    return token.lexeme;
}

} // namespace Lua::ParserUtils
