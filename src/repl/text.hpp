#ifndef LUA_REPL_TEXT_HPP
#define LUA_REPL_TEXT_HPP

#include "common/types.hpp"

#include <cctype>
#include <string_view>

namespace Lua::REPL::detail {

inline bool isSpace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

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

inline bool startsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_TEXT_HPP
