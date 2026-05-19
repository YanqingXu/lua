#pragma once

#include "common/types.hpp"

#include <filesystem>

namespace Lua {

Str readWholeFile(const std::filesystem::path& path);

} // namespace Lua
