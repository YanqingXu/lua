#include "lib_func_registry.hpp"
#include "../utils/error_handling.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace Lua {
    namespace Lib {

        // === 构造函数 ===

        LibFuncRegistry::LibFuncRegistry() 
            : maxCacheSize_(256), cacheEnabled_(true), performanceMonitoring_(false) {}

        // === 函数注册 ===

        void LibFuncRegistry::registerFunction(const FunctionMetadata& meta, LibFunction func) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            functions_[meta.name] = std::move(func);
            metadata_[meta.name] = meta;
            
            // 清除缓存中的旧版本
            if (cacheEnabled_) {
                cache_.erase(meta.name);
            }
        }

        void LibFuncRegistry::registerFunction(StrView name, LibFunction func) {
            FunctionMetadata meta(name);
            registerFunction(meta, std::move(func));
        }

        void LibFuncRegistry::registerFunction(StrView name, LibFunction func, StrView description) {
            FunctionMetadata meta(name);
            meta.description = Str(description);
            registerFunction(meta, std::move(func));
        }

        void LibFuncRegistry::registerFunctions(const std::vector<FunctionRegistration>& functions) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            for (const auto& reg : functions) {
                functions_[reg.metadata.name] = reg.function;
                metadata_[reg.metadata.name] = reg.metadata;
                
                // 清除缓存
                if (cacheEnabled_) {
                    cache_.erase(reg.metadata.name);
                }
            }
        }

        // === 函数调用 ===

        Value LibFuncRegistry::callFunction(StrView name, State* state, i32 nargs) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // 获取函数
            LibFunction func = getCachedFunction(name);
            if (!func) {
                throw LibException(LibErrorCode::InvalidArgument, "Function not found: " + Str(name));
            }
            
            // 验证参数（如果有元数据）
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                auto metaIt = metadata_.find(Str(name));
                if (metaIt != metadata_.end()) {
                    const auto& meta = metaIt->second;
                    if (nargs < meta.minArgs) {
                        throw LibException(LibErrorCode::InvalidArgument, 
                                         "Too few arguments for function " + Str(name) + 
                                         ": expected at least " + std::to_string(meta.minArgs) + 
                                         ", got " + std::to_string(nargs));
                    }
                    if (meta.maxArgs >= 0 && nargs > meta.maxArgs) {
                        throw LibException(LibErrorCode::InvalidArgument, 
                                         "Too many arguments for function " + Str(name) + 
                                         ": expected at most " + std::to_string(meta.maxArgs) + 
                                         ", got " + std::to_string(nargs));
                    }
                }
            }
            
            // 调用函数
            Value result = func(state, nargs);
            
            // 记录性能统计
            if (performanceMonitoring_) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                recordFunctionCall(name, duration);
            }
            
            return result;
        }

        LibFunction LibFuncRegistry::getFunction(StrView name) const {
            return getCachedFunction(name);
        }

        bool LibFuncRegistry::hasFunction(StrView name) const noexcept {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return functions_.find(Str(name)) != functions_.end();
        }

        // === 元数据查询 ===

        const FunctionMetadata* LibFuncRegistry::getFunctionMetadata(StrView name) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = metadata_.find(Str(name));
            return (it != metadata_.end()) ? &it->second : nullptr;
        }

        std::vector<Str> LibFuncRegistry::getFunctionNames() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<Str> names;
            names.reserve(functions_.size());
            
            for (const auto& [name, _] : functions_) {
                names.push_back(name);
            }
            
            std::sort(names.begin(), names.end());
            return names;
        }

        std::vector<Str> LibFuncRegistry::getFunctionsByCategory(StrView category) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<Str> names;
            Str categoryStr(category);
            
            // Note: Category filtering not implemented yet - returning all functions
            // TODO: Add category field to FunctionMetadata
            for (const auto& [name, meta] : metadata_) {
                // Temporary implementation: filter by name prefix as category
                if (name.find(categoryStr) == 0) {
                    names.push_back(name);
                }
            }
            
            std::sort(names.begin(), names.end());
            return names;
        }

        std::vector<Str> LibFuncRegistry::searchFunctions(StrView query) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<Str> results;
            Str queryStr(query);
            std::transform(queryStr.begin(), queryStr.end(), queryStr.begin(), ::tolower);
            
            for (const auto& [name, meta] : metadata_) {
                Str nameStr = name;
                Str descStr = meta.description;
                std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
                std::transform(descStr.begin(), descStr.end(), descStr.begin(), ::tolower);
                
                if (nameStr.find(queryStr) != Str::npos || 
                    descStr.find(queryStr) != Str::npos) {
                    results.push_back(name);
                }
            }
            
            std::sort(results.begin(), results.end());
            return results;
        }

        // === 缓存管理 ===

        void LibFuncRegistry::enableCache(bool enable) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            cacheEnabled_ = enable;
            if (!enable) {
                cache_.clear();
                cacheHits_ = 0;
                cacheMisses_ = 0;
            }
        }

        void LibFuncRegistry::clearCache() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            cache_.clear();
            cacheHits_ = 0;
            cacheMisses_ = 0;
        }

        void LibFuncRegistry::setMaxCacheSize(size_t size) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            maxCacheSize_ = size;
            
            // 如果当前缓存超过限制，清理旧项
            while (cache_.size() > maxCacheSize_) {
                cache_.erase(cache_.begin());
            }
        }

        LibFuncRegistry::CacheStats LibFuncRegistry::getCacheStats() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            CacheStats stats;
            stats.hitCount = cacheHits_;
            stats.missCount = cacheMisses_;
            stats.cacheSize = cache_.size();
            if (cacheHits_ + cacheMisses_ > 0) {
                stats.hitRate = static_cast<double>(cacheHits_) / (cacheHits_ + cacheMisses_);
            }
            return stats;
        }

        // === 性能监控 ===

        void LibFuncRegistry::enablePerformanceMonitoring(bool enable) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            performanceMonitoring_ = enable;
            if (!enable) {
                stats_.clear();
            }
        }

        CallStats LibFuncRegistry::getFunctionStats(StrView name) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = stats_.find(Str(name));
            return it != stats_.end() ? it->second : CallStats{};
        }

        std::unordered_map<Str, CallStats> LibFuncRegistry::getAllStats() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return stats_;
        }

        void LibFuncRegistry::resetStats() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            stats_.clear();
            cacheHits_ = 0;
            cacheMisses_ = 0;
        }

        // === 注册表管理 ===

        void LibFuncRegistry::clear() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            functions_.clear();
            metadata_.clear();
            cache_.clear();
            stats_.clear();
            cacheHits_ = 0;
            cacheMisses_ = 0;
        }

        bool LibFuncRegistry::removeFunction(StrView name) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            Str nameStr(name);
            
            bool removed = false;
            if (functions_.erase(nameStr)) {
                removed = true;
            }
            metadata_.erase(nameStr);
            cache_.erase(nameStr);
            stats_.erase(nameStr);
            
            return removed;
        }

        size_t LibFuncRegistry::size() const noexcept {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return functions_.size();
        }

        bool LibFuncRegistry::empty() const noexcept {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return functions_.empty();
        }

        // === 调试和诊断 ===

        bool LibFuncRegistry::validateRegistry() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            
            // 检查所有函数都有对应的元数据
            for (const auto& [name, _] : functions_) {
                if (metadata_.find(name) == metadata_.end()) {
                    return false;
                }
            }
            
            // 检查所有元数据都有对应的函数
            for (const auto& [name, _] : metadata_) {
                if (functions_.find(name) == functions_.end()) {
                    return false;
                }
            }
            
            return true;
        }

        LibFuncRegistry::RegistryStats LibFuncRegistry::getRegistryStats() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            RegistryStats stats;
            stats.functionCount = functions_.size();
            
            // Category counting not implemented yet - setting to 0
            // TODO: Add category field to FunctionMetadata
            stats.categoryCount = 0;
            
            // 统计调用信息
            for (const auto& [_, callStat] : stats_) {
                stats.totalCalls += callStat.callCount;
                stats.totalExecutionTime += callStat.totalTime;
            }
            
            // 获取最常调用的函数
            std::vector<std::pair<Str, size_t>> callCounts;
            for (const auto& [name, callStat] : stats_) {
                callCounts.emplace_back(name, callStat.callCount);
            }
            
            std::sort(callCounts.begin(), callCounts.end(),
                     [](const auto& a, const auto& b) {
                         return a.second > b.second;
                     });
            
            for (size_t i = 0; i < std::min(size_t(5), callCounts.size()); ++i) {
                stats.topCalledFunctions.push_back(callCounts[i].first);
            }
            
            return stats;
        }

        void LibFuncRegistry::printDiagnostics() const {
            auto regStats = getRegistryStats();
            auto cacheStats = getCacheStats();
            
            std::cout << "=== LibFuncRegistry Diagnostics ===" << std::endl;
            std::cout << "Functions: " << regStats.functionCount << std::endl;
            std::cout << "Categories: " << regStats.categoryCount << std::endl;
            std::cout << "Total calls: " << regStats.totalCalls << std::endl;
            std::cout << "Total execution time: " << regStats.totalExecutionTime.count() << "ms" << std::endl;
            
            if (cacheEnabled_) {
                std::cout << "Cache size: " << cacheStats.cacheSize << std::endl;
                std::cout << "Cache hit rate: " << std::fixed << std::setprecision(2) 
                          << (cacheStats.hitRate * 100) << "%" << std::endl;
            }
            
            if (!regStats.topCalledFunctions.empty()) {
                std::cout << "Top called functions:" << std::endl;
                for (const auto& name : regStats.topCalledFunctions) {
                    auto stats = getFunctionStats(name);
                    std::cout << "  " << name << ": " << stats.callCount 
                              << " calls, avg " << stats.avgTime.count() << "ms" << std::endl;
                }
            }
        }

        // === 内部辅助方法 ===

        LibFunction LibFuncRegistry::getCachedFunction(StrView name) const {
            Str nameStr(name);
            
            if (cacheEnabled_) {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                auto cacheIt = cache_.find(nameStr);
                if (cacheIt != cache_.end()) {
                    cacheHits_++;
                    return cacheIt->second;
                }
                cacheMisses_++;
            }
            
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = functions_.find(nameStr);
            if (it != functions_.end()) {
                if (cacheEnabled_) {
                    updateCache(nameStr, it->second);
                }
                return it->second;
            }
            
            return nullptr;
        }

        void LibFuncRegistry::updateCache(StrView name, LibFunction func) const {
            if (!cacheEnabled_) return;
            
            Str nameStr(name);
            if (cache_.size() >= maxCacheSize_) {
                // 简单的FIFO策略，移除第一个元素
                cache_.erase(cache_.begin());
            }
            cache_[nameStr] = func;
        }

        void LibFuncRegistry::recordFunctionCall(StrView name, std::chrono::milliseconds duration) const {
            if (!performanceMonitoring_) return;
            
            std::unique_lock<std::shared_mutex> lock(mutex_);
            stats_[Str(name)].addCall(duration);
        }

    } // namespace Lib
} // namespace Lua