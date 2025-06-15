#pragma once

#include "plugin_interface.hpp"
#include "../../common/types.hpp"
#include <memory>
#include <functional>

#ifdef _WIN32
    #include <windows.h>
    using LibraryHandle = HMODULE;
#else
    #include <dlfcn.h>
    using LibraryHandle = void*;
#endif

namespace Lua {
    /**
     * 插件加载类型
     */
    enum class PluginLoadType {
        Dynamic,    // 动态库加载
        Static,     // 静态链接
        Script      // 脚本插件
    };
    
    /**
     * 插件文件信息
     */
    struct PluginFileInfo {
        Str filePath;                    // 文件路径
        Str fileName;                    // 文件名
        PluginLoadType loadType;         // 加载类型
        u64 fileSize = 0;               // 文件大小
        u64 lastModified = 0;           // 最后修改时间
        Str checksum;                   // 文件校验和
        
        PluginFileInfo() = default;
        PluginFileInfo(StrView path, PluginLoadType type) 
            : filePath(path), loadType(type) {
            // 从路径提取文件名
            auto pos = filePath.find_last_of("/\\");
            fileName = (pos != Str::npos) ? filePath.substr(pos + 1) : filePath;
        }
    };
    
    /**
     * 加载结果
     */
    struct PluginLoadResult {
        bool success = false;
        UPtr<IPlugin> plugin;
        PluginMetadata metadata;
        Str errorMessage;
        LibraryHandle libraryHandle = nullptr;
        
        PluginLoadResult() = default;
        PluginLoadResult(bool ok, Str error = "") : success(ok), errorMessage(std::move(error)) {}
    };
    
    /**
     * 插件加载器 - 负责插件的物理加载和符号解析
     */
    class PluginLoader {
    public:
        PluginLoader();
        ~PluginLoader();
        
        // 禁用拷贝和移动
        PluginLoader(const PluginLoader&) = delete;
        PluginLoader& operator=(const PluginLoader&) = delete;
        PluginLoader(PluginLoader&&) = delete;
        PluginLoader& operator=(PluginLoader&&) = delete;
        
        // === 插件发现 ===
        
        /**
         * 扫描目录中的插件文件
         */
        Vec<PluginFileInfo> scanDirectory(StrView directory) const;
        
        /**
         * 扫描多个目录
         */
        Vec<PluginFileInfo> scanDirectories(const Vec<Str>& directories) const;
        
        /**
         * 检查文件是否为有效的插件
         */
        bool isValidPluginFile(StrView filePath) const;
        
        /**
         * 获取插件文件信息
         */
        Opt<PluginFileInfo> getPluginFileInfo(StrView filePath) const;
        
        /**
         * 检查插件文件完整性
         */
        bool verifyPluginFile(const PluginFileInfo& fileInfo) const;
        
        // === 插件加载 ===
        
        /**
         * 从文件加载插件
         */
        PluginLoadResult loadFromFile(StrView filePath);
        
        /**
         * 从内存加载插件（预编译的静态插件）
         */
        PluginLoadResult loadFromMemory(StrView pluginName, IPluginFactory* factory);
        
        /**
         * 加载脚本插件
         */
        PluginLoadResult loadScript(StrView scriptPath);
        
        /**
         * 预加载插件（仅加载元数据，不实例化）
         */
        Opt<PluginMetadata> preloadMetadata(StrView filePath);
        
        // === 插件卸载 ===
        
        /**
         * 卸载插件
         */
        bool unloadPlugin(StrView pluginName);
        
        /**
         * 卸载所有插件
         */
        void unloadAllPlugins();
        
        // === 符号解析 ===
        
        /**
         * 获取插件导出函数
         */
        template<typename T>
        T getPluginFunction(LibraryHandle handle, StrView functionName) const {
            if (!handle) return nullptr;
            
#ifdef _WIN32
            return reinterpret_cast<T>(GetProcAddress(handle, functionName.data()));
#else
            return reinterpret_cast<T>(dlsym(handle, functionName.data()));
#endif
        }
        
        /**
         * 检查符号是否存在
         */
        bool hasSymbol(LibraryHandle handle, StrView symbolName) const;
        
        /**
         * 获取所有导出符号
         */
        Vec<Str> getExportedSymbols(LibraryHandle handle) const;
        
        // === 依赖检查 ===
        
        /**
         * 检查插件依赖
         */
        bool checkDependencies(const PluginMetadata& metadata) const;
        
        /**
         * 解析依赖库
         */
        Vec<Str> resolveDependencyLibraries(const PluginMetadata& metadata) const;
        
        /**
         * 检查ABI兼容性
         */
        bool checkABICompatibility(const PluginMetadata& metadata) const;
        
        // === 错误处理 ===
        
        /**
         * 获取最后的加载错误
         */
        StrView getLastError() const { return lastError_; }
        
        /**
         * 清除错误信息
         */
        void clearError() { lastError_.clear(); }
        
        /**
         * 获取系统错误信息
         */
        Str getSystemError() const;
        
        // === 缓存管理 ===
        
        /**
         * 启用元数据缓存
         */
        void enableMetadataCache(bool enable = true) { cacheEnabled_ = enable; }
        
        /**
         * 清除元数据缓存
         */
        void clearMetadataCache();
        
        /**
         * 获取缓存统计
         */
        HashMap<Str, size_t> getCacheStats() const;
        
        // === 安全检查 ===
        
        /**
         * 启用安全检查
         */
        void enableSecurityCheck(bool enable = true) { securityCheckEnabled_ = enable; }
        
        /**
         * 检查插件签名
         */
        bool verifyPluginSignature(StrView filePath) const;
        
        /**
         * 检查插件来源
         */
        bool verifyPluginSource(StrView filePath) const;
        
        /**
         * 添加可信路径
         */
        void addTrustedPath(StrView path);
        
        /**
         * 移除可信路径
         */
        void removeTrustedPath(StrView path);
        
        // === 调试和诊断 ===
        
        /**
         * 启用详细日志
         */
        void enableVerboseLogging(bool enable = true) { verboseLogging_ = enable; }
        
        /**
         * 获取加载统计
         */
        HashMap<Str, size_t> getLoadStats() const;
        
        /**
         * 导出诊断信息
         */
        Str exportDiagnostics() const;
        
    private:
        // 已加载的库句柄
        HashMap<Str, LibraryHandle> loadedLibraries_;
        
        // 插件工厂缓存
        HashMap<Str, IPluginFactory*> staticFactories_;
        
        // 元数据缓存
        HashMap<Str, PluginMetadata> metadataCache_;
        bool cacheEnabled_ = true;
        
        // 安全设置
        bool securityCheckEnabled_ = true;
        Vec<Str> trustedPaths_;
        
        // 错误信息
        Str lastError_;
        
        // 统计信息
        HashMap<Str, size_t> loadStats_;
        bool verboseLogging_ = false;
        
        // 私有方法
        
        /**
         * 加载动态库
         */
        LibraryHandle loadLibrary(StrView filePath);
        
        /**
         * 卸载动态库
         */
        void unloadLibrary(LibraryHandle handle);
        
        /**
         * 从动态库创建插件
         */
        PluginLoadResult createPluginFromLibrary(LibraryHandle handle, StrView filePath);
        
        /**
         * 验证插件接口
         */
        bool validatePluginInterface(IPlugin* plugin) const;
        
        /**
         * 检查文件扩展名
         */
        PluginLoadType getLoadTypeFromExtension(StrView filePath) const;
        
        /**
         * 计算文件校验和
         */
        Str calculateChecksum(StrView filePath) const;
        
        /**
         * 获取文件修改时间
         */
        u64 getFileModificationTime(StrView filePath) const;
        
        /**
         * 获取文件大小
         */
        u64 getFileSize(StrView filePath) const;
        
        /**
         * 检查路径是否可信
         */
        bool isPathTrusted(StrView filePath) const;
        
        /**
         * 设置错误信息
         */
        void setError(StrView error);
        
        /**
         * 记录加载统计
         */
        void recordLoadStat(StrView operation);
        
        /**
         * 日志记录
         */
        void logVerbose(StrView message) const;
        void logError(StrView message) const;
        
        // 平台特定的实现
        
#ifdef _WIN32
        /**
         * Windows特定的符号枚举
         */
        Vec<Str> enumerateWindowsSymbols(HMODULE handle) const;
#else
        /**
         * Unix特定的符号枚举
         */
        Vec<Str> enumerateUnixSymbols(void* handle) const;
#endif
    };
    
    /**
     * 插件加载器工厂
     */
    class PluginLoaderFactory {
    public:
        static UPtr<PluginLoader> create() {
            return std::make_unique<PluginLoader>();
        }
    };
    
    /**
     * 静态插件注册辅助类
     */
    class StaticPluginRegistry {
    public:
        /**
         * 注册静态插件工厂
         */
        static void registerFactory(StrView name, IPluginFactory* factory);
        
        /**
         * 获取静态插件工厂
         */
        static IPluginFactory* getFactory(StrView name);
        
        /**
         * 获取所有静态插件名称
         */
        static Vec<Str> getStaticPluginNames();
        
        /**
         * 清除所有注册
         */
        static void clear();
        
    private:
        static HashMap<Str, IPluginFactory*>& getFactories();
    };
}

// 静态插件注册宏
#define LUA_REGISTER_STATIC_PLUGIN(name, factory) \
    namespace { \
        struct StaticPluginRegistrar_##name { \
            StaticPluginRegistrar_##name() { \
                ::Lua::StaticPluginRegistry::registerFactory(#name, factory); \
            } \
        }; \
        static StaticPluginRegistrar_##name registrar_##name; \
    }
