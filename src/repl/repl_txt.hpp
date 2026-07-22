#ifndef LUA_REPL_TXT_HPP
#define LUA_REPL_TXT_HPP

/**
 * @file repl_txt.hpp
 * @brief REPL 文本裁剪、分词与转义辅助函数
 */

#include "common/types.hpp"

#include <cctype>
#include <string_view>

namespace Lua::REPL::detail {

/** @brief 判断字符是否为空白字符。 */
inline bool isSpace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

/** @brief 返回移除首尾空白后的字符串副本。 */
inline Str trimCopy(const Str& text) {
    usize first = 0;
    while (first < text.size() && isSpace(text[first])) {
        first++;
    }

    usize last = text.size();
    while (last > first && isSpace(text[last - 1])) {
        last--;
    }

    return text.substr(first, last - first);
}

/** @brief 判断文本是否以指定前缀开头。 */
inline bool startsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_TXT_HPP
