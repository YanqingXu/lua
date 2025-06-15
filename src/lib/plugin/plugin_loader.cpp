#include "plugin_loader.hpp"
#include "plugin_interface.hpp"
#include "../../common/defines.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <dlfcn.h>
    #include <link.h>
#endif

namespace Lua {

// === PluginLoader Implementation ===

PluginLoader::PluginLoader() {
    // 初始化统计信息
    loadStats_["total_loads"] = 0;
    loadStats_["successful_loads"] = 0;
    loadStats_["failed_loads"] = 0;
    loadStats_["cache_hits"] = 0;
    loadStats_["cache_misses"] = 0;
}

PluginLoader::~PluginLoader() {
    unloadAllPlugins();
}

// === 插件发现 ===

Vec<PluginFileInfo> PluginLoader::scanDirectory(StrView directory) const {
    Vec<PluginFileInfo> plugins;
    
    try {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            logError("Directory does not exist or is not a directory: " + Str(directory));
            return plugins;
        }
        
        // 使用普通迭代器避免递归导致的无限循环
        std::filesystem::directory_iterator iter(directory);
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
            if (ec) {
                logError("Error iterating directory: " + ec.message());
                break;
            }
            if (entry.is_regular_file()) {
                Str filePath = entry.path().string();
                if (isValidPluginFile(filePath)) {
                    PluginLoadType loadType = getLoadTypeFromExtension(filePath);
                    PluginFileInfo info(filePath, loadType);
                    
                    // 填充文件信息
                    info.fileSize = getFileSize(filePath);
                    info.lastModified = getFileModificationTime(filePath);
                    info.checksum = calculateChecksum(filePath);
                    
                    plugins.push_back(std::move(info));
                }
            }
        }
    } catch (const std::exception& e) {
        logError("Error scanning directory: " + Str(e.what()));
    }
    
    return plugins;
}

Vec<PluginFileInfo> PluginLoader::scanDirectories(const Vec<Str>& directories) const {
    Vec<PluginFileInfo> allPlugins;
    
    for (const auto& dir : directories) {
        auto plugins = scanDirectory(dir);
        allPlugins.insert(allPlugins.end(), plugins.begin(), plugins.end());
    }
    
    return allPlugins;
}

bool PluginLoader::isValidPluginFile(StrView filePath) const {
    PluginLoadType loadType = getLoadTypeFromExtension(filePath);
    
    switch (loadType) {
        case PluginLoadType::Dynamic:
            return true; // .dll, .so, .dylib files
        case PluginLoadType::Script:
            return true; // .lua files
        default:
            return false;
    }
}

Opt<PluginFileInfo> PluginLoader::getPluginFileInfo(StrView filePath) const {
    if (!std::filesystem::exists(filePath)) {
        return std::nullopt;
    }
    
    PluginLoadType loadType = getLoadTypeFromExtension(filePath);
    PluginFileInfo info(filePath, loadType);
    
    info.fileSize = getFileSize(filePath);
    info.lastModified = getFileModificationTime(filePath);
    info.checksum = calculateChecksum(filePath);
    
    return info;
}

bool PluginLoader::verifyPluginFile(const PluginFileInfo& fileInfo) const {
    // 检查文件是否存在
    if (!std::filesystem::exists(fileInfo.filePath)) {
        return false;
    }
    
    // 检查文件大小
    u64 currentSize = getFileSize(fileInfo.filePath);
    if (currentSize != fileInfo.fileSize) {
        return false;
    }
    
    // 检查修改时间
    u64 currentModTime = getFileModificationTime(fileInfo.filePath);
    if (currentModTime != fileInfo.lastModified) {
        return false;
    }
    
    // 检查校验和
    Str currentChecksum = calculateChecksum(fileInfo.filePath);
    if (currentChecksum != fileInfo.checksum) {
        return false;
    }
    
    return true;
}

// === 插件加载 ===

PluginLoadResult PluginLoader::loadFromFile(StrView filePath) {
    recordLoadStat("total_loads");
    
    // 安全检查
    if (securityCheckEnabled_) {
        if (!isPathTrusted(filePath)) {
            setError("Plugin path is not trusted: " + Str(filePath));
            recordLoadStat("failed_loads");
            return PluginLoadResult(false, lastError_);
        }
        
        if (!verifyPluginSignature(filePath)) {
            setError("Plugin signature verification failed: " + Str(filePath));
            recordLoadStat("failed_loads");
            return PluginLoadResult(false, lastError_);
        }
    }
    
    PluginLoadType loadType = getLoadTypeFromExtension(filePath);
    
    switch (loadType) {
        case PluginLoadType::Dynamic: {
            LibraryHandle handle = loadLibrary(filePath);
            if (!handle) {
                recordLoadStat("failed_loads");
                return PluginLoadResult(false, lastError_);
            }
            
            auto result = createPluginFromLibrary(handle, filePath);
            if (result.success) {
                recordLoadStat("successful_loads");
            } else {
                recordLoadStat("failed_loads");
                unloadLibrary(handle);
            }
            return result;
        }
        
        case PluginLoadType::Script:
            return loadScript(filePath);
            
        default:
            setError("Unsupported plugin type: " + Str(filePath));
            recordLoadStat("failed_loads");
            return PluginLoadResult(false, lastError_);
    }
}

PluginLoadResult PluginLoader::loadFromMemory(StrView pluginName, IPluginFactory* factory) {
    recordLoadStat("total_loads");
    
    if (!factory) {
        setError("Invalid plugin factory");
        recordLoadStat("failed_loads");
        return PluginLoadResult(false, lastError_);
    }
    
    try {
        auto plugin = factory->createPlugin();
        if (!plugin) {
            setError("Failed to create plugin from factory");
            recordLoadStat("failed_loads");
            return PluginLoadResult(false, lastError_);
        }
        
        if (!validatePluginInterface(plugin.get())) {
            setError("Plugin interface validation failed");
            recordLoadStat("failed_loads");
            return PluginLoadResult(false, lastError_);
        }
        
        PluginLoadResult result;
        result.success = true;
        result.plugin = std::move(plugin);
        result.metadata = result.plugin->getMetadata();
        
        // 缓存静态工厂
        staticFactories_[Str(pluginName)] = factory;
        
        recordLoadStat("successful_loads");
        return result;
        
    } catch (const std::exception& e) {
        setError("Exception during plugin creation: " + Str(e.what()));
        recordLoadStat("failed_loads");
        return PluginLoadResult(false, lastError_);
    }
}

PluginLoadResult PluginLoader::loadScript(StrView scriptPath) {
    // 脚本插件加载的简单实现
    // 实际项目中需要集成Lua解释器
    setError("Script plugin loading not implemented yet");
    return PluginLoadResult(false, lastError_);
}

Opt<PluginMetadata> PluginLoader::preloadMetadata(StrView filePath) {
    // 检查缓存
    if (cacheEnabled_) {
        auto it = metadataCache_.find(Str(filePath));
        if (it != metadataCache_.end()) {
            recordLoadStat("cache_hits");
            return it->second;
        }
        recordLoadStat("cache_misses");
    }
    
    PluginLoadType loadType = getLoadTypeFromExtension(filePath);
    
    if (loadType == PluginLoadType::Dynamic) {
        LibraryHandle handle = loadLibrary(filePath);
        if (!handle) {
            return std::nullopt;
        }
        
        // 尝试获取元数据函数
        auto getMetadataFunc = getPluginFunction<PluginMetadata(*)()>(handle, "getPluginMetadata");
        if (getMetadataFunc) {
            PluginMetadata metadata = getMetadataFunc();
            
            // 缓存元数据
            if (cacheEnabled_) {
                metadataCache_[Str(filePath)] = metadata;
            }
            
            unloadLibrary(handle);
            return metadata;
        }
        
        unloadLibrary(handle);
    }
    
    return std::nullopt;
}

// === 插件卸载 ===

bool PluginLoader::unloadPlugin(StrView pluginName) {
    auto it = loadedLibraries_.find(Str(pluginName));
    if (it != loadedLibraries_.end()) {
        unloadLibrary(it->second);
        loadedLibraries_.erase(it);
        return true;
    }
    
    // 检查静态插件
    auto staticIt = staticFactories_.find(Str(pluginName));
    if (staticIt != staticFactories_.end()) {
        staticFactories_.erase(staticIt);
        return true;
    }
    
    return false;
}

void PluginLoader::unloadAllPlugins() {
    // 卸载所有动态库
    for (auto& [name, handle] : loadedLibraries_) {
        unloadLibrary(handle);
    }
    loadedLibraries_.clear();
    
    // 清除静态工厂
    staticFactories_.clear();
    
    // 清除缓存
    if (cacheEnabled_) {
        metadataCache_.clear();
    }
}

// === 符号解析 ===

bool PluginLoader::hasSymbol(LibraryHandle handle, StrView symbolName) const {
    if (!handle) return false;
    
#ifdef _WIN32
    return GetProcAddress(handle, symbolName.data()) != nullptr;
#else
    return dlsym(handle, symbolName.data()) != nullptr;
#endif
}

Vec<Str> PluginLoader::getExportedSymbols(LibraryHandle handle) const {
    if (!handle) return {};
    
#ifdef _WIN32
    return enumerateWindowsSymbols(handle);
#else
    return enumerateUnixSymbols(handle);
#endif
}

// === 依赖检查 ===

bool PluginLoader::checkDependencies(const PluginMetadata& metadata) const {
    for (const auto& dep : metadata.dependencies) {
        // 检查依赖是否已加载或可用
        if (loadedLibraries_.find(dep.name) == loadedLibraries_.end() &&
            staticFactories_.find(dep.name) == staticFactories_.end()) {
            return false;
        }
    }
    return true;
}

Vec<Str> PluginLoader::resolveDependencyLibraries(const PluginMetadata& metadata) const {
    Vec<Str> libraries;
    
    for (const auto& dep : metadata.dependencies) {
        // 简单实现：假设依赖库名就是文件名
        libraries.push_back(dep.name);
    }
    
    return libraries;
}

bool PluginLoader::checkABICompatibility(const PluginMetadata& metadata) const {
    // 检查版本兼容性
    if (metadata.version.major > 1) {
        return false; // 主版本号不兼容
    }
    
    // 检查API版本
    if (metadata.apiVersion.major != 1) {
        return false; // API主版本必须匹配
    }
    
    return true;
}

// === 错误处理 ===

Str PluginLoader::getSystemError() const {
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0) return "";
    
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr);
    
    Str message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
#else
    const char* error = dlerror();
    return error ? Str(error) : "";
#endif
}

// === 缓存管理 ===

void PluginLoader::clearMetadataCache() {
    metadataCache_.clear();
}

HashMap<Str, size_t> PluginLoader::getCacheStats() const {
    HashMap<Str, size_t> stats;
    stats["cache_size"] = metadataCache_.size();
    stats["cache_hits"] = loadStats_.count("cache_hits") ? loadStats_.at("cache_hits") : 0;
    stats["cache_misses"] = loadStats_.count("cache_misses") ? loadStats_.at("cache_misses") : 0;
    return stats;
}

// === 安全检查 ===

bool PluginLoader::verifyPluginSignature(StrView filePath) const {
    // 简单实现：总是返回true
    // 实际项目中需要实现数字签名验证
    return true;
}

bool PluginLoader::verifyPluginSource(StrView filePath) const {
    // 简单实现：检查文件是否在可信路径中
    return isPathTrusted(filePath);
}

void PluginLoader::addTrustedPath(StrView path) {
    trustedPaths_.push_back(Str(path));
}

void PluginLoader::removeTrustedPath(StrView path) {
    auto it = std::find(trustedPaths_.begin(), trustedPaths_.end(), Str(path));
    if (it != trustedPaths_.end()) {
        trustedPaths_.erase(it);
    }
}

// === 调试和诊断 ===

HashMap<Str, size_t> PluginLoader::getLoadStats() const {
    return loadStats_;
}

Str PluginLoader::exportDiagnostics() const {
    std::ostringstream oss;
    
    oss << "=== Plugin Loader Diagnostics ===\n";
    oss << "Loaded Libraries: " << loadedLibraries_.size() << "\n";
    oss << "Static Factories: " << staticFactories_.size() << "\n";
    oss << "Metadata Cache: " << metadataCache_.size() << "\n";
    oss << "Trusted Paths: " << trustedPaths_.size() << "\n";
    
    oss << "\nLoad Statistics:\n";
    for (const auto& [key, value] : loadStats_) {
        oss << "  " << key << ": " << value << "\n";
    }
    
    oss << "\nLast Error: " << lastError_ << "\n";
    
    return oss.str();
}

// === 私有方法实现 ===

LibraryHandle PluginLoader::loadLibrary(StrView filePath) {
#ifdef _WIN32
    LibraryHandle handle = LoadLibraryA(filePath.data());
    if (!handle) {
        setError("Failed to load library: " + Str(filePath) + " - " + getSystemError());
    }
    return handle;
#else
    LibraryHandle handle = dlopen(filePath.data(), RTLD_LAZY);
    if (!handle) {
        setError("Failed to load library: " + Str(filePath) + " - " + getSystemError());
    }
    return handle;
#endif
}

void PluginLoader::unloadLibrary(LibraryHandle handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

PluginLoadResult PluginLoader::createPluginFromLibrary(LibraryHandle handle, StrView filePath) {
    // 查找插件工厂函数
    auto createFunc = getPluginFunction<IPlugin*(*)()>(handle, "createPlugin");
    if (!createFunc) {
        setError("Plugin does not export createPlugin function: " + Str(filePath));
        return PluginLoadResult(false, lastError_);
    }
    
    try {
        IPlugin* rawPlugin = createFunc();
        if (!rawPlugin) {
            setError("createPlugin returned null: " + Str(filePath));
            return PluginLoadResult(false, lastError_);
        }
        
        UPtr<IPlugin> plugin(rawPlugin);
        
        if (!validatePluginInterface(plugin.get())) {
            setError("Plugin interface validation failed: " + Str(filePath));
            return PluginLoadResult(false, lastError_);
        }
        
        PluginLoadResult result;
        result.success = true;
        result.plugin = std::move(plugin);
        result.metadata = result.plugin->getMetadata();
        result.libraryHandle = handle;
        
        // 记录已加载的库
        loadedLibraries_[result.metadata.name] = handle;
        
        return result;
        
    } catch (const std::exception& e) {
        setError("Exception during plugin creation: " + Str(e.what()));
        return PluginLoadResult(false, lastError_);
    }
}

bool PluginLoader::validatePluginInterface(IPlugin* plugin) const {
    if (!plugin) return false;
    
    try {
        // 基本接口检查
        auto metadata = plugin->getMetadata();
        if (metadata.name.empty()) {
            return false;
        }
        
        // 检查ABI兼容性
        if (!checkABICompatibility(metadata)) {
            return false;
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

PluginLoadType PluginLoader::getLoadTypeFromExtension(StrView filePath) const {
    std::filesystem::path path(filePath);
    Str extension = path.extension().string();
    
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".dll" || extension == ".so" || extension == ".dylib") {
        return PluginLoadType::Dynamic;
    } else if (extension == ".lua") {
        return PluginLoadType::Script;
    }
    
    return PluginLoadType::Static; // 默认
}

Str PluginLoader::calculateChecksum(StrView filePath) const {
    std::ifstream file(filePath.data(), std::ios::binary);
    if (!file) return "";
    
    // 简单的校验和计算（实际项目中应使用更强的哈希算法）
    std::ostringstream checksum;
    char buffer[4096];
    size_t hash = 0;
    
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        for (std::streamsize i = 0; i < file.gcount(); ++i) {
            hash = hash * 31 + static_cast<unsigned char>(buffer[i]);
        }
    }
    
    checksum << std::hex << hash;
    return checksum.str();
}

u64 PluginLoader::getFileModificationTime(StrView filePath) const {
    try {
        auto ftime = std::filesystem::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
    } catch (...) {
        return 0;
    }
}

u64 PluginLoader::getFileSize(StrView filePath) const {
    try {
        return std::filesystem::file_size(filePath);
    } catch (...) {
        return 0;
    }
}

bool PluginLoader::isPathTrusted(StrView filePath) const {
    if (trustedPaths_.empty()) {
        return true; // 如果没有设置可信路径，则信任所有路径
    }
    
    std::filesystem::path path(filePath);
    path = std::filesystem::absolute(path);
    
    for (const auto& trustedPath : trustedPaths_) {
        std::filesystem::path trusted(trustedPath);
        trusted = std::filesystem::absolute(trusted);
        
        // 检查文件是否在可信路径下
        auto rel = std::filesystem::relative(path, trusted);
        if (!rel.empty() && rel.native()[0] != '.') {
            return true;
        }
    }
    
    return false;
}

void PluginLoader::setError(StrView error) {
    lastError_ = error;
    logError(error);
}

void PluginLoader::recordLoadStat(StrView operation) {
    loadStats_[Str(operation)]++;
}

void PluginLoader::logVerbose(StrView message) const {
    if (verboseLogging_) {
        // 实际项目中应该使用日志系统
        // std::cout << "[VERBOSE] " << message << std::endl;
    }
}

void PluginLoader::logError(StrView message) const {
    // 实际项目中应该使用日志系统
    // std::cerr << "[ERROR] " << message << std::endl;
}

// === 平台特定实现 ===

#ifdef _WIN32
Vec<Str> PluginLoader::enumerateWindowsSymbols(HMODULE handle) const {
    Vec<Str> symbols;
    // Windows符号枚举的简单实现
    // 实际项目中需要解析PE格式
    return symbols;
}
#else
Vec<Str> PluginLoader::enumerateUnixSymbols(void* handle) const {
    Vec<Str> symbols;
    // Unix符号枚举的简单实现
    // 实际项目中需要解析ELF格式
    return symbols;
}
#endif

// === StaticPluginRegistry Implementation ===

HashMap<Str, IPluginFactory*>& StaticPluginRegistry::getFactories() {
    static HashMap<Str, IPluginFactory*> factories;
    return factories;
}

void StaticPluginRegistry::registerFactory(StrView name, IPluginFactory* factory) {
    getFactories()[Str(name)] = factory;
}

IPluginFactory* StaticPluginRegistry::getFactory(StrView name) {
    auto& factories = getFactories();
    auto it = factories.find(Str(name));
    return (it != factories.end()) ? it->second : nullptr;
}

Vec<Str> StaticPluginRegistry::getStaticPluginNames() {
    Vec<Str> names;
    auto& factories = getFactories();
    
    for (const auto& [name, factory] : factories) {
        names.push_back(name);
    }
    
    return names;
}

void StaticPluginRegistry::clear() {
    getFactories().clear();
}

} // namespace Lua