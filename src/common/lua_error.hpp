/**
 * @file lua_error.hpp
 * @brief Unified Lua exception hierarchy.
 *
 * LuaError is the interpreter exception base and still preserves the
 * Lua 5.1 error(obj) Value-object path. ParseError, CodegenError,
 * RuntimeError, and MemoryError derive from it while std::runtime_error
 * remains a catch boundary for callers that need standard exception
 * compatibility.
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"
#include <stdexcept>

namespace Lua {

class LuaError : public std::runtime_error {
public:
    explicit LuaError(const Str& message)
        : std::runtime_error(message)
        , errorObj_()
        , hasErrorObject_(false) {}

    explicit LuaError(const char* message)
        : LuaError(Str(message ? message : "")) {}

    explicit LuaError(Value errorObj)
        : std::runtime_error(messageFromValue(errorObj))
        , errorObj_(std::move(errorObj))
        , hasErrorObject_(true) {}

    const Value& getErrorObject() const noexcept { return errorObj_; }
    bool hasErrorObject() const noexcept { return hasErrorObject_; }

private:
    static Str messageFromValue(const Value& errorObj) {
        if (errorObj.isString()) {
            return errorObj.asString()->c_str();
        }
        return "(error object is not a string)";
    }

    Value errorObj_;
    bool hasErrorObject_;
};

/**
 * @brief Syntax error with source location.
 */
class ParseError : public LuaError {
public:
    ParseError(const Str& message, i32 line, i32 column)
        : LuaError(message)
        , line_(line)
        , column_(column) {}

    i32 getLine() const { return line_; }
    i32 getColumn() const { return column_; }

private:
    i32 line_;
    i32 column_;
};

/**
 * @brief Bytecode generation error.
 */
class CodegenError : public LuaError {
public:
    using LuaError::LuaError;
};

/**
 * @brief VM/runtime error.
 */
class RuntimeError : public LuaError {
public:
    using LuaError::LuaError;
};

/**
 * @brief Memory and resource exhaustion error.
 */
class MemoryError : public RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

} // namespace Lua
