#pragma once

/**
 * @file file_loader.hpp
 * @brief 文件完整读取接口
 */

#include "common/types.hpp"

#include <filesystem>

namespace Lua {

/**
 * @brief 以二进制方式读取完整文件
 * @param path 文件路径
 * @return 文件的全部字节
 * @throws std::runtime_error 文件无法打开或读取时抛出
 */
Str readWholeFile(const std::filesystem::path& path);

} // namespace Lua
