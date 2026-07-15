/**
 * @file native_module_registry.cpp
 * @brief Platform implementation of context-owned native module leases.
 */

#include "runtime/native_module_registry.hpp"
#include "runtime/sandbox_policy.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

namespace Lua {
namespace {

#ifdef _WIN32

Str lastModuleError() {
    const DWORD error = GetLastError();
    if (error == 0) {
        return "unknown dynamic library error";
    }

    LPSTR buffer = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    Str message = length != 0 && buffer != nullptr ? Str(buffer, static_cast<usize>(length))
                                                   : "Windows error " + std::to_string(error);
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }
    return message;
}

Str executablePath() {
    std::array<char, MAX_PATH> buffer{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return {};
    }
    return Str(buffer.data(), static_cast<usize>(length));
}

Str absolutePath(const Str& path) {
    const DWORD required = GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
    if (required == 0) {
        return path;
    }

    Vec<char> buffer(static_cast<usize>(required) + 1, '\0');
    const DWORD length = GetFullPathNameA(path.c_str(), required + 1, buffer.data(), nullptr);
    if (length == 0) {
        return path;
    }
    return Str(buffer.data(), static_cast<usize>(length));
}

Str platformPathKey(Str path) {
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return path;
}

#else

Str lastModuleError() {
    const char* error = dlerror();
    return error != nullptr ? Str(error) : Str("unknown dynamic library error");
}

Str executablePath() {
#ifdef __APPLE__
    uint32_t capacity = static_cast<uint32_t>(PATH_MAX);
    Vec<char> buffer(static_cast<usize>(capacity) + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &capacity) != 0) {
        buffer.assign(static_cast<usize>(capacity) + 1, '\0');
        if (_NSGetExecutablePath(buffer.data(), &capacity) != 0) {
            return {};
        }
    }
    return Str(buffer.data());
#else
    std::array<char, PATH_MAX> buffer{};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        return {};
    }
    return Str(buffer.data(), static_cast<usize>(length));
#endif
}

Str absolutePath(const Str& path) {
    std::array<char, PATH_MAX> buffer{};
    if (realpath(path.c_str(), buffer.data()) == nullptr) {
        return path;
    }
    return Str(buffer.data());
}

Str platformPathKey(Str path) {
    return path;
}

#endif

} // namespace

NativeModuleRegistry::~NativeModuleRegistry() noexcept {
    for (auto entry = entries_.rbegin(); entry != entries_.rend(); ++entry) {
        close(entry->handle, entry->owned);
    }
}

std::expected<NativeModuleRegistry::Handle, Str> NativeModuleRegistry::load(const Str& filename) {
    if (sandboxPolicy_ != nullptr && !sandboxPolicy_->allows(SandboxCapability::NativeModules)) {
        return std::unexpected(Str(SandboxPolicy::deniedMessage(SandboxCapability::NativeModules)));
    }

    if (filename.empty()) {
        return std::unexpected(Str("empty dynamic library path"));
    }

    const Str key = normalizedPath(filename);
    const auto hasPath = [&key](const Entry& entry) {
        return entry.normalizedPath == key ||
               std::find(entry.aliases.begin(), entry.aliases.end(), key) != entry.aliases.end();
    };
    const auto existing = std::find_if(entries_.begin(), entries_.end(), hasPath);
    if (existing != entries_.end()) {
        return existing->handle;
    }

    Handle handle = nullptr;
    bool owned = true;
#ifdef _WIN32
    if (isCurrentExecutable(filename)) {
        handle = reinterpret_cast<Handle>(GetModuleHandleA(nullptr));
        // GetModuleHandle does not increment the module reference count.
        owned = false;
    } else {
        // Preserve the platform loader's ordinary search semantics. The
        // normalized absolute path is only a per-context comparison key.
        handle = reinterpret_cast<Handle>(LoadLibraryA(filename.c_str()));
    }
#else
    dlerror();
    if (isCurrentExecutable(filename)) {
        handle = dlopen(nullptr, RTLD_NOW | RTLD_LOCAL);
    } else {
        handle = dlopen(filename.c_str(), RTLD_NOW | RTLD_LOCAL);
    }
#endif

    if (handle == nullptr) {
        return std::unexpected(lastModuleError());
    }

    try {
        const auto sameHandle = std::find_if(entries_.begin(), entries_.end(),
                                             [handle](const Entry& entry) { return entry.handle == handle; });
        if (sameHandle != entries_.end()) {
            sameHandle->aliases.push_back(key);
            close(handle, owned);
            return sameHandle->handle;
        }

        entries_.push_back(Entry{key, {}, handle, owned});
    } catch (...) {
        close(handle, owned);
        throw;
    }
    return handle;
}

std::expected<void*, Str> NativeModuleRegistry::findSymbol(Handle handle, const Str& symbolName) const {
    if (handle == nullptr) {
        return std::unexpected(Str("invalid dynamic library handle"));
    }
    if (symbolName.empty()) {
        return std::unexpected(Str("empty dynamic library symbol"));
    }

#ifdef _WIN32
    FARPROC symbol = GetProcAddress(reinterpret_cast<HMODULE>(handle), symbolName.c_str());
    if (symbol == nullptr) {
        return std::unexpected(lastModuleError());
    }
    return reinterpret_cast<void*>(symbol);
#else
    dlerror();
    void* symbol = dlsym(handle, symbolName.c_str());
    const char* error = dlerror();
    if (error != nullptr) {
        return std::unexpected(Str(error));
    }
    return symbol;
#endif
}

bool NativeModuleRegistry::contains(const Str& filename) const {
    if (filename.empty()) {
        return false;
    }
    const Str key = normalizedPath(filename);
    return std::any_of(entries_.begin(), entries_.end(), [&key](const Entry& entry) {
        return entry.normalizedPath == key ||
               std::find(entry.aliases.begin(), entry.aliases.end(), key) != entry.aliases.end();
    });
}

Str NativeModuleRegistry::normalizedPath(const Str& filename) {
    return platformPathKey(absolutePath(filename));
}

bool NativeModuleRegistry::isCurrentExecutable(const Str& filename) {
    const Str current = executablePath();
    return !current.empty() && normalizedPath(filename) == normalizedPath(current);
}

void NativeModuleRegistry::close(Handle handle, bool owned) noexcept {
    if (!owned || handle == nullptr) {
        return;
    }
#ifdef _WIN32
    (void)FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
    (void)dlclose(handle);
#endif
}

} // namespace Lua
