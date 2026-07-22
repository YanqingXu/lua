#pragma once

/**
 * @file bytecode_verifier.hpp
 * @brief 函数原型树进入可执行状态前使用的集中验证器
 */

#include "common/types.hpp"

#include <expected>

namespace Lua {

class Proto;

/** @brief 字节码验证过程的资源限制。 */
struct BytecodeVerifierLimits {
    usize maxProtoDepth = 200;
    usize maxProtoCount = 10'000;
    usize maxInstructionCount = 1'000'000;
    usize maxConstantCount = 1'000'000;
    usize maxDebugEntries = 1'000'000;
};

/** @brief 在执行前验证函数原型及其字节码不变量。 */
class BytecodeVerifier {
public:
    [[nodiscard]] static std::expected<void, Str>
    verify(const Proto& root, const BytecodeVerifierLimits& limits = {});
};

} // namespace Lua
