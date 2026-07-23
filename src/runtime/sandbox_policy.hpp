#pragma once

/**
 * @file sandbox_policy.hpp
 * @brief 上下文拥有的标准库与特权操作策略
 */

#include "common/types.hpp"

#include <type_traits>

namespace Lua {

/**
 * @brief Lua 标准库可访问的特权宿主资源
 */
enum class SandboxCapability : u8 {
    Filesystem,
    Process,
    NativeModules,
    RuntimeCompilation,
    BinaryChunks,
    GCControl,
};

/**
 * @brief SandboxProfile 使用的标准库暴露位
 */
enum class StandardLibrarySet : u16 {
    None = 0,
    Base = 1U << 0U,
    Math = 1U << 1U,
    IO = 1U << 2U,
    String = 1U << 3U,
    Table = 1U << 4U,
    OS = 1U << 5U,
    Coroutine = 1U << 6U,
    Debug = 1U << 7U,
    Package = 1U << 8U,
    All = (1U << 9U) - 1U,
};

[[nodiscard]] constexpr StandardLibrarySet operator|(StandardLibrarySet lhs, StandardLibrarySet rhs) noexcept {
    using Underlying = std::underlying_type_t<StandardLibrarySet>;
    return static_cast<StandardLibrarySet>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

[[nodiscard]] constexpr bool contains(StandardLibrarySet set, StandardLibrarySet member) noexcept {
    using Underlying = std::underlying_type_t<StandardLibrarySet>;
    return (static_cast<Underlying>(set) & static_cast<Underlying>(member)) != 0;
}

/**
 * @brief 配置到单个上下文 SandboxPolicy 中的值
 */
struct SandboxProfile {
    StandardLibrarySet standardLibraries = StandardLibrarySet::All;
    bool filesystem = true;
    bool process = true;
    bool nativeModules = true;
    bool runtimeCompilation = true;
    bool binaryChunks = true;
    bool gcControl = true;

    /**
     * @brief 与 Lua 5.1 兼容的无限制行为。
     */
    [[nodiscard]] static constexpr SandboxProfile unrestricted() noexcept {
        return {};
    }

    /**
     * @brief 仅允许预加载 Lua 模块的保守服务器配置
     */
    [[nodiscard]] static constexpr SandboxProfile gameServer() noexcept {
        return {
            StandardLibrarySet::Base | StandardLibrarySet::Math | StandardLibrarySet::String |
                StandardLibrarySet::Table | StandardLibrarySet::Coroutine | StandardLibrarySet::Package,
            false,
            false,
            false,
            false,
            false,
            false,
        };
    }
};

/**
 * @brief 单个上下文内所有 LuaState 共享的所有者线程配置
 *
 * 打开标准库时评估库暴露权限。每次操作也会检查特权能力，避免已捕获的函数绕过后续限制。
 * 若被禁用的全局项绝不能发布，应在打开标准库之前配置此方案。
 */
class SandboxPolicy {
public:
    SandboxPolicy() noexcept = default;

    void configure(const SandboxProfile& profile) noexcept {
        profile_ = profile;
    }

    void reset() noexcept {
        profile_ = SandboxProfile::unrestricted();
    }

    [[nodiscard]] const SandboxProfile& profile() const noexcept {
        return profile_;
    }

    [[nodiscard]] bool allowsStandardLibrary(StrView id) const noexcept {
        return contains(profile_.standardLibraries, librarySetForId(id));
    }

    [[nodiscard]] bool allows(SandboxCapability capability) const noexcept {
        switch (capability) {
        case SandboxCapability::Filesystem:
            return profile_.filesystem;
        case SandboxCapability::Process:
            return profile_.process;
        case SandboxCapability::NativeModules:
            return profile_.nativeModules;
        case SandboxCapability::RuntimeCompilation:
            return profile_.runtimeCompilation;
        case SandboxCapability::BinaryChunks:
            return profile_.binaryChunks;
        case SandboxCapability::GCControl:
            return profile_.gcControl;
        }
        return false;
    }

    [[nodiscard]] static constexpr const char* deniedMessage(SandboxCapability capability) noexcept {
        switch (capability) {
        case SandboxCapability::Filesystem:
            return "sandbox: filesystem access denied";
        case SandboxCapability::Process:
            return "sandbox: process access denied";
        case SandboxCapability::NativeModules:
            return "sandbox: native module access denied";
        case SandboxCapability::RuntimeCompilation:
            return "sandbox: runtime compilation denied";
        case SandboxCapability::BinaryChunks:
            return "sandbox: binary chunk loading denied";
        case SandboxCapability::GCControl:
            return "sandbox: GC control denied";
        }
        return "sandbox: access denied";
    }

    [[nodiscard]] static constexpr const char* libraryDeniedMessage() noexcept {
        return "sandbox: standard library disabled";
    }

private:
    [[nodiscard]] static constexpr StandardLibrarySet librarySetForId(StrView id) noexcept {
        if (id == "base") {
            return StandardLibrarySet::Base;
        }
        if (id == "math") {
            return StandardLibrarySet::Math;
        }
        if (id == "io") {
            return StandardLibrarySet::IO;
        }
        if (id == "string") {
            return StandardLibrarySet::String;
        }
        if (id == "table") {
            return StandardLibrarySet::Table;
        }
        if (id == "os") {
            return StandardLibrarySet::OS;
        }
        if (id == "coroutine") {
            return StandardLibrarySet::Coroutine;
        }
        if (id == "debug") {
            return StandardLibrarySet::Debug;
        }
        if (id == "package") {
            return StandardLibrarySet::Package;
        }
        return StandardLibrarySet::None;
    }

    SandboxProfile profile_ = SandboxProfile::unrestricted();
};

} // namespace Lua
