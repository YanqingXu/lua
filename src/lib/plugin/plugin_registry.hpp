#pragma once

#include "plugin_interface.hpp"
#include "../../common/types.hpp"
#include <memory>
#include <shared_mutex>
#include <chrono>

namespace Lua {
    /**
     * 插件注册信息
     */
    struct PluginRegistration {
        PluginMetadata metadata;                    // 插件元数据
        Str filePath;                              // 文件路径
        PluginState state = PluginState::Unloaded; // 当前状态
        std::chrono::system_clock::time_point registeredTime; // 注册时间
        std::chrono::system_clock::time_point lastLoadTime;   // 最后加载时间
        u32 loadCount = 0;                         // 加载次数
        Vec<Str> loadErrors;                       // 加载错误历史
        HashMap<Str, Str> properties;              // 扩展属性
        
        PluginRegistration() {
            registeredTime = std::chrono::system_clock::now();
        }
        
        PluginRegistration(const PluginMetadata& meta, StrView path) 
            : metadata(meta), filePath(path) {
            registeredTime = std::chrono::system_clock::now();
        }
    };
    
    /**
     * 插件查询条件
     */
    struct PluginQuery {
        Opt<Str> name;                        // 插件名称
        Opt<PluginVersion> minVersion;       // 最小版本
        Opt<PluginVersion> maxVersion;       // 最大版本
        Opt<Str> author;                     // 作者
        Opt<Str> category;                   // 分类
        Opt<PluginState> state;              // 状态
        Vec<Str> tags;                            // 标签
        Vec<Str> requiredCapabilities;           // 必需功能
        bool includeDisabled = false;            // 包含禁用的插件
        
        PluginQuery() = default;
    };
    
    /**
     * 插件统计信息
     */
    struct PluginStatistics {
        size_t totalPlugins = 0;                  // 总插件数
        size_t loadedPlugins = 0;                // 已加载插件数
        size_t enabledPlugins = 0;               // 已启用插件数
        size_t failedPlugins = 0;                // 加载失败插件数
        HashMap<Str, size_t> pluginsByCategory;  // 按分类统计
        HashMap<Str, size_t> pluginsByAuthor;    // 按作者统计
        HashMap<PluginState, size_t> pluginsByState; // 按状态统计
        
        PluginStatistics() = default;
    };
    
    /**
     * 插件注册表 - 维护所有插件的注册信息和查询服务
     */
    class PluginRegistry {
    public:
        PluginRegistry();
        ~PluginRegistry() = default;
        
        // 禁用拷贝和移动
        PluginRegistry(const PluginRegistry&) = delete;
        PluginRegistry& operator=(const PluginRegistry&) = delete;
        PluginRegistry(PluginRegistry&&) = delete;
        PluginRegistry& operator=(PluginRegistry&&) = delete;
        
        // === 插件注册 ===
        
        /**
         * 注册插件
         */
        bool registerPlugin(const PluginMetadata& metadata, StrView filePath = "");
        
        /**
         * 批量注册插件
         */
        size_t registerPlugins(const Vec<std::pair<PluginMetadata, Str>>& plugins);
        
        /**
         * 取消注册插件
         */
        bool unregisterPlugin(StrView name);
        
        /**
         * 取消注册所有插件
         */
        void unregisterAllPlugins();
        
        /**
         * 更新插件注册信息
         */
        bool updateRegistration(StrView name, const PluginMetadata& metadata);
        
        // === 插件查询 ===
        
        /**
         * 检查插件是否已注册
         */
        bool isRegistered(StrView name) const;
        
        /**
         * 获取插件注册信息
         */
        Opt<PluginRegistration> getRegistration(StrView name) const;
        
        /**
         * 获取插件元数据
         */
        Opt<PluginMetadata> getMetadata(StrView name) const;
        
        /**
         * 获取所有已注册的插件名称
         */
        Vec<Str> getRegisteredPluginNames() const;
        
        /**
         * 根据条件查询插件
         */
        Vec<PluginRegistration> queryPlugins(const PluginQuery& query) const;
        
        /**
         * 查找插件（模糊匹配）
         */
        Vec<PluginRegistration> findPlugins(StrView pattern) const;
        
        /**
         * 按分类获取插件
         */
        Vec<PluginRegistration> getPluginsByCategory(StrView category) const;
        
        /**
         * 按作者获取插件
         */
        Vec<PluginRegistration> getPluginsByAuthor(StrView author) const;
        
        /**
         * 按标签获取插件
         */
        Vec<PluginRegistration> getPluginsByTag(StrView tag) const;
        
        // === 状态管理 ===
        
        /**
         * 更新插件状态
         */
        bool updatePluginState(StrView name, PluginState state);
        
        /**
         * 获取插件状态
         */
        PluginState getPluginState(StrView name) const;
        
        /**
         * 按状态获取插件
         */
        Vec<PluginRegistration> getPluginsByState(PluginState state) const;
        
        /**
         * 记录加载事件
         */
        void recordLoadEvent(StrView name, bool success, StrView error = "");
        
        // === 依赖管理 ===
        
        /**
         * 获取插件依赖
         */
        Vec<PluginDependency> getPluginDependencies(StrView name) const;
        
        /**
         * 查找依赖此插件的其他插件
         */
        Vec<Str> getDependentPlugins(StrView name) const;
        
        /**
         * 构建依赖图
         */
        HashMap<Str, Vec<Str>> buildDependencyGraph() const;
        
        /**
         * 检查循环依赖
         */
        bool hasCyclicDependency() const;
        
        /**
         * 获取加载顺序
         */
        Vec<Str> getLoadOrder() const;
        
        /**
         * 解析依赖冲突
         */
        Vec<Str> resolveDependencyConflicts() const;
        
        // === 版本管理 ===
        
        /**
         * 检查版本兼容性
         */
        bool isVersionCompatible(StrView name, const PluginVersion& requiredVersion) const;
        
        /**
         * 查找兼容版本
         */
        Vec<PluginRegistration> findCompatibleVersions(StrView name, const PluginVersion& requiredVersion) const;
        
        /**
         * 获取最新版本
         */
        Opt<PluginRegistration> getLatestVersion(StrView name) const;
        
        /**
         * 比较插件版本
         */
        int compareVersions(const PluginVersion& v1, const PluginVersion& v2) const;
        
        // === 属性管理 ===
        
        /**
         * 设置插件属性
         */
        bool setPluginProperty(StrView name, StrView key, StrView value);
        
        /**
         * 获取插件属性
         */
        Str getPluginProperty(StrView name, StrView key, StrView defaultValue = "") const;
        
        /**
         * 获取所有插件属性
         */
        HashMap<Str, Str> getPluginProperties(StrView name) const;
        
        /**
         * 移除插件属性
         */
        bool removePluginProperty(StrView name, StrView key);
        
        // === 统计和监控 ===
        
        /**
         * 获取插件统计信息
         */
        PluginStatistics getStatistics() const;
        
        /**
         * 获取插件加载历史
         */
        Vec<Str> getLoadHistory(StrView name) const;
        
        /**
         * 获取错误统计
         */
        HashMap<Str, size_t> getErrorStatistics() const;
        
        /**
         * 重置统计信息
         */
        void resetStatistics();
        
        // === 持久化 ===
        
        /**
         * 保存注册表到文件
         */
        bool saveToFile(StrView filePath) const;
        
        /**
         * 从文件加载注册表
         */
        bool loadFromFile(StrView filePath);
        
        /**
         * 导出为JSON格式
         */
        Str exportToJson() const;
        
        /**
         * 从JSON导入
         */
        bool importFromJson(StrView json);
        
        // === 缓存管理 ===
        
        /**
         * 启用查询缓存
         */
        void enableQueryCache(bool enable = true) { queryCacheEnabled_ = enable; }
        
        /**
         * 清除查询缓存
         */
        void clearQueryCache();
        
        /**
         * 获取缓存统计
         */
        HashMap<Str, size_t> getCacheStatistics() const;
        
        // === 事件通知 ===
        
        /**
         * 注册变更监听器
         */
        void addChangeListener(std::function<void(StrView, StrView)> listener);
        
        /**
         * 移除变更监听器
         */
        void removeChangeListener();
        
        // === 调试和诊断 ===
        
        /**
         * 验证注册表完整性
         */
        bool validateRegistry() const;
        
        /**
         * 获取诊断信息
         */
        HashMap<Str, Str> getDiagnostics() const;
        
        /**
         * 导出调试信息
         */
        Str exportDebugInfo() const;
        
        /**
         * 清理无效注册
         */
        size_t cleanupInvalidRegistrations();
        
    private:
        // 插件注册数据
        HashMap<Str, PluginRegistration> registrations_;
        
        // 索引数据（用于快速查询）
        HashMap<Str, Vec<Str>> categoryIndex_;     // 分类索引
        HashMap<Str, Vec<Str>> authorIndex_;       // 作者索引
        HashMap<Str, Vec<Str>> tagIndex_;          // 标签索引
        HashMap<PluginState, Vec<Str>> stateIndex_; // 状态索引
        
        // 依赖关系缓存
        mutable HashMap<Str, Vec<Str>> dependencyCache_;   // 依赖缓存
        mutable HashMap<Str, Vec<Str>> dependentCache_;    // 被依赖缓存
        mutable bool dependencyCacheValid_ = false;
        
        // 查询缓存
        mutable HashMap<Str, Vec<PluginRegistration>> queryCache_;
        bool queryCacheEnabled_ = true;
        
        // 变更监听器
        Vec<std::function<void(StrView, StrView)>> changeListeners_;
        
        // 线程安全
        mutable std::shared_mutex registryMutex_;
        mutable std::mutex cacheMutex_;
        
        // 统计信息
        mutable PluginStatistics cachedStats_;
        mutable bool statsValid_ = false;
        
        // 私有方法
        
        /**
         * 更新索引
         */
        void updateIndices(StrView name, const PluginRegistration& registration);
        
        /**
         * 移除索引
         */
        void removeFromIndices(StrView name, const PluginRegistration& registration);
        
        /**
         * 重建索引
         */
        void rebuildIndices();
        
        /**
         * 无效化缓存
         */
        void invalidateCaches();
        
        /**
         * 更新依赖缓存
         */
        void updateDependencyCache() const;
        
        /**
         * 计算统计信息
         */
        void calculateStatistics() const;
        
        /**
         * 通知变更
         */
        void notifyChange(StrView pluginName, StrView changeType);
        
        /**
         * 验证插件元数据
         */
        bool validateMetadata(const PluginMetadata& metadata) const;
        
        /**
         * 生成查询缓存键
         */
        Str generateQueryCacheKey(const PluginQuery& query) const;
        
        /**
         * 匹配查询条件
         */
        bool matchesQuery(const PluginRegistration& registration, const PluginQuery& query) const;
        
        /**
         * 拓扑排序（用于依赖解析）
         */
        Vec<Str> topologicalSort(const HashMap<Str, Vec<Str>>& graph) const;
        
        /**
         * 检测循环依赖
         */
        bool detectCycle(const HashMap<Str, Vec<Str>>& graph) const;
    };
    
    /**
     * 插件注册表工厂
     */
    class PluginRegistryFactory {
    public:
        static UPtr<PluginRegistry> create() {
            return std::make_unique<PluginRegistry>();
        }
    };
}