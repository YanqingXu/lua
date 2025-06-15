#pragma once

#include "plugin_interface.hpp"
#include "../../common/types.hpp"
#include "../../vm/state.hpp"
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>

namespace Lua {
    /**
     * 资源限制配置
     */
    struct ResourceLimits {
        size_t maxMemoryUsage = 64 * 1024 * 1024;    // 最大内存使用量 (64MB)
        size_t maxFileSize = 10 * 1024 * 1024;       // 最大文件大小 (10MB)
        size_t maxOpenFiles = 100;                   // 最大打开文件数
        u32 maxExecutionTime = 30000;                // 最大执行时间 (30秒)
        u32 maxCpuTime = 10000;                      // 最大CPU时间 (10秒)
        size_t maxStackDepth = 1000;                 // 最大调用栈深度
        size_t maxStringLength = 1024 * 1024;        // 最大字符串长度 (1MB)
        size_t maxTableSize = 10000;                 // 最大表大小
        u32 maxNetworkConnections = 10;              // 最大网络连接数
        
        ResourceLimits() = default;
    };
    
    /**
     * 权限类型
     */
    enum class PermissionType {
        FileRead,           // 文件读取
        FileWrite,          // 文件写入
        FileExecute,        // 文件执行
        NetworkAccess,      // 网络访问
        SystemCall,         // 系统调用
        ProcessCreate,      // 进程创建
        RegistryAccess,     // 注册表访问 (Windows)
        EnvironmentAccess,  // 环境变量访问
        LibraryLoad,        // 动态库加载
        DebugAccess,        // 调试访问
        AdminAccess         // 管理员权限
    };
    
    /**
     * 权限配置
     */
    struct PermissionConfig {
        HashMap<PermissionType, bool> permissions;
        Vec<Str> allowedPaths;          // 允许访问的路径
        Vec<Str> blockedPaths;          // 禁止访问的路径
        Vec<Str> allowedHosts;          // 允许访问的主机
        Vec<Str> blockedHosts;          // 禁止访问的主机
        Vec<Str> allowedLibraries;      // 允许加载的库
        Vec<Str> blockedLibraries;      // 禁止加载的库
        
        PermissionConfig() {
            // 默认权限配置（最小权限原则）
            permissions[PermissionType::FileRead] = true;
            permissions[PermissionType::FileWrite] = false;
            permissions[PermissionType::FileExecute] = false;
            permissions[PermissionType::NetworkAccess] = false;
            permissions[PermissionType::SystemCall] = false;
            permissions[PermissionType::ProcessCreate] = false;
            permissions[PermissionType::RegistryAccess] = false;
            permissions[PermissionType::EnvironmentAccess] = false;
            permissions[PermissionType::LibraryLoad] = false;
            permissions[PermissionType::DebugAccess] = false;
            permissions[PermissionType::AdminAccess] = false;
        }
    };
    
    /**
     * 沙箱违规类型
     */
    enum class ViolationType {
        MemoryLimit,        // 内存限制违规
        TimeLimit,          // 时间限制违规
        FileAccess,         // 文件访问违规
        NetworkAccess,      // 网络访问违规
        SystemCall,         // 系统调用违规
        PermissionDenied,   // 权限拒绝
        ResourceExhaustion, // 资源耗尽
        StackOverflow,      // 栈溢出
        InvalidOperation    // 无效操作
    };
    
    /**
     * 沙箱违规事件
     */
    struct ViolationEvent {
        ViolationType type;
        Str pluginName;
        Str description;
        Str details;
        std::chrono::system_clock::time_point timestamp;
        
        ViolationEvent(ViolationType t, StrView plugin, StrView desc, StrView det = "")
            : type(t), pluginName(plugin), description(desc), details(det) {
            timestamp = std::chrono::system_clock::now();
        }
    };
    
    /**
     * 资源使用统计
     */
    struct ResourceUsage {
        size_t currentMemory = 0;       // 当前内存使用
        size_t peakMemory = 0;          // 峰值内存使用
        u32 executionTime = 0;          // 执行时间 (毫秒)
        u32 cpuTime = 0;                // CPU时间 (毫秒)
        size_t currentStackDepth = 0;   // 当前栈深度
        size_t maxStackDepth = 0;       // 最大栈深度
        size_t openFiles = 0;           // 打开文件数
        size_t networkConnections = 0;  // 网络连接数
        u64 bytesRead = 0;              // 读取字节数
        u64 bytesWritten = 0;           // 写入字节数
        
        ResourceUsage() = default;
    };
    
    /**
     * 插件沙箱 - 提供安全隔离和资源限制
     */
    class PluginSandbox {
    public:
        explicit PluginSandbox(StrView pluginName);
        ~PluginSandbox();
        
        // 禁用拷贝和移动
        PluginSandbox(const PluginSandbox&) = delete;
        PluginSandbox& operator=(const PluginSandbox&) = delete;
        PluginSandbox(PluginSandbox&&) = delete;
        PluginSandbox& operator=(PluginSandbox&&) = delete;
        
        // === 沙箱配置 ===
        
        /**
         * 设置资源限制
         */
        void setResourceLimits(const ResourceLimits& limits);
        
        /**
         * 获取资源限制
         */
        const ResourceLimits& getResourceLimits() const { return limits_; }
        
        /**
         * 设置权限配置
         */
        void setPermissionConfig(const PermissionConfig& config);
        
        /**
         * 获取权限配置
         */
        const PermissionConfig& getPermissionConfig() const { return permissions_; }
        
        /**
         * 启用沙箱
         */
        void enable(bool enabled = true) { enabled_ = enabled; }
        
        /**
         * 检查沙箱是否启用
         */
        bool isEnabled() const { return enabled_; }
        
        // === 权限检查 ===
        
        /**
         * 检查权限
         */
        bool checkPermission(PermissionType type) const;
        
        /**
         * 检查文件访问权限
         */
        bool checkFileAccess(StrView path, PermissionType accessType) const;
        
        /**
         * 检查网络访问权限
         */
        bool checkNetworkAccess(StrView host, u16 port) const;
        
        /**
         * 检查库加载权限
         */
        bool checkLibraryLoad(StrView libraryPath) const;
        
        /**
         * 授予临时权限
         */
        void grantTemporaryPermission(PermissionType type, u32 durationMs);
        
        /**
         * 撤销临时权限
         */
        void revokeTemporaryPermission(PermissionType type);
        
        // === 资源监控 ===
        
        /**
         * 检查内存使用
         */
        bool checkMemoryUsage(size_t requestedSize) const;
        
        /**
         * 记录内存分配
         */
        void recordMemoryAllocation(size_t size);
        
        /**
         * 记录内存释放
         */
        void recordMemoryDeallocation(size_t size);
        
        /**
         * 检查执行时间
         */
        bool checkExecutionTime() const;
        
        /**
         * 开始执行计时
         */
        void startExecution();
        
        /**
         * 结束执行计时
         */
        void endExecution();
        
        /**
         * 检查栈深度
         */
        bool checkStackDepth(size_t depth) const;
        
        /**
         * 记录栈操作
         */
        void recordStackPush();
        void recordStackPop();
        
        /**
         * 记录文件操作
         */
        void recordFileOpen();
        void recordFileClose();
        void recordFileRead(size_t bytes);
        void recordFileWrite(size_t bytes);
        
        /**
         * 记录网络操作
         */
        void recordNetworkConnection();
        void recordNetworkDisconnection();
        
        // === 资源统计 ===
        
        /**
         * 获取资源使用统计
         */
        ResourceUsage getResourceUsage() const;
        
        /**
         * 重置资源统计
         */
        void resetResourceUsage();
        
        /**
         * 获取资源使用率
         */
        HashMap<Str, double> getResourceUtilization() const;
        
        // === 违规处理 ===
        
        /**
         * 记录违规事件
         */
        void recordViolation(ViolationType type, StrView description, StrView details = "") const;
        
        /**
         * 获取违规历史
         */
        Vec<ViolationEvent> getViolationHistory() const;
        
        /**
         * 清除违规历史
         */
        void clearViolationHistory();
        
        /**
         * 获取违规统计
         */
        HashMap<ViolationType, size_t> getViolationStatistics() const;
        
        /**
         * 设置违规处理器
         */
        void setViolationHandler(std::function<void(const ViolationEvent&)> handler);
        
        // === 沙箱控制 ===
        
        /**
         * 暂停插件执行
         */
        void suspend();
        
        /**
         * 恢复插件执行
         */
        void resume();
        
        /**
         * 检查是否暂停
         */
        bool isSuspended() const { return suspended_; }
        
        /**
         * 终止插件执行
         */
        void terminate();
        
        /**
         * 检查是否终止
         */
        bool isTerminated() const { return terminated_; }
        
        // === 安全策略 ===
        
        /**
         * 设置严格模式
         */
        void setStrictMode(bool strict = true) { strictMode_ = strict; }
        
        /**
         * 检查是否为严格模式
         */
        bool isStrictMode() const { return strictMode_; }
        
        /**
         * 启用审计日志
         */
        void enableAuditLog(bool enable = true) { auditLogEnabled_ = enable; }
        
        /**
         * 获取审计日志
         */
        Vec<Str> getAuditLog() const;
        
        /**
         * 清除审计日志
         */
        void clearAuditLog();
        
        // === 调试和诊断 ===
        
        /**
         * 启用调试模式
         */
        void setDebugMode(bool debug = true) { debugMode_ = debug; }
        
        /**
         * 检查是否为调试模式
         */
        bool isDebugMode() const { return debugMode_; }
        
        /**
         * 获取沙箱状态
         */
        HashMap<Str, Str> getSandboxStatus() const;
        
        /**
         * 导出沙箱配置
         */
        Str exportConfiguration() const;
        
        /**
         * 导入沙箱配置
         */
        bool importConfiguration(StrView config);
        
        /**
         * 验证沙箱完整性
         */
        bool validateIntegrity() const;
        
    private:
        Str pluginName_;                    // 插件名称
        bool enabled_ = true;               // 沙箱是否启用
        bool suspended_ = false;            // 是否暂停
        bool terminated_ = false;           // 是否终止
        bool strictMode_ = false;           // 严格模式
        bool auditLogEnabled_ = true;       // 审计日志
        bool debugMode_ = false;            // 调试模式
        
        ResourceLimits limits_;             // 资源限制
        PermissionConfig permissions_;      // 权限配置
        ResourceUsage usage_;               // 资源使用统计
        
        // 临时权限
        HashMap<PermissionType, std::chrono::system_clock::time_point> temporaryPermissions_;
        
        // 违规记录
        Vec<ViolationEvent> violationHistory_;
        std::function<void(const ViolationEvent&)> violationHandler_;
        
        // 审计日志
        Vec<Str> auditLog_;
        
        // 执行计时
        std::chrono::high_resolution_clock::time_point executionStart_;
        std::atomic<bool> executionActive_{false};
        
        // 线程安全
        mutable std::mutex usageMutex_;
        mutable std::mutex violationMutex_;
        mutable std::mutex auditMutex_;
        
        // 私有方法
        
        /**
         * 检查路径权限
         */
        bool checkPathPermission(StrView path, const Vec<Str>& allowedPaths, const Vec<Str>& blockedPaths) const;
        
        /**
         * 检查主机权限
         */
        bool checkHostPermission(StrView host, const Vec<Str>& allowedHosts, const Vec<Str>& blockedHosts) const;
        
        /**
         * 更新资源使用统计
         */
        void updateResourceUsage();
        
        /**
         * 检查临时权限是否过期
         */
        void cleanupExpiredPermissions();
        
        /**
         * 记录审计日志
         */
        void logAudit(StrView operation, StrView details = "");
        
        /**
         * 触发违规处理
         */
        void handleViolation(const ViolationEvent& event);
        
        /**
         * 路径匹配
         */
        bool matchPath(StrView path, StrView pattern) const;
        
        /**
         * 主机匹配
         */
        bool matchHost(StrView host, StrView pattern) const;
        
        /**
         * 格式化时间
         */
        Str formatTime(const std::chrono::system_clock::time_point& time) const;
        
        /**
         * 获取当前时间戳
         */
        u64 getCurrentTimestamp() const;
    };
    
    /**
     * 沙箱管理器 - 管理多个插件沙箱
     */
    class SandboxManager {
    public:
        SandboxManager() = default;
        ~SandboxManager() = default;
        
        /**
         * 创建沙箱
         */
        UPtr<PluginSandbox> createSandbox(StrView pluginName);
        
        /**
         * 获取沙箱
         */
        PluginSandbox* getSandbox(StrView pluginName) const;
        
        /**
         * 移除沙箱
         */
        bool removeSandbox(StrView pluginName);
        
        /**
         * 获取所有沙箱
         */
        Vec<Str> getAllSandboxes() const;
        
        /**
         * 设置全局资源限制
         */
        void setGlobalResourceLimits(const ResourceLimits& limits);
        
        /**
         * 设置全局权限配置
         */
        void setGlobalPermissionConfig(const PermissionConfig& config);
        
        /**
         * 获取全局统计
         */
        HashMap<Str, ResourceUsage> getGlobalResourceUsage() const;
        
        /**
         * 获取全局违规统计
         */
        HashMap<Str, Vec<ViolationEvent>> getGlobalViolations() const;
        
    private:
        HashMap<Str, UPtr<PluginSandbox>> sandboxes_;
        ResourceLimits globalLimits_;
        PermissionConfig globalPermissions_;
        mutable std::shared_mutex sandboxesMutex_;
    };
    
    /**
     * 沙箱工厂
     */
    class PluginSandboxFactory {
    public:
        static UPtr<PluginSandbox> create(StrView pluginName) {
            return std::make_unique<PluginSandbox>(pluginName);
        }
        
        static UPtr<SandboxManager> createManager() {
            return std::make_unique<SandboxManager>();
        }
    };
}