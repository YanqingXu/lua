#pragma once

/**
 * @file lib_manager.hpp
 * @brief Lua 标准库的统一打开与兼容入口
 */

#include "lib/lib_module.hpp"

namespace Lua {

/** @brief 提供标准库目录驱动入口及旧式兼容打开函数。 */
class StandardLibrary {
public:
    /** @brief 打开当前沙箱策略允许的全部标准库。 */
    static void openAll(LuaState* L);

    /** @brief 按目录标识打开标准库。 */
    static void openCatalogLibrary(LuaState* L, StrView id);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"base\") instead.")]]
    static void openBase(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"math\") instead.")]]
    static void openMath(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"io\") instead.")]]
    static void openIO(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"string\") instead.")]]
    static void openString(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"table\") instead.")]]
    static void openTable(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"os\") instead.")]]
    static void openOS(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"coroutine\") instead.")]]
    static void openCoroutine(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"debug\") instead.")]]
    static void openDebug(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"package\") instead.")]]
    static void openPackage(LuaState* L);

    /** @brief 注册并初始化指定标准库模块。 */
    static void openModule(LuaState* L, LibModule& module);
};

} // namespace Lua
