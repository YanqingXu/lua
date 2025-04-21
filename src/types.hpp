#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <stdexcept>

namespace Lua {
    // Signed integers
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    // Unsigned integers
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    // Floating point
    using f32 = float;
    using f64 = double;

    // Size and difference types
    using usize = size_t;
    using isize = ptrdiff_t;

    // String types
    using Str = std::string;
    using StringView = std::string_view;  // Added for efficient string references

    // Container templates
    template<typename T>
    using Vec = std::vector<T>;

    template<typename K, typename V>
    using HashMap = std::unordered_map<K, V>;

    // Smart pointers
    template<typename T>
    using Ptr = std::shared_ptr<T>;

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    // Create a shared_ptr
    template<typename T, typename... Args>
    Ptr<T> make_ptr(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // Create a unique_ptr
    template<typename T, typename... Args>
    UniquePtr<T> make_unique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Lua specific types
    using LuaInteger = i64;
    using LuaNumber = f64;
    using LuaBoolean = bool;

    // Lua instruction is an unsigned 32-bit integer
    using Instruction = u32;

    // Error handling
    class LuaException : public std::runtime_error {
    public:
        explicit LuaException(const std::string& message) : std::runtime_error(message) {}
        explicit LuaException(const char* message) : std::runtime_error(message) {}
    };
}