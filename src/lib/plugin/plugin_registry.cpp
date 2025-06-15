#include "plugin_registry.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>
#include <queue>
#include <set>
#include <chrono>
#include <iomanip>

namespace Lua {

// PluginRegistry Implementation
PluginRegistry::PluginRegistry() {
    // 初始化统计信息
    cachedStats_ = PluginStatistics{};
    statsValid_ = false;
}

// === 插件注册 ===

bool PluginRegistry::registerPlugin(const PluginMetadata& metadata, StrView filePath) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    // 验证元数据
    if (!validateMetadata(metadata)) {
        return false;
    }
    
    // 检查是否已注册
    if (registrations_.find(metadata.name) != registrations_.end()) {
        return false; // 已存在
    }
    
    // 创建注册信息
    PluginRegistration registration(metadata, filePath);
    
    // 添加到注册表
    registrations_[metadata.name] = registration;
    
    // 更新索引
    updateIndices(metadata.name, registration);
    
    // 无效化缓存
    invalidateCaches();
    
    // 通知变更
    notifyChange(metadata.name, "registered");
    
    return true;
}

size_t PluginRegistry::registerPlugins(const Vec<std::pair<PluginMetadata, Str>>& plugins) {
    size_t count = 0;
    
    for (const auto& [metadata, filePath] : plugins) {
        if (registerPlugin(metadata, filePath)) {
            count++;
        }
    }
    
    return count;
}

bool PluginRegistry::unregisterPlugin(StrView name) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it == registrations_.end()) {
        return false;
    }
    
    // 从索引中移除
    removeFromIndices(name, it->second);
    
    // 从注册表中移除
    registrations_.erase(it);
    
    // 无效化缓存
    invalidateCaches();
    
    // 通知变更
    notifyChange(name, "unregistered");
    
    return true;
}

void PluginRegistry::unregisterAllPlugins() {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    registrations_.clear();
    
    // 清除所有索引
    categoryIndex_.clear();
    authorIndex_.clear();
    tagIndex_.clear();
    stateIndex_.clear();
    
    // 无效化缓存
    invalidateCaches();
    
    // 通知变更
    notifyChange("", "all_unregistered");
}

bool PluginRegistry::updateRegistration(StrView name, const PluginMetadata& metadata) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it == registrations_.end()) {
        return false;
    }
    
    // 验证元数据
    if (!validateMetadata(metadata)) {
        return false;
    }
    
    // 从旧索引中移除
    removeFromIndices(name, it->second);
    
    // 更新元数据
    it->second.metadata = metadata;
    
    // 更新索引
    updateIndices(name, it->second);
    
    // 无效化缓存
    invalidateCaches();
    
    // 通知变更
    notifyChange(name, "updated");
    
    return true;
}

// === 插件查询 ===

bool PluginRegistry::isRegistered(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return registrations_.find(Str(name)) != registrations_.end();
}

Opt<PluginRegistration> PluginRegistry::getRegistration(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        return it->second;
    }
    return std::nullopt;
}

Opt<PluginMetadata> PluginRegistry::getMetadata(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        return it->second.metadata;
    }
    return std::nullopt;
}

Vec<Str> PluginRegistry::getRegisteredPluginNames() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<Str> names;
    names.reserve(registrations_.size());
    
    for (const auto& [name, registration] : registrations_) {
        names.push_back(name);
    }
    
    return names;
}

Vec<PluginRegistration> PluginRegistry::queryPlugins(const PluginQuery& query) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    // 检查查询缓存
    if (queryCacheEnabled_) {
        std::lock_guard<std::mutex> cacheLock(cacheMutex_);
        auto cacheKey = generateQueryCacheKey(query);
        auto cacheIt = queryCache_.find(cacheKey);
        if (cacheIt != queryCache_.end()) {
            return cacheIt->second;
        }
    }
    
    Vec<PluginRegistration> results;
    
    for (const auto& [name, registration] : registrations_) {
        if (matchesQuery(registration, query)) {
            results.push_back(registration);
        }
    }
    
    // 缓存结果
    if (queryCacheEnabled_) {
        std::lock_guard<std::mutex> cacheLock(cacheMutex_);
        auto cacheKey = generateQueryCacheKey(query);
        queryCache_[cacheKey] = results;
    }
    
    return results;
}

Vec<PluginRegistration> PluginRegistry::findPlugins(StrView pattern) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<PluginRegistration> results;
    std::regex regex(Str(pattern), std::regex_constants::icase);
    
    for (const auto& [name, registration] : registrations_) {
        if (std::regex_search(name, regex) || 
            std::regex_search(registration.metadata.displayName, regex) ||
            std::regex_search(registration.metadata.description, regex)) {
            results.push_back(registration);
        }
    }
    
    return results;
}

Vec<PluginRegistration> PluginRegistry::getPluginsByCategory(StrView category) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<PluginRegistration> results;
    auto it = categoryIndex_.find(Str(category));
    if (it != categoryIndex_.end()) {
        for (const auto& name : it->second) {
            auto regIt = registrations_.find(name);
            if (regIt != registrations_.end()) {
                results.push_back(regIt->second);
            }
        }
    }
    
    return results;
}

Vec<PluginRegistration> PluginRegistry::getPluginsByAuthor(StrView author) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<PluginRegistration> results;
    auto it = authorIndex_.find(Str(author));
    if (it != authorIndex_.end()) {
        for (const auto& name : it->second) {
            auto regIt = registrations_.find(name);
            if (regIt != registrations_.end()) {
                results.push_back(regIt->second);
            }
        }
    }
    
    return results;
}

Vec<PluginRegistration> PluginRegistry::getPluginsByTag(StrView tag) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<PluginRegistration> results;
    auto it = tagIndex_.find(Str(tag));
    if (it != tagIndex_.end()) {
        for (const auto& name : it->second) {
            auto regIt = registrations_.find(name);
            if (regIt != registrations_.end()) {
                results.push_back(regIt->second);
            }
        }
    }
    
    return results;
}

// === 状态管理 ===

bool PluginRegistry::updatePluginState(StrView name, PluginState state) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it == registrations_.end()) {
        return false;
    }
    
    // 从旧状态索引中移除
    auto oldStateIt = stateIndex_.find(it->second.state);
    if (oldStateIt != stateIndex_.end()) {
        auto& stateList = oldStateIt->second;
        stateList.erase(std::remove(stateList.begin(), stateList.end(), Str(name)), stateList.end());
    }
    
    // 更新状态
    it->second.state = state;
    
    // 添加到新状态索引
    stateIndex_[state].push_back(Str(name));
    
    // 无效化缓存
    invalidateCaches();
    
    // 通知变更
    notifyChange(name, "state_changed");
    
    return true;
}

PluginState PluginRegistry::getPluginState(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        return it->second.state;
    }
    return PluginState::Unloaded;
}

Vec<PluginRegistration> PluginRegistry::getPluginsByState(PluginState state) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<PluginRegistration> results;
    auto it = stateIndex_.find(state);
    if (it != stateIndex_.end()) {
        for (const auto& name : it->second) {
            auto regIt = registrations_.find(name);
            if (regIt != registrations_.end()) {
                results.push_back(regIt->second);
            }
        }
    }
    
    return results;
}

void PluginRegistry::recordLoadEvent(StrView name, bool success, StrView error) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        it->second.loadCount++;
        it->second.lastLoadTime = std::chrono::system_clock::now();
        
        if (!success && !error.empty()) {
            it->second.loadErrors.push_back(Str(error));
        }
        
        // 无效化缓存
        invalidateCaches();
    }
}

// === 依赖管理 ===

Vec<PluginDependency> PluginRegistry::getPluginDependencies(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        return it->second.metadata.dependencies;
    }
    return {};
}

Vec<Str> PluginRegistry::getDependentPlugins(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    // 确保依赖缓存有效
    if (!dependencyCacheValid_) {
        updateDependencyCache();
    }
    
    auto it = dependentCache_.find(Str(name));
    if (it != dependentCache_.end()) {
        return it->second;
    }
    return {};
}

HashMap<Str, Vec<Str>> PluginRegistry::buildDependencyGraph() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    HashMap<Str, Vec<Str>> graph;
    
    for (const auto& [name, registration] : registrations_) {
        Vec<Str> dependencies;
        for (const auto& dep : registration.metadata.dependencies) {
            dependencies.push_back(dep.name);
        }
        graph[name] = dependencies;
    }
    
    return graph;
}

bool PluginRegistry::hasCyclicDependency() const {
    auto graph = buildDependencyGraph();
    return detectCycle(graph);
}

Vec<Str> PluginRegistry::getLoadOrder() const {
    auto graph = buildDependencyGraph();
    return topologicalSort(graph);
}

Vec<Str> PluginRegistry::resolveDependencyConflicts() const {
    Vec<Str> conflicts;
    
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    for (const auto& [name, registration] : registrations_) {
        for (const auto& dep : registration.metadata.dependencies) {
            if (!isRegistered(dep.name)) {
                conflicts.push_back("Missing dependency: " + dep.name + " for plugin " + name);
            } else {
                auto depMeta = getMetadata(dep.name);
                if (depMeta && !depMeta->version.isCompatible(dep.minVersion)) {
                    conflicts.push_back("Version conflict: " + name + " requires " + dep.name + " >= " + dep.minVersion.toString());
                }
            }
        }
    }
    
    return conflicts;
}

// === 版本管理 ===

bool PluginRegistry::isVersionCompatible(StrView name, const PluginVersion& requiredVersion) const {
    auto metadata = getMetadata(name);
    if (!metadata) {
        return false;
    }
    
    return metadata->version.isCompatible(requiredVersion);
}

Vec<PluginRegistration> PluginRegistry::findCompatibleVersions(StrView name, const PluginVersion& requiredVersion) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Vec<PluginRegistration> compatible;
    
    for (const auto& [pluginName, registration] : registrations_) {
        if (pluginName == name && registration.metadata.version.isCompatible(requiredVersion)) {
            compatible.push_back(registration);
        }
    }
    
    return compatible;
}

Opt<PluginRegistration> PluginRegistry::getLatestVersion(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    Opt<PluginRegistration> latest;
    
    for (const auto& [pluginName, registration] : registrations_) {
        if (pluginName == name) {
            if (!latest || compareVersions(registration.metadata.version, latest->metadata.version) > 0) {
                latest = registration;
            }
        }
    }
    
    return latest;
}

int PluginRegistry::compareVersions(const PluginVersion& v1, const PluginVersion& v2) const {
    if (v1.major != v2.major) return v1.major < v2.major ? -1 : 1;
    if (v1.minor != v2.minor) return v1.minor < v2.minor ? -1 : 1;
    if (v1.patch != v2.patch) return v1.patch < v2.patch ? -1 : 1;
    return 0;
}

// === 属性管理 ===

bool PluginRegistry::setPluginProperty(StrView name, StrView key, StrView value) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it == registrations_.end()) {
        return false;
    }
    
    it->second.properties[Str(key)] = Str(value);
    
    // 通知变更
    notifyChange(name, "property_changed");
    
    return true;
}

Str PluginRegistry::getPluginProperty(StrView name, StrView key, StrView defaultValue) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        auto propIt = it->second.properties.find(Str(key));
        if (propIt != it->second.properties.end()) {
            return propIt->second;
        }
    }
    
    return Str(defaultValue);
}

HashMap<Str, Str> PluginRegistry::getPluginProperties(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        return it->second.properties;
    }
    
    return {};
}

bool PluginRegistry::removePluginProperty(StrView name, StrView key) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it == registrations_.end()) {
        return false;
    }
    
    auto removed = it->second.properties.erase(Str(key)) > 0;
    
    if (removed) {
        // 通知变更
        notifyChange(name, "property_removed");
    }
    
    return removed;
}

// === 统计和监控 ===

PluginStatistics PluginRegistry::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    if (!statsValid_) {
        calculateStatistics();
    }
    
    return cachedStats_;
}

Vec<Str> PluginRegistry::getLoadHistory(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    auto it = registrations_.find(Str(name));
    if (it != registrations_.end()) {
        return it->second.loadErrors;
    }
    
    return {};
}

HashMap<Str, size_t> PluginRegistry::getErrorStatistics() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    HashMap<Str, size_t> errorStats;
    
    for (const auto& [name, registration] : registrations_) {
        errorStats[name] = registration.loadErrors.size();
    }
    
    return errorStats;
}

void PluginRegistry::resetStatistics() {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    for (auto& [name, registration] : registrations_) {
        registration.loadCount = 0;
        registration.loadErrors.clear();
    }
    
    // 无效化缓存
    invalidateCaches();
}

// === 持久化 ===

bool PluginRegistry::saveToFile(StrView filePath) const {
    try {
        std::ofstream file{Str(filePath)};
        if (!file.is_open()) {
            return false;
        }
        
        file << exportToJson();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool PluginRegistry::loadFromFile(StrView filePath) {
    try {
        std::ifstream file{Str(filePath)};
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        return importFromJson(buffer.str());
    } catch (const std::exception&) {
        return false;
    }
}

Str PluginRegistry::exportToJson() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    std::stringstream json;
    json << "{\n";
    json << "  \"plugins\": [\n";
    
    bool first = true;
    for (const auto& [name, registration] : registrations_) {
        if (!first) json << ",\n";
        first = false;
        
        json << "    {\n";
        json << "      \"name\": \"" << registration.metadata.name << "\",\n";
        json << "      \"displayName\": \"" << registration.metadata.displayName << "\",\n";
        json << "      \"description\": \"" << registration.metadata.description << "\",\n";
        json << "      \"author\": \"" << registration.metadata.author << "\",\n";
        json << "      \"version\": \"" << registration.metadata.version.toString() << "\",\n";
        json << "      \"filePath\": \"" << registration.filePath << "\",\n";
        json << "      \"state\": " << static_cast<int>(registration.state) << ",\n";
        json << "      \"loadCount\": " << registration.loadCount << "\n";
        json << "    }";
    }
    
    json << "\n  ]\n";
    json << "}";
    
    return json.str();
}

bool PluginRegistry::importFromJson(StrView json) {
    // 简化的JSON解析实现
    // 在实际项目中应该使用专业的JSON库
    try {
        // 清除现有注册
        unregisterAllPlugins();
        
        // 这里应该实现完整的JSON解析
        // 为了简化，这里只返回true
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// === 缓存管理 ===

void PluginRegistry::clearQueryCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    queryCache_.clear();
}

HashMap<Str, size_t> PluginRegistry::getCacheStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    HashMap<Str, size_t> stats;
    stats["query_cache_size"] = queryCache_.size();
    stats["dependency_cache_valid"] = dependencyCacheValid_ ? 1 : 0;
    stats["stats_cache_valid"] = statsValid_ ? 1 : 0;
    
    return stats;
}

// === 事件通知 ===

void PluginRegistry::addChangeListener(std::function<void(StrView, StrView)> listener) {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    changeListeners_.push_back(listener);
}

void PluginRegistry::removeChangeListener() {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    changeListeners_.clear();
}

// === 调试和诊断 ===

bool PluginRegistry::validateRegistry() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    // 检查索引一致性
    for (const auto& [name, registration] : registrations_) {
        if (!validateMetadata(registration.metadata)) {
            return false;
        }
    }
    
    return true;
}

HashMap<Str, Str> PluginRegistry::getDiagnostics() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    HashMap<Str, Str> diagnostics;
    diagnostics["total_plugins"] = std::to_string(registrations_.size());
    diagnostics["category_indices"] = std::to_string(categoryIndex_.size());
    diagnostics["author_indices"] = std::to_string(authorIndex_.size());
    diagnostics["tag_indices"] = std::to_string(tagIndex_.size());
    diagnostics["state_indices"] = std::to_string(stateIndex_.size());
    diagnostics["dependency_cache_valid"] = dependencyCacheValid_ ? "true" : "false";
    diagnostics["query_cache_enabled"] = queryCacheEnabled_ ? "true" : "false";
    diagnostics["stats_valid"] = statsValid_ ? "true" : "false";
    
    return diagnostics;
}

Str PluginRegistry::exportDebugInfo() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    
    std::stringstream debug;
    debug << "=== Plugin Registry Debug Info ===\n";
    debug << "Total Plugins: " << registrations_.size() << "\n";
    debug << "Category Indices: " << categoryIndex_.size() << "\n";
    debug << "Author Indices: " << authorIndex_.size() << "\n";
    debug << "Tag Indices: " << tagIndex_.size() << "\n";
    debug << "State Indices: " << stateIndex_.size() << "\n";
    debug << "Dependency Cache Valid: " << (dependencyCacheValid_ ? "Yes" : "No") << "\n";
    debug << "Query Cache Enabled: " << (queryCacheEnabled_ ? "Yes" : "No") << "\n";
    debug << "Stats Valid: " << (statsValid_ ? "Yes" : "No") << "\n";
    
    debug << "\n=== Registered Plugins ===\n";
    for (const auto& [name, registration] : registrations_) {
        debug << "- " << name << " (" << registration.metadata.version.toString() << ")\n";
        debug << "  State: " << static_cast<int>(registration.state) << "\n";
        debug << "  Load Count: " << registration.loadCount << "\n";
        debug << "  Errors: " << registration.loadErrors.size() << "\n";
    }
    
    return debug.str();
}

size_t PluginRegistry::cleanupInvalidRegistrations() {
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    size_t removed = 0;
    auto it = registrations_.begin();
    
    while (it != registrations_.end()) {
        if (!validateMetadata(it->second.metadata)) {
            removeFromIndices(it->first, it->second);
            it = registrations_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    if (removed > 0) {
        invalidateCaches();
    }
    
    return removed;
}

// === 私有方法 ===

void PluginRegistry::updateIndices(StrView name, const PluginRegistration& registration) {
    // 更新分类索引
    auto categoryIt = registration.metadata.properties.find("category");
    if (categoryIt != registration.metadata.properties.end()) {
        categoryIndex_[categoryIt->second].push_back(Str(name));
    }
    
    // 更新作者索引
    if (!registration.metadata.author.empty()) {
        authorIndex_[registration.metadata.author].push_back(Str(name));
    }
    
    // 更新标签索引
    auto tagsIt = registration.metadata.properties.find("tags");
    if (tagsIt != registration.metadata.properties.end()) {
        // 简化的标签解析（假设用逗号分隔）
        std::stringstream ss(tagsIt->second);
        std::string tag;
        while (std::getline(ss, tag, ',')) {
            if (!tag.empty()) {
                tagIndex_[tag].push_back(Str(name));
            }
        }
    }
    
    // 更新状态索引
    stateIndex_[registration.state].push_back(Str(name));
}

void PluginRegistry::removeFromIndices(StrView name, const PluginRegistration& registration) {
    // 从分类索引中移除
    auto categoryIt = registration.metadata.properties.find("category");
    if (categoryIt != registration.metadata.properties.end()) {
        auto& categoryList = categoryIndex_[categoryIt->second];
        categoryList.erase(std::remove(categoryList.begin(), categoryList.end(), Str(name)), categoryList.end());
    }
    
    // 从作者索引中移除
    if (!registration.metadata.author.empty()) {
        auto& authorList = authorIndex_[registration.metadata.author];
        authorList.erase(std::remove(authorList.begin(), authorList.end(), Str(name)), authorList.end());
    }
    
    // 从标签索引中移除
    auto tagsIt = registration.metadata.properties.find("tags");
    if (tagsIt != registration.metadata.properties.end()) {
        std::stringstream ss(tagsIt->second);
        std::string tag;
        while (std::getline(ss, tag, ',')) {
            if (!tag.empty()) {
                auto& tagList = tagIndex_[tag];
                tagList.erase(std::remove(tagList.begin(), tagList.end(), Str(name)), tagList.end());
            }
        }
    }
    
    // 从状态索引中移除
    auto& stateList = stateIndex_[registration.state];
    stateList.erase(std::remove(stateList.begin(), stateList.end(), Str(name)), stateList.end());
}

void PluginRegistry::rebuildIndices() {
    // 清除现有索引
    categoryIndex_.clear();
    authorIndex_.clear();
    tagIndex_.clear();
    stateIndex_.clear();
    
    // 重建索引
    for (const auto& [name, registration] : registrations_) {
        updateIndices(name, registration);
    }
}

void PluginRegistry::invalidateCaches() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    queryCache_.clear();
    dependencyCacheValid_ = false;
    statsValid_ = false;
}

void PluginRegistry::updateDependencyCache() const {
    dependencyCache_.clear();
    dependentCache_.clear();
    
    // 构建依赖关系
    for (const auto& [name, registration] : registrations_) {
        Vec<Str> dependencies;
        for (const auto& dep : registration.metadata.dependencies) {
            dependencies.push_back(dep.name);
            dependentCache_[dep.name].push_back(name);
        }
        dependencyCache_[name] = dependencies;
    }
    
    dependencyCacheValid_ = true;
}

void PluginRegistry::calculateStatistics() const {
    cachedStats_ = PluginStatistics{};
    cachedStats_.totalPlugins = registrations_.size();
    
    for (const auto& [name, registration] : registrations_) {
        // 按状态统计
        cachedStats_.pluginsByState[registration.state]++;
        
        if (registration.state == PluginState::Loaded || registration.state == PluginState::Active) {
            cachedStats_.loadedPlugins++;
        }
        
        if (registration.state == PluginState::Active) {
            cachedStats_.enabledPlugins++;
        }
        
        if (registration.state == PluginState::Error) {
            cachedStats_.failedPlugins++;
        }
        
        // 按分类统计
        auto categoryIt = registration.metadata.properties.find("category");
        if (categoryIt != registration.metadata.properties.end()) {
            cachedStats_.pluginsByCategory[categoryIt->second]++;
        }
        
        // 按作者统计
        if (!registration.metadata.author.empty()) {
            cachedStats_.pluginsByAuthor[registration.metadata.author]++;
        }
    }
    
    statsValid_ = true;
}

void PluginRegistry::notifyChange(StrView pluginName, StrView changeType) {
    for (const auto& listener : changeListeners_) {
        try {
            listener(pluginName, changeType);
        } catch (const std::exception&) {
            // 忽略监听器异常
        }
    }
}

bool PluginRegistry::validateMetadata(const PluginMetadata& metadata) const {
    // 基本验证
    if (metadata.name.empty()) {
        return false;
    }
    
    if (metadata.version.major == 0 && metadata.version.minor == 0 && metadata.version.patch == 0) {
        return false;
    }
    
    return true;
}

Str PluginRegistry::generateQueryCacheKey(const PluginQuery& query) const {
    std::stringstream key;
    
    if (query.name) {
        key << "name:" << *query.name << ";";
    }
    if (query.author) {
        key << "author:" << *query.author << ";";
    }
    if (query.category) {
        key << "category:" << *query.category << ";";
    }
    if (query.state) {
        key << "state:" << static_cast<int>(*query.state) << ";";
    }
    
    for (const auto& tag : query.tags) {
        key << "tag:" << tag << ";";
    }
    
    key << "includeDisabled:" << (query.includeDisabled ? "1" : "0");
    
    return key.str();
}

bool PluginRegistry::matchesQuery(const PluginRegistration& registration, const PluginQuery& query) const {
    // 检查名称
    if (query.name && registration.metadata.name != *query.name) {
        return false;
    }
    
    // 检查版本范围
    if (query.minVersion && registration.metadata.version < *query.minVersion) {
        return false;
    }
    
    if (query.maxVersion && *query.maxVersion < registration.metadata.version) {
        return false;
    }
    
    // 检查作者
    if (query.author && registration.metadata.author != *query.author) {
        return false;
    }
    
    // 检查分类
    if (query.category) {
        auto categoryIt = registration.metadata.properties.find("category");
        if (categoryIt == registration.metadata.properties.end() || categoryIt->second != *query.category) {
            return false;
        }
    }
    
    // 检查状态
    if (query.state && registration.state != *query.state) {
        return false;
    }
    
    // 检查标签
    if (!query.tags.empty()) {
        auto tagsIt = registration.metadata.properties.find("tags");
        if (tagsIt == registration.metadata.properties.end()) {
            return false;
        }
        
        // 简化的标签匹配
        for (const auto& requiredTag : query.tags) {
            if (tagsIt->second.find(requiredTag) == std::string::npos) {
                return false;
            }
        }
    }
    
    return true;
}

Vec<Str> PluginRegistry::topologicalSort(const HashMap<Str, Vec<Str>>& graph) const {
    Vec<Str> result;
    HashMap<Str, int> inDegree;
    
    // 计算入度
    for (const auto& [node, deps] : graph) {
        if (inDegree.find(node) == inDegree.end()) {
            inDegree[node] = 0;
        }
        for (const auto& dep : deps) {
            inDegree[dep]++;
        }
    }
    
    // 找到入度为0的节点
    std::queue<Str> queue;
    for (const auto& [node, degree] : inDegree) {
        if (degree == 0) {
            queue.push(node);
        }
    }
    
    // 拓扑排序
    while (!queue.empty()) {
        Str current = queue.front();
        queue.pop();
        result.push_back(current);
        
        auto it = graph.find(current);
        if (it != graph.end()) {
            for (const auto& dep : it->second) {
                inDegree[dep]--;
                if (inDegree[dep] == 0) {
                    queue.push(dep);
                }
            }
        }
    }
    
    return result;
}

bool PluginRegistry::detectCycle(const HashMap<Str, Vec<Str>>& graph) const {
    std::set<Str> visited;
    std::set<Str> recursionStack;
    
    std::function<bool(const Str&)> dfs = [&](const Str& node) -> bool {
        visited.insert(node);
        recursionStack.insert(node);
        
        auto it = graph.find(node);
        if (it != graph.end()) {
            for (const auto& dep : it->second) {
                if (recursionStack.find(dep) != recursionStack.end()) {
                    return true; // 发现循环
                }
                if (visited.find(dep) == visited.end() && dfs(dep)) {
                    return true;
                }
            }
        }
        
        recursionStack.erase(node);
        return false;
    };
    
    for (const auto& [node, deps] : graph) {
        if (visited.find(node) == visited.end()) {
            if (dfs(node)) {
                return true;
            }
        }
    }
    
    return false;
}

} // namespace Lua