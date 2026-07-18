#pragma once

/**
 * @file bytecode_verifier.hpp
 * @brief Central verifier for Proto trees before they become executable.
 */

#include "common/types.hpp"

#include <expected>

namespace Lua {

class Proto;

struct BytecodeVerifierLimits {
    usize maxProtoDepth = 200;
    usize maxProtoCount = 10'000;
    usize maxInstructionCount = 1'000'000;
    usize maxConstantCount = 1'000'000;
    usize maxDebugEntries = 1'000'000;
};

class BytecodeVerifier {
public:
    [[nodiscard]] static std::expected<void, Str>
    verify(const Proto& root, const BytecodeVerifierLimits& limits = {});
};

} // namespace Lua
