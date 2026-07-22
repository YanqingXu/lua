/**
 * @file lua_error.hpp
 * @brief 统一的 Lua 异常层次结构
 *
 * LuaError 是解释器异常基类，同时保留 Lua 5.1 error(obj) 的 Value 对象传递路径。
 * ParseError、CodegenError、RuntimeError 与 MemoryError 均派生自它；对于需要兼容标准异常的
 * 调用者，std::runtime_error 仍是捕获边界。
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"
#include <stdexcept>

namespace Lua {

/** @brief Lua 解释器异常层次结构的基类。 */
class LuaError : public std::runtime_error {
public:
    explicit LuaError(const Str& message) : std::runtime_error(message), errorObj_(), hasErrorObject_(false) {}

    explicit LuaError(const char* message) : LuaError(Str(message ? message : "")) {}

    explicit LuaError(Value errorObj)
        : std::runtime_error(messageFromValue(errorObj)), errorObj_(std::move(errorObj)), hasErrorObject_(true) {}

    const Value& getErrorObject() const noexcept {
        return errorObj_;
    }
    bool hasErrorObject() const noexcept {
        return hasErrorObject_;
    }

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
 * @brief 带源码位置的语法错误
 */
class ParseError : public LuaError {
public:
    ParseError(const Str& message, i32 line, i32 column) : LuaError(message), line_(line), column_(column) {}

    i32 getLine() const {
        return line_;
    }
    i32 getColumn() const {
        return column_;
    }

private:
    i32 line_;
    i32 column_;
};

/**
 * @brief 字节码生成错误。
 */
class CodegenError : public LuaError {
public:
    using LuaError::LuaError;
};

/**
 * @brief VM 或运行时错误
 */
class RuntimeError : public LuaError {
public:
    using LuaError::LuaError;
};

/**
 * @brief 内存与资源耗尽错误
 */
class MemoryError : public RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

/**
 * @brief Lua 栈或调用深度耗尽，并作为运行时错误报告
 *
 * Lua 5.1 区分逻辑栈溢出与分配器失败：保护调用必须保留“栈溢出”消息并返回 LUA_ERRRUN，
 * 而不能将其替换为固定的 LUA_ERRMEM 对象。
 */
class StackOverflowError : public RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

} // namespace Lua
