#pragma once

/**
 * @file testlib.hpp
 * @brief Lua 测试辅助库模块及其打开接口
 */

#include "lib/lib_module.hpp"

namespace Lua {

/** @brief 向测试环境注册辅助函数的标准库模块。 */
class TestLibModule : public LibModule {
public:
    const char* getName() const override {
        return "T";
    }

    void registerFunctions(LuaState* L) override;
    void initialize(LuaState* L) override;
};

/** @brief 打开测试辅助库。 */
void openTestLib(LuaState* L);

} // namespace Lua
