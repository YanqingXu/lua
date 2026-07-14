#pragma once

#include "common/types.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace LuaTest {

class TemporaryTestFile final {
public:
    TemporaryTestFile(const Lua::Str& path, std::string_view content) : path_(path) {
        const std::filesystem::path parent = path_.parent_path();
        if (!parent.empty()) {
            std::error_code error;
            std::filesystem::create_directories(parent, error);
            if (error) {
                throw std::runtime_error("failed to create test directory '" + parent.string() +
                                         "': " + error.message());
            }
        }

        std::ofstream file(path_, std::ios::binary);
        if (!file) {
            throw std::runtime_error("failed to create test file: " + path_.string());
        }
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.close();
        if (!file) {
            throw std::runtime_error("failed to write test file: " + path_.string());
        }
    }

    ~TemporaryTestFile() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    TemporaryTestFile(const TemporaryTestFile&) = delete;
    TemporaryTestFile& operator=(const TemporaryTestFile&) = delete;

private:
    std::filesystem::path path_;
};

} // namespace LuaTest
