#pragma once

/**
 * @file lib_module.hpp
 * @brief Lua 标准库模块的公共抽象接口
 */

#include "common/types.hpp"

namespace Lua {

class LuaState;

/** @brief 标准库 C 函数签名。 */
using LibCFunction = i32 (*)(LuaState*);

/** @brief 标准库函数名称与入口的静态描述项。 */
struct LibFunctionEntry {
    const char* name;
    LibCFunction func;
};

/** @brief 标准库模块的注册与初始化抽象接口。 */
class LibModule {
public:
    virtual ~LibModule() = default;

    /** @brief 获取模块名称。 */
    virtual const char* getName() const = 0;

    /** @brief 将模块函数注册到指定 Lua 状态。 */
    virtual void registerFunctions(LuaState* L) = 0;

    /** @brief 执行可选的模块初始化。 */
    virtual void initialize(LuaState* L) {
        (void)L;
    }
};

} // namespace Lua
