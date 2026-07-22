/**
 * @file file_loader.cpp
 * @brief 文件完整读取与错误报告的实现
 */

#include "file_loader.hpp"

#include <fstream>
#include <stdexcept>

namespace Lua {

Str readWholeFile(const std::filesystem::path& path) {
    const Str pathText = path.string();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + pathText);
    }

    std::streamsize size = file.tellg();
    if (size < 0) {
        throw std::runtime_error("error reading file: " + pathText);
    }

    Str content(static_cast<usize>(size), '\0');
    file.seekg(0, std::ios::beg);
    if (!file) {
        throw std::runtime_error("error reading file: " + pathText);
    }

    if (!content.empty()) {
        file.read(content.data(), size);
        if (!file) {
            throw std::runtime_error("error reading file: " + pathText);
        }
    }

    return content;
}

} // namespace Lua
