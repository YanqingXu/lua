#pragma once

#include "common/types.hpp"

#include <cerrno>
#include <cctype>
#include <cstdlib>

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

    while (end != nullptr && *end != '\0' &&
           std::isspace(static_cast<unsigned char>(*end)) != 0) {
        ++end;
    }

    if (end == nullptr || *end != '\0' || errno == ERANGE) {
        return false;
    }

    out = value;
    return true;
}

} // namespace Lua
