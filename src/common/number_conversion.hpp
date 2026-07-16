#pragma once

#include "common/types.hpp"

#include <array>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <system_error>

namespace Lua {

inline bool luaStringToNumber(StrView text, LuaNumber& out) {
    Str copy(text);
    const char* start = copy.c_str();
    char* end = nullptr;

    errno = 0;
    LuaNumber value = std::strtod(start, &end);
    if (end == start) {
        return false;
    }

    while (end != nullptr && *end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0) {
        ++end;
    }

    if (end == nullptr || *end != '\0' || errno == ERANGE) {
        return false;
    }

    out = value;
    return true;
}

inline StrView luaNumberToView(LuaNumber value, std::array<char, 64>& buffer) {
    const auto result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::general, 14);

    if (result.ec != std::errc{}) {
        throw std::runtime_error("failed to format Lua number");
    }

    return StrView(buffer.data(), static_cast<usize>(result.ptr - buffer.data()));
}

inline Str luaNumberToString(LuaNumber value) {
    std::array<char, 64> buffer{};
    return Str(luaNumberToView(value, buffer));
}

} // namespace Lua
