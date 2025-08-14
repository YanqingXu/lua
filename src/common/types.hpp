#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <variant>
#include <optional>
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
    using StrView = std::string_view;  // Added for efficient string references

    // Container templates
    template<typename T>
    using Vec = std::vector<T>;

    template<typename K, typename V>
    using HashMap = std::unordered_map<K, V>;

	template<typename... Args>
	using HashSet = std::unordered_set<Args...>;

    // Other type utilities
    template<typename... Types>
    using Var = std::variant<Types...>;

    template<typename T>
    using Opt = std::optional<T>;

    template<typename T>
    using Atom = std::atomic<T>;

    using Mtx = std::mutex;
    using ScopedLock = std::scoped_lock<Mtx>;
    using SharedMtx = std::shared_mutex;
    using UniqueLock = std::unique_lock<SharedMtx>;
    using SharedLock = std::shared_lock<SharedMtx>;

    // Smart pointers
    template<typename T>
    using Ptr = std::shared_ptr<T>;

    template<typename T>
    using WPtr = std::weak_ptr<T>;

    template<typename T>
    using UPtr = std::unique_ptr<T>;

    // Create a shared_ptr
    template<typename T, typename... Args>
    Ptr<T> make_ptr(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // Create a unique_ptr
    template<typename T, typename... Args>
    UPtr<T> make_unique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Lua specific types
    using LuaInteger = i64;
    using LuaNumber = f64;
    using LuaBoolean = bool;

    // Forward declarations for error handling
    class LuaState;
    struct CallInfo;

    // Error handling with enhanced debugging information
    class LuaException : public std::runtime_error {
    private:
        std::string message_;
        std::string filename_;
        i32 line_;
        i32 column_;
        std::string functionName_;
        std::vector<std::string> callStack_;
        std::string contextInfo_;

    public:
        // Basic constructors (backward compatibility)
        explicit LuaException(const std::string& message)
            : std::runtime_error(message), message_(message), line_(-1), column_(-1) {}
        explicit LuaException(const char* message)
            : std::runtime_error(message), message_(message), line_(-1), column_(-1) {}

        // Enhanced constructor with location information
        LuaException(const std::string& message, const std::string& filename, i32 line, i32 column = -1)
            : std::runtime_error(message), message_(message), filename_(filename), line_(line), column_(column) {}

        // Enhanced constructor with full context
        LuaException(const std::string& message, const std::string& filename, i32 line,
                    const std::string& functionName, const std::vector<std::string>& callStack)
            : std::runtime_error(message), message_(message), filename_(filename), line_(line),
              column_(-1), functionName_(functionName), callStack_(callStack) {}

        // Accessors
        const std::string& getMessage() const { return message_; }
        const std::string& getFilename() const { return filename_; }
        i32 getLine() const { return line_; }
        i32 getColumn() const { return column_; }
        const std::string& getFunctionName() const { return functionName_; }
        const std::vector<std::string>& getCallStack() const { return callStack_; }
        const std::string& getContextInfo() const { return contextInfo_; }

        // Setters for building error information
        void setFilename(const std::string& filename) { filename_ = filename; }
        void setLine(i32 line) { line_ = line; }
        void setColumn(i32 column) { column_ = column; }
        void setFunctionName(const std::string& functionName) { functionName_ = functionName; }
        void setCallStack(const std::vector<std::string>& callStack) { callStack_ = callStack; }
        void setContextInfo(const std::string& contextInfo) { contextInfo_ = contextInfo; }

        // Generate formatted error message
        std::string getFormattedMessage() const;

        // Generate call stack trace
        std::string getStackTrace() const;

        // Override what() to provide enhanced information
        const char* what() const noexcept override;

    private:
        mutable std::string formattedMessage_; // Cache for formatted message
    };
}