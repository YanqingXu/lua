#pragma once

#include "../common/types.hpp"
#include "../vm/value.hpp"
#include "../vm/state.hpp"
#include "core/lib_define.hpp"
#include <string>
#include <optional>

namespace Lua {
namespace Lib {

/**
 * Utility functions for base library implementation
 */
class BaseLibUtils {
public:
    /**
     * Convert a Value to its string representation
     */
    static std::string toString(const Value& value);

    /**
     * Get the type name of a Value
     */
    static std::string getTypeName(const Value& value);

    /**
     * Check if a Value is truthy in Lua semantics
     * false and nil are falsy, everything else is truthy
     */
    static bool isTruthy(const Value& value);

    /**
     * Convert string to number with base support
     */
    static std::optional<f64> stringToNumber(StrView str, i32 base = 10);

    /**
     * Raw equality comparison (no metamethods)
     */
    static bool rawEqual(const Value& a, const Value& b);

    /**
     * Get raw length of string or table
     */
    static size_t rawLength(const Value& value);
};

/**
 * Argument checking utilities
 */
class ArgUtils {
public:
    /**
     * Check exact argument count
     */
    static void checkArgCount(State* state, i32 expected, const char* funcName);

    /**
     * Check argument count in range
     */
    static void checkArgCount(State* state, i32 minArgs, i32 maxArgs, const char* funcName);

    /**
     * Check if argument is a number and return it
     */
    static Value checkNumber(State* state, i32 index, const char* funcName);

    /**
     * Check if argument is a table and return it
     */
    static Value checkTable(State* state, i32 index, const char* funcName);

    /**
     * Check if argument is a string and return it
     */
    static Value checkString(State* state, i32 index, const char* funcName);
};

/**
 * Error handling utilities
 */
class ErrorUtils {
public:
    /**
     * Raise a general error
     */
    static void error(State* state, const std::string& message, i32 level = 1);

    /**
     * Raise an argument error
     */
    static void argumentError(State* state, i32 argIndex, const std::string& message);

    /**
     * Raise a type error
     */
    static void typeError(State* state, i32 argIndex, const std::string& expectedType);
};

} // namespace Lib
} // namespace Lua