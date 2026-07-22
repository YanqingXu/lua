#pragma once

/**
 * @file native_module_registry.hpp
 * @brief 动态加载原生模块的 EngineContext 级所有权管理
 */

#include "common/types.hpp"

#include <expected>

namespace Lua {

class SandboxPolicy;

/** @brief 原生模块注册表的加载与可见性策略。 */
struct NativeModulePolicy {
    bool requireAbsolutePath = true;
    bool requireAbiHandshake = false;
    u32 expectedAbiVersion = 1;
    Str abiVersionSymbol = "lua_cpp_module_abi_version";
    Vec<Str> allowedCanonicalPaths;
};

/**
 * @brief 持有单个运行时所用原生模块的操作系统租约
 *
 * 每个注册表对同一路径最多打开一次。若不同加载器路径写法解析为同一操作系统句柄，则释放
 * 多余引用并将该写法缓存为别名。不同 EngineContext 实例有意使用不同注册表，从而获取独立的
 * 操作系统引用。句柄仅在注册表销毁时释放；只要 Lua Function 对象仍可能包含模块代码指针，
 * 就不会提前卸载。
 */
class NativeModuleRegistry {
public:
    using Handle = void*;

    explicit NativeModuleRegistry(const SandboxPolicy* sandboxPolicy = nullptr) noexcept
        : sandboxPolicy_(sandboxPolicy) {}
    ~NativeModuleRegistry() noexcept;

    NativeModuleRegistry(const NativeModuleRegistry&) = delete;
    NativeModuleRegistry& operator=(const NativeModuleRegistry&) = delete;
    NativeModuleRegistry(NativeModuleRegistry&&) = delete;
    NativeModuleRegistry& operator=(NativeModuleRegistry&&) = delete;

    /**
     * @brief 为当前运行时获取或复用模块租约
     */
    [[nodiscard]] std::expected<Handle, Str> load(const Str& filename);

    /**
     * @brief 从已获取的模块中解析符号
     */
    [[nodiscard]] std::expected<void*, Str> findSymbol(Handle handle, const Str& symbolName) const;

    NativeModulePolicy& policy() noexcept { return policy_; }
    const NativeModulePolicy& policy() const noexcept { return policy_; }

    /**
     * @brief 向允许列表添加一条规范路径
     * @note 空允许列表放行所有绝对路径。
     */
    void allowPath(const Str& filename);

    [[nodiscard]] usize loadedCount() const noexcept {
        return entries_.size();
    }

    /**
     * @brief 为测试与诊断提供当前上下文路径缓存的只读视图
     */
    [[nodiscard]] bool contains(const Str& filename) const;

private:
    struct Entry {
        Str normalizedPath;
        Vec<Str> aliases;
        Handle handle;
        bool owned;
    };

    [[nodiscard]] static Str normalizedPath(const Str& filename);
    [[nodiscard]] static bool isCurrentExecutable(const Str& filename);
    [[nodiscard]] bool pathAllowed(const Str& normalized) const;
    [[nodiscard]] std::expected<void, Str> verifyAbi(Handle handle) const;
    static void close(Handle handle, bool owned) noexcept;

    Vec<Entry> entries_;
    const SandboxPolicy* sandboxPolicy_;
    NativeModulePolicy policy_;
};

} // namespace Lua
