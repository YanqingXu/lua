#pragma once

/**
 * @file lib_catalog.hpp
 * @brief Lua 标准库目录项及查询接口
 */

#include "common/types.hpp"

#include <functional>
#include <span>

namespace Lua {

class LuaState;

/** @brief 标准库打开函数签名。 */
using LibOpenFunction = void (*)(LuaState*);

/** @brief 标准库目录中的标识、名称与打开入口。 */
struct LibCatalogEntry {
    StrView id;
    StrView name;
    LibOpenFunction open;
};

/** @brief 获取完整的标准库目录只读视图。 */
std::span<const LibCatalogEntry> getStandardLibraryCatalog();

/** @brief 按标识查找标准库目录项。 */
Opt<std::reference_wrapper<const LibCatalogEntry>> findStandardLibrary(StrView id);

} // namespace Lua
