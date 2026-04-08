/**
 * @file lua_error.hpp
 * @brief 统一的 Lua 运行时错误类型
 *
 * 替代当前散落的 std::runtime_error，成为 VM 内部的错误抛出类型。
 * 错误对象本身就是一个 Lua Value（可以是 string，也可以是 table 或任意类型），
 * 与 Lua 5.1 的 error(msg) 语义对齐。
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include <exception>

namespace Lua {

class LuaError : public std::exception {
public:
    explicit LuaError(Value errorObj)
        : errorObj_(std::move(errorObj)) {}

    const char* what() const noexcept override {
        if (cachedWhat_.empty()) {
            if (errorObj_.isString()) {
                cachedWhat_ = errorObj_.asString()->c_str();
            } else {
                cachedWhat_ = "(error object is not a string)";
            }
        }
        return cachedWhat_.c_str();
    }

    const Value& getErrorObject() const noexcept { return errorObj_; }

private:
    Value errorObj_;
    mutable Str cachedWhat_;
};

} // namespace Lua
