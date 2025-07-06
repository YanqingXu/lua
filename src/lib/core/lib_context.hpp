#pragma once

#include "lib_define.hpp"
#include <unordered_map>
#include <memory>
#include <optional>
#include <any>
#include <typeinfo>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <chrono>

namespace Lua {
    namespace Lib {

        // 安全级别枚举
        enum class SandboxLevel {
            None,       // 无限制
            Basic,      // 基础限制
            Strict,     // 严格限制
            Full        // 完全沙箱
        };

        // 日志级别枚举
        enum class LogLevel {
            Debug,
            Info,
            Warning,
            Error,
            Critical
        };

        /**
         * Enhanced library context for dependency injection and configuration
         * 支持类型安全的配置管理、依赖注入、环境设置和安全控制
         */
        class LibContext {
        public:
            LibContext() = default;
            LibContext(const LibContext& other);
            LibContext& operator=(const LibContext& other);
            
            // === 配置管理 ===
            
            /**
             * 设置配置值（类型安全）
             */
            template<typename T>
            void setConfig(StrView key, T&& value) {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                config_[Str(key)] = std::forward<T>(value);
            }
            
            /**
             * 获取配置值（类型安全）
             */
            template<typename T>
            std::optional<T> getConfig(StrView key) const {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                auto it = config_.find(Str(key));
                if (it != config_.end()) {
                    try {
                        return std::any_cast<T>(it->second);
                    } catch (const std::bad_any_cast&) {
                        return std::nullopt;
                    }
                }
                return std::nullopt;
            }
            
            /**
             * 检查配置是否存在
             */
            bool hasConfig(StrView key) const;
            
            /**
             * 移除配置
             */
            void removeConfig(StrView key);
            
            /**
             * 清空所有配置
             */
            void clearConfig();
            
            // === 批量配置 ===
            
            /**
             * 从文件加载配置
             */
            void setConfigFromFile(StrView filename);
            
            /**
             * 从字符串加载配置
             */
            void setConfigFromString(StrView configStr);
            
            /**
             * 合并其他上下文的配置
             */
            void mergeConfig(const LibContext& other);
            
            // === 依赖注入 ===
            
            /**
             * 添加依赖对象
             */
            template<typename T>
            void addDependency(std::shared_ptr<T> dependency) {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                dependencies_[typeid(T).name()] = dependency;
            }
            
            /**
             * 获取依赖对象
             */
            template<typename T>
            std::shared_ptr<T> getDependency() const {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                auto it = dependencies_.find(typeid(T).name());
                if (it != dependencies_.end()) {
                    return std::static_pointer_cast<T>(it->second);
                }
                return nullptr;
            }
            
            /**
             * 检查是否有某个依赖
             */
            template<typename T>
            bool hasDependency() const {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                return dependencies_.find(typeid(T).name()) != dependencies_.end();
            }
            
            // === 环境变量 ===
            
            /**
             * 设置环境变量
             */
            void setEnvironment(StrView key, StrView value);
            
            /**
             * 获取环境变量
             */
            std::optional<Str> getEnvironment(StrView key) const;
            
            /**
             * 获取所有环境变量
             */
            std::unordered_map<Str, Str> getAllEnvironment() const;
            
            // === 日志配置 ===
            
            /**
             * 设置日志级别
             */
            void setLogLevel(LogLevel level);
            
            /**
             * 获取日志级别
             */
            LogLevel getLogLevel() const;
            
            /**
             * 启用详细日志
             */
            void enableDetailedLogging(bool enable);
            
            /**
             * 检查是否启用详细日志
             */
            bool isDetailedLoggingEnabled() const;
            
            // === 性能配置 ===
            
            /**
             * 设置最大缓存大小
             */
            void setMaxCacheSize(size_t size);
            
            /**
             * 获取最大缓存大小
             */
            size_t getMaxCacheSize() const;
            
            /**
             * 设置加载超时时间
             */
            void setLoadTimeout(std::chrono::milliseconds timeout);
            
            /**
             * 获取加载超时时间
             */
            std::chrono::milliseconds getLoadTimeout() const;
            
            /**
             * 启用异步加载
             */
            void enableAsyncLoading(bool enable);
            
            /**
             * 检查是否启用异步加载
             */
            bool isAsyncLoadingEnabled() const;
            
            // === 安全配置 ===
            
            /**
             * 启用安全模式
             */
            void enableSafeMode(bool enable);
            
            /**
             * 检查是否启用安全模式
             */
            bool isSafeModeEnabled() const;
            
            /**
             * 设置沙箱级别
             */
            void setSandboxLevel(SandboxLevel level);
            
            /**
             * 获取沙箱级别
             */
            SandboxLevel getSandboxLevel() const;
            
            /**
             * 添加受信任路径
             */
            void addTrustedPath(StrView path);
            
            /**
             * 获取所有受信任路径
             */
            std::vector<Str> getTrustedPaths() const;
            
            /**
             * 检查路径是否受信任
             */
            bool isPathTrusted(StrView path) const;
            
            // === 调试和统计 ===
            
            /**
             * 启用性能监控
             */
            void enablePerformanceMonitoring(bool enable);
            
            /**
             * 检查是否启用性能监控
             */
            bool isPerformanceMonitoringEnabled() const;
            
            /**
             * 获取配置统计信息
             */
            struct ContextStats {
                size_t configCount = 0;
                size_t dependencyCount = 0;
                size_t environmentCount = 0;
                size_t trustedPathCount = 0;
            };
            
            ContextStats getStats() const;

        private:
            // 线程安全
            mutable std::shared_mutex mutex_;
            
            // 配置存储
            std::unordered_map<Str, std::any> config_;
            
            // 依赖注入
            std::unordered_map<Str, std::shared_ptr<void>> dependencies_;
            
            // 环境变量
            std::unordered_map<Str, Str> environment_;
            
            // 日志配置
            LogLevel logLevel_ = LogLevel::Info;
            bool detailedLogging_ = false;
            
            // 性能配置
            size_t maxCacheSize_ = 1024;
            std::chrono::milliseconds loadTimeout_{5000};
            bool asyncLoading_ = false;
            
            // 安全配置
            bool safeMode_ = false;
            SandboxLevel sandboxLevel_ = SandboxLevel::None;
            std::vector<Str> trustedPaths_;
            
            // 监控配置
            bool performanceMonitoring_ = false;
            
            // 内部辅助方法
            void copyFrom(const LibContext& other);
        };

    } // namespace Lib
} // namespace Lua