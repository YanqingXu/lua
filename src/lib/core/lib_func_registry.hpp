#pragma once

#include "lib_define.hpp"
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <shared_mutex>

namespace Lua {
    namespace Lib {

        // LibFunction and FunctionMetadata moved to lib_define.hpp

        /**
         * 函数调用统计信息
         */
        struct CallStats {
            size_t callCount = 0;
            std::chrono::milliseconds totalTime{0};
            std::chrono::milliseconds avgTime{0};
            std::chrono::milliseconds minTime{std::chrono::milliseconds::max()};
            std::chrono::milliseconds maxTime{0};
            
            void addCall(std::chrono::milliseconds time) {
                callCount++;
                totalTime += time;
                avgTime = totalTime / callCount;
                minTime = std::min(minTime, time);
                maxTime = std::max(maxTime, time);
            }
        };

        /**
         * Enhanced function registry with metadata support and performance monitoring
         * 支持元数据、性能监控、缓存和批量操作的高级函数注册表
         */
        class LibFuncRegistry {
        public:
            LibFuncRegistry();
            ~LibFuncRegistry() = default;

            // === 函数注册 ===
            
            /**
             * 使用元数据注册函数
             */
            void registerFunction(const FunctionMetadata& meta, LibFunction func);
            
            /**
             * 简单函数注册（便捷方法）
             */
            void registerFunction(StrView name, LibFunction func);
            
            /**
             * 使用描述信息注册函数
             */
            void registerFunction(StrView name, LibFunction func, StrView description);
            
            /**
             * 批量注册函数
             */
            struct FunctionRegistration {
                FunctionMetadata metadata;
                LibFunction function;
                
                FunctionRegistration(StrView name, LibFunction func)
                    : function(func) {
                    metadata.name = Str(name);
                }
                
                FunctionRegistration(const FunctionMetadata& meta, LibFunction func)
                    : metadata(meta), function(func) {}
            };
            
            void registerFunctions(const std::vector<FunctionRegistration>& functions);
            
            // === 函数调用 ===
            
            /**
             * 调用已注册的函数
             */
            Value callFunction(StrView name, State* state, i32 nargs);
            
            /**
             * 获取函数指针（用于直接调用）
             */
            LibFunction getFunction(StrView name) const;
            
            /**
             * 检查函数是否存在
             */
            bool hasFunction(StrView name) const noexcept;
            
            // === 元数据查询 ===
            
            /**
             * 获取函数元数据
             */
            const FunctionMetadata* getFunctionMetadata(StrView name) const;
            
            /**
             * 获取所有已注册函数名
             */
            std::vector<Str> getFunctionNames() const;
            
            /**
             * 按类别获取函数名
             */
            std::vector<Str> getFunctionsByCategory(StrView category) const;
            
            /**
             * 搜索函数（按名称或描述）
             */
            std::vector<Str> searchFunctions(StrView query) const;
            
            // === 缓存管理 ===
            
            /**
             * 启用函数查找缓存
             */
            void enableCache(bool enable);
            
            /**
             * 清空缓存
             */
            void clearCache();
            
            /**
             * 设置最大缓存大小
             */
            void setMaxCacheSize(size_t size);
            
            /**
             * 获取缓存统计
             */
            struct CacheStats {
                size_t hitCount = 0;
                size_t missCount = 0;
                size_t cacheSize = 0;
                double hitRate = 0.0;
            };
            
            CacheStats getCacheStats() const;
            
            // === 性能监控 ===
            
            /**
             * 启用性能监控
             */
            void enablePerformanceMonitoring(bool enable);
            
            /**
             * 获取函数调用统计
             */
            CallStats getFunctionStats(StrView name) const;
            
            /**
             * 获取所有函数的统计信息
             */
            std::unordered_map<Str, CallStats> getAllStats() const;
            
            /**
             * 重置统计信息
             */
            void resetStats();
            
            // === 注册表管理 ===
            
            /**
             * 清空所有函数
             */
            void clear();
            
            /**
             * 移除特定函数
             */
            bool removeFunction(StrView name);
            
            /**
             * 获取注册函数数量
             */
            size_t size() const noexcept;
            
            /**
             * 检查注册表是否为空
             */
            bool empty() const noexcept;
            
            // === 调试和诊断 ===
            
            /**
             * 验证所有注册的函数
             */
            bool validateRegistry() const;
            
            /**
             * 获取注册表统计信息
             */
            struct RegistryStats {
                size_t functionCount = 0;
                size_t categoryCount = 0;
                size_t totalCalls = 0;
                std::chrono::milliseconds totalExecutionTime{0};
                std::vector<Str> topCalledFunctions;
            };
            
            RegistryStats getRegistryStats() const;
            
            /**
             * 打印诊断信息
             */
            void printDiagnostics() const;

        private:
            // 内部数据结构
            std::unordered_map<Str, LibFunction> functions_;
            std::unordered_map<Str, FunctionMetadata> metadata_;
            
            // 缓存系统
            mutable std::unordered_map<Str, LibFunction> cache_;
            mutable size_t cacheHits_ = 0;
            mutable size_t cacheMisses_ = 0;
            size_t maxCacheSize_ = 256;
            bool cacheEnabled_ = true;
            
            // 性能监控
            mutable std::unordered_map<Str, CallStats> stats_;
            bool performanceMonitoring_ = false;
            
            // 线程安全
            mutable std::shared_mutex mutex_;
            
            // 内部辅助方法
            void updateCache(StrView name, LibFunction func) const;
            LibFunction getCachedFunction(StrView name) const;
            void recordFunctionCall(StrView name, std::chrono::milliseconds duration) const;
        };

        // Macros moved to lib_define.hpp

    } // namespace Lib
} // namespace Lua