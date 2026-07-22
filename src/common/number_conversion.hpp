#pragma once

/**
 * @file number_conversion.hpp
 * @brief Lua 数值解析、格式化与安全转换辅助函数
 */

#include "common/types.hpp"
#include "runtime/lua_allocator.hpp"

#include <array>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cmath>
#include <expected>
#include <limits>
#include <system_error>

namespace Lua {

/**
 * @brief Lua 整数转换模式。
 */
enum class IntegerConversion : u8 {
    Truncate,
    Exact,
};

/**
 * @brief Lua 整数转换错误。
 */
enum class IntegerConversionError : u8 {
    NotFinite,
    NotIntegral,
    OutOfRange,
};

/**
 * @brief 将 Lua 数值安全转换为 32 位整数。
 * @param value 待转换的数值。
 * @param mode 转换模式。
 * @return 转换后的整数；失败时返回具体错误。
 */
[[nodiscard]] inline std::expected<i32, IntegerConversionError>
checkedLuaInteger(LuaNumber value, IntegerConversion mode = IntegerConversion::Truncate) noexcept {
    if (!std::isfinite(value)) {
        return std::unexpected(IntegerConversionError::NotFinite);
    }

    const LuaNumber truncated = std::trunc(value);
    if (mode == IntegerConversion::Exact && truncated != value) {
        return std::unexpected(IntegerConversionError::NotIntegral);
    }

    if (truncated < static_cast<LuaNumber>(std::numeric_limits<i32>::min()) ||
        truncated > static_cast<LuaNumber>(std::numeric_limits<i32>::max())) {
        return std::unexpected(IntegerConversionError::OutOfRange);
    }

    return static_cast<i32>(truncated);
}

/**
 * @brief 按 Lua 数字语法解析字符串。
 * @param text 待解析的文本。
 * @param out 用于接收解析结果。
 * @param allocator 可选的内存分配器。
 * @return 解析成功时返回 true，否则返回 false。
 */
inline bool luaStringToNumber(StrView text, LuaNumber& out, LuaAllocator* allocator = nullptr) {
    LuaString copy(text.begin(), text.end(), LuaStdAllocator<char>(allocator));
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

/**
 * @brief 将 Lua 数值格式化到调用方提供的缓冲区中。
 * @param value 待格式化的数值。
 * @param buffer 输出缓冲区。
 * @return 指向格式化结果的字符串视图。
 */
inline StrView luaNumberToView(LuaNumber value, std::array<char, 64>& buffer) {
    const auto result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::general, 14);

    if (result.ec != std::errc{}) {
        throw std::runtime_error("failed to format Lua number");
    }

    return StrView(buffer.data(), static_cast<usize>(result.ptr - buffer.data()));
}

/**
 * @brief 将 Lua 数值转换为字符串。
 * @param value 待转换的数值。
 * @return 格式化后的字符串。
 */
inline Str luaNumberToString(LuaNumber value) {
    std::array<char, 64> buffer{};
    return Str(luaNumberToView(value, buffer));
}

} // namespace Lua
