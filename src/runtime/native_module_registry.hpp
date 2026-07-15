#pragma once

/**
 * @file native_module_registry.hpp
 * @brief EngineContext-scoped ownership for dynamically loaded native modules.
 */

#include "common/types.hpp"

#include <expected>

namespace Lua {

/**
 * @brief Owns the operating-system leases for native modules used by one runtime.
 *
 * A path is opened at most once per registry. If distinct loader spellings
 * resolve to the same OS handle, the extra reference is released and the
 * spelling is cached as an alias. Different EngineContext instances deliberately
 * have different registries and therefore acquire independent OS references.
 * Handles are released only when the registry is destroyed; there is no eager
 * unload while Lua Function objects may still contain module code pointers.
 */
class NativeModuleRegistry {
public:
    using Handle = void*;

    NativeModuleRegistry() = default;
    ~NativeModuleRegistry() noexcept;

    NativeModuleRegistry(const NativeModuleRegistry&) = delete;
    NativeModuleRegistry& operator=(const NativeModuleRegistry&) = delete;
    NativeModuleRegistry(NativeModuleRegistry&&) = delete;
    NativeModuleRegistry& operator=(NativeModuleRegistry&&) = delete;

    /**
     * @brief Acquire or reuse a module lease for this runtime.
     */
    [[nodiscard]] std::expected<Handle, Str> load(const Str& filename);

    /**
     * @brief Resolve a symbol from an already acquired module.
     */
    [[nodiscard]] std::expected<void*, Str> findSymbol(Handle handle, const Str& symbolName) const;

    [[nodiscard]] usize loadedCount() const noexcept {
        return entries_.size();
    }

    /**
     * @brief Test-only/diagnostic visibility into this context's path cache.
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
    static void close(Handle handle, bool owned) noexcept;

    Vec<Entry> entries_;
};

} // namespace Lua
