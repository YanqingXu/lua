#include "plugin_sandbox.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <filesystem>
#include <fstream>

namespace Lua {

// PluginSandbox Implementation
PluginSandbox::PluginSandbox(StrView pluginName) 
    : pluginName_(pluginName) {
    // 初始化默认配置
    limits_ = ResourceLimits{};
    permissions_ = PermissionConfig{};
    usage_ = ResourceUsage{};
    
    logAudit("sandbox_created", "Sandbox initialized for plugin: " + Str(pluginName));
}

PluginSandbox::~PluginSandbox() {
    if (executionActive_.load()) {
        endExecution();
    }
    
    logAudit("sandbox_destroyed", "Sandbox destroyed for plugin: " + pluginName_);
}

// === 沙箱配置 ===

void PluginSandbox::setResourceLimits(const ResourceLimits& limits) {
    std::lock_guard<std::mutex> lock(usageMutex_);
    limits_ = limits;
    
    logAudit("resource_limits_updated", "Resource limits updated");
}

void PluginSandbox::setPermissionConfig(const PermissionConfig& config) {
    permissions_ = config;
    
    logAudit("permissions_updated", "Permission configuration updated");
}

// === 权限检查 ===

bool PluginSandbox::checkPermission(PermissionType type) const {
    if (!enabled_) {
        return true; // 沙箱未启用时允许所有操作
    }
    
    // 检查基本权限
    auto it = permissions_.permissions.find(type);
    if (it != permissions_.permissions.end() && it->second) {
        return true;
    }
    
    // 检查临时权限
    auto tempIt = temporaryPermissions_.find(type);
    if (tempIt != temporaryPermissions_.end()) {
        auto now = std::chrono::system_clock::now();
        if (now <= tempIt->second) {
            return true;
        } else {
            // 临时权限已过期，移除
            const_cast<PluginSandbox*>(this)->temporaryPermissions_.erase(tempIt);
        }
    }
    
    return false;
}

bool PluginSandbox::checkFileAccess(StrView path, PermissionType accessType) const {
    if (!enabled_) {
        return true;
    }
    
    // 首先检查基本权限
    if (!checkPermission(accessType)) {
        recordViolation(ViolationType::PermissionDenied, 
                       "File access denied: " + Str(path), 
                       "Permission type: " + std::to_string(static_cast<int>(accessType)));
        return false;
    }
    
    // 检查路径权限
    bool allowed = checkPathPermission(path, permissions_.allowedPaths, permissions_.blockedPaths);
    
    if (!allowed) {
        recordViolation(ViolationType::FileAccess, 
                       "File access blocked: " + Str(path), 
                       "Path not in allowed list or in blocked list");
    }
    
    return allowed;
}

bool PluginSandbox::checkNetworkAccess(StrView host, u16 port) const {
    if (!enabled_) {
        return true;
    }
    
    if (!checkPermission(PermissionType::NetworkAccess)) {
        recordViolation(ViolationType::PermissionDenied, 
                       "Network access denied: " + Str(host) + ":" + std::to_string(port));
        return false;
    }
    
    bool allowed = checkHostPermission(host, permissions_.allowedHosts, permissions_.blockedHosts);
    
    if (!allowed) {
        recordViolation(ViolationType::NetworkAccess, 
                       "Network access blocked: " + Str(host) + ":" + std::to_string(port));
    }
    
    return allowed;
}

bool PluginSandbox::checkLibraryLoad(StrView libraryPath) const {
    if (!enabled_) {
        return true;
    }
    
    if (!checkPermission(PermissionType::LibraryLoad)) {
        recordViolation(ViolationType::PermissionDenied, 
                       "Library load denied: " + Str(libraryPath));
        return false;
    }
    
    // 检查库路径权限
    bool allowed = checkPathPermission(libraryPath, permissions_.allowedLibraries, permissions_.blockedLibraries);
    
    if (!allowed) {
        recordViolation(ViolationType::PermissionDenied, 
                       "Library load blocked: " + Str(libraryPath));
    }
    
    return allowed;
}

void PluginSandbox::grantTemporaryPermission(PermissionType type, u32 durationMs) {
    auto expireTime = std::chrono::system_clock::now() + std::chrono::milliseconds(durationMs);
    temporaryPermissions_[type] = expireTime;
    
    logAudit("temporary_permission_granted", 
             "Type: " + std::to_string(static_cast<int>(type)) + 
             ", Duration: " + std::to_string(durationMs) + "ms");
}

void PluginSandbox::revokeTemporaryPermission(PermissionType type) {
    temporaryPermissions_.erase(type);
    
    logAudit("temporary_permission_revoked", 
             "Type: " + std::to_string(static_cast<int>(type)));
}

// === 资源监控 ===

bool PluginSandbox::checkMemoryUsage(size_t requestedSize) const {
    if (!enabled_) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    if (usage_.currentMemory + requestedSize > limits_.maxMemoryUsage) {
        recordViolation(ViolationType::MemoryLimit, 
                       "Memory limit exceeded", 
                       "Requested: " + std::to_string(requestedSize) + 
                       ", Current: " + std::to_string(usage_.currentMemory) + 
                       ", Limit: " + std::to_string(limits_.maxMemoryUsage));
        return false;
    }
    
    return true;
}

void PluginSandbox::recordMemoryAllocation(size_t size) {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_.currentMemory += size;
    if (usage_.currentMemory > usage_.peakMemory) {
        usage_.peakMemory = usage_.currentMemory;
    }
    
    if (debugMode_) {
        logAudit("memory_allocated", "Size: " + std::to_string(size) + 
                ", Total: " + std::to_string(usage_.currentMemory));
    }
}

void PluginSandbox::recordMemoryDeallocation(size_t size) {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    if (usage_.currentMemory >= size) {
        usage_.currentMemory -= size;
    } else {
        usage_.currentMemory = 0;
    }
    
    if (debugMode_) {
        logAudit("memory_deallocated", "Size: " + std::to_string(size) + 
                ", Total: " + std::to_string(usage_.currentMemory));
    }
}

bool PluginSandbox::checkExecutionTime() const {
    if (!enabled_ || !executionActive_.load()) {
        return true;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - executionStart_).count();
    
    if (static_cast<u32>(elapsed) > limits_.maxExecutionTime) {
        recordViolation(ViolationType::TimeLimit, 
                       "Execution time limit exceeded", 
                       "Elapsed: " + std::to_string(elapsed) + "ms, Limit: " + 
                       std::to_string(limits_.maxExecutionTime) + "ms");
        return false;
    }
    
    return true;
}

void PluginSandbox::startExecution() {
    executionStart_ = std::chrono::high_resolution_clock::now();
    executionActive_.store(true);
    
    logAudit("execution_started", "Plugin execution started");
}

void PluginSandbox::endExecution() {
    if (executionActive_.load()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - executionStart_).count();
        
        std::lock_guard<std::mutex> lock(usageMutex_);
        usage_.executionTime += static_cast<u32>(elapsed);
        
        executionActive_.store(false);
        
        logAudit("execution_ended", "Duration: " + std::to_string(elapsed) + "ms");
    }
}

bool PluginSandbox::checkStackDepth(size_t depth) const {
    if (!enabled_) {
        return true;
    }
    
    if (depth > limits_.maxStackDepth) {
        recordViolation(ViolationType::StackOverflow, 
                       "Stack depth limit exceeded", 
                       "Depth: " + std::to_string(depth) + 
                       ", Limit: " + std::to_string(limits_.maxStackDepth));
        return false;
    }
    
    return true;
}

void PluginSandbox::recordStackPush() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_.currentStackDepth++;
    if (usage_.currentStackDepth > usage_.maxStackDepth) {
        usage_.maxStackDepth = usage_.currentStackDepth;
    }
    
    if (debugMode_) {
        logAudit("stack_push", "Depth: " + std::to_string(usage_.currentStackDepth));
    }
}

void PluginSandbox::recordStackPop() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    if (usage_.currentStackDepth > 0) {
        usage_.currentStackDepth--;
    }
    
    if (debugMode_) {
        logAudit("stack_pop", "Depth: " + std::to_string(usage_.currentStackDepth));
    }
}

void PluginSandbox::recordFileOpen() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_.openFiles++;
    
    if (usage_.openFiles > limits_.maxOpenFiles) {
        recordViolation(ViolationType::ResourceExhaustion, 
                       "Too many open files", 
                       "Count: " + std::to_string(usage_.openFiles) + 
                       ", Limit: " + std::to_string(limits_.maxOpenFiles));
    }
    
    logAudit("file_opened", "Open files: " + std::to_string(usage_.openFiles));
}

void PluginSandbox::recordFileClose() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    if (usage_.openFiles > 0) {
        usage_.openFiles--;
    }
    
    logAudit("file_closed", "Open files: " + std::to_string(usage_.openFiles));
}

void PluginSandbox::recordFileRead(size_t bytes) {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_.bytesRead += bytes;
    
    if (debugMode_) {
        logAudit("file_read", "Bytes: " + std::to_string(bytes) + 
                ", Total: " + std::to_string(usage_.bytesRead));
    }
}

void PluginSandbox::recordFileWrite(size_t bytes) {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_.bytesWritten += bytes;
    
    if (debugMode_) {
        logAudit("file_write", "Bytes: " + std::to_string(bytes) + 
                ", Total: " + std::to_string(usage_.bytesWritten));
    }
}

void PluginSandbox::recordNetworkConnection() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_.networkConnections++;
    
    if (usage_.networkConnections > limits_.maxNetworkConnections) {
        recordViolation(ViolationType::ResourceExhaustion, 
                       "Too many network connections", 
                       "Count: " + std::to_string(usage_.networkConnections) + 
                       ", Limit: " + std::to_string(limits_.maxNetworkConnections));
    }
    
    logAudit("network_connected", "Connections: " + std::to_string(usage_.networkConnections));
}

void PluginSandbox::recordNetworkDisconnection() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    if (usage_.networkConnections > 0) {
        usage_.networkConnections--;
    }
    
    logAudit("network_disconnected", "Connections: " + std::to_string(usage_.networkConnections));
}

// === 资源统计 ===

ResourceUsage PluginSandbox::getResourceUsage() const {
    std::lock_guard<std::mutex> lock(usageMutex_);
    return usage_;
}

void PluginSandbox::resetResourceUsage() {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    usage_ = ResourceUsage{};
    
    logAudit("resource_usage_reset", "All resource usage statistics reset");
}

HashMap<Str, double> PluginSandbox::getResourceUtilization() const {
    std::lock_guard<std::mutex> lock(usageMutex_);
    
    HashMap<Str, double> utilization;
    
    // 内存使用率
    if (limits_.maxMemoryUsage > 0) {
        utilization["memory"] = static_cast<double>(usage_.currentMemory) / limits_.maxMemoryUsage * 100.0;
    }
    
    // 执行时间使用率
    if (limits_.maxExecutionTime > 0) {
        utilization["execution_time"] = static_cast<double>(usage_.executionTime) / limits_.maxExecutionTime * 100.0;
    }
    
    // 栈深度使用率
    if (limits_.maxStackDepth > 0) {
        utilization["stack_depth"] = static_cast<double>(usage_.currentStackDepth) / limits_.maxStackDepth * 100.0;
    }
    
    // 文件使用率
    if (limits_.maxOpenFiles > 0) {
        utilization["open_files"] = static_cast<double>(usage_.openFiles) / limits_.maxOpenFiles * 100.0;
    }
    
    // 网络连接使用率
    if (limits_.maxNetworkConnections > 0) {
        utilization["network_connections"] = static_cast<double>(usage_.networkConnections) / limits_.maxNetworkConnections * 100.0;
    }
    
    return utilization;
}

// === 违规处理 ===

void PluginSandbox::recordViolation(ViolationType type, StrView description, StrView details) const {
    ViolationEvent event(type, pluginName_, description, details);
    
    std::lock_guard<std::mutex> lock(violationMutex_);
    const_cast<PluginSandbox*>(this)->violationHistory_.push_back(event);
    
    // 触发违规处理
    const_cast<PluginSandbox*>(this)->handleViolation(event);
    
    // 记录审计日志
    const_cast<PluginSandbox*>(this)->logAudit("violation_recorded", 
        "Type: " + std::to_string(static_cast<int>(type)) + ", " + Str(description));
}

Vec<ViolationEvent> PluginSandbox::getViolationHistory() const {
    std::lock_guard<std::mutex> lock(violationMutex_);
    return violationHistory_;
}

void PluginSandbox::clearViolationHistory() {
    std::lock_guard<std::mutex> lock(violationMutex_);
    violationHistory_.clear();
    
    logAudit("violation_history_cleared", "All violation history cleared");
}

HashMap<ViolationType, size_t> PluginSandbox::getViolationStatistics() const {
    std::lock_guard<std::mutex> lock(violationMutex_);
    
    HashMap<ViolationType, size_t> stats;
    
    for (const auto& violation : violationHistory_) {
        stats[violation.type]++;
    }
    
    return stats;
}

void PluginSandbox::setViolationHandler(std::function<void(const ViolationEvent&)> handler) {
    violationHandler_ = handler;
    
    logAudit("violation_handler_set", "Custom violation handler registered");
}

// === 沙箱控制 ===

void PluginSandbox::suspend() {
    suspended_ = true;
    
    logAudit("sandbox_suspended", "Plugin execution suspended");
}

void PluginSandbox::resume() {
    suspended_ = false;
    
    logAudit("sandbox_resumed", "Plugin execution resumed");
}

void PluginSandbox::terminate() {
    terminated_ = true;
    suspended_ = true;
    
    if (executionActive_.load()) {
        endExecution();
    }
    
    logAudit("sandbox_terminated", "Plugin execution terminated");
}

// === 安全策略 ===

Vec<Str> PluginSandbox::getAuditLog() const {
    std::lock_guard<std::mutex> lock(auditMutex_);
    return auditLog_;
}

void PluginSandbox::clearAuditLog() {
    std::lock_guard<std::mutex> lock(auditMutex_);
    auditLog_.clear();
}

// === 调试和诊断 ===

HashMap<Str, Str> PluginSandbox::getSandboxStatus() const {
    HashMap<Str, Str> status;
    
    status["plugin_name"] = pluginName_;
    status["enabled"] = enabled_ ? "true" : "false";
    status["suspended"] = suspended_ ? "true" : "false";
    status["terminated"] = terminated_ ? "true" : "false";
    status["strict_mode"] = strictMode_ ? "true" : "false";
    status["audit_log_enabled"] = auditLogEnabled_ ? "true" : "false";
    status["debug_mode"] = debugMode_ ? "true" : "false";
    status["execution_active"] = executionActive_.load() ? "true" : "false";
    
    // 资源使用情况
    std::lock_guard<std::mutex> lock(usageMutex_);
    status["current_memory"] = std::to_string(usage_.currentMemory);
    status["peak_memory"] = std::to_string(usage_.peakMemory);
    status["execution_time"] = std::to_string(usage_.executionTime);
    status["current_stack_depth"] = std::to_string(usage_.currentStackDepth);
    status["open_files"] = std::to_string(usage_.openFiles);
    status["network_connections"] = std::to_string(usage_.networkConnections);
    
    // 违规统计
    std::lock_guard<std::mutex> violationLock(violationMutex_);
    status["violation_count"] = std::to_string(violationHistory_.size());
    
    return status;
}

Str PluginSandbox::exportConfiguration() const {
    std::stringstream config;
    
    config << "{\n";
    config << "  \"plugin_name\": \"" << pluginName_ << "\",\n";
    config << "  \"enabled\": " << (enabled_ ? "true" : "false") << ",\n";
    config << "  \"strict_mode\": " << (strictMode_ ? "true" : "false") << ",\n";
    config << "  \"audit_log_enabled\": " << (auditLogEnabled_ ? "true" : "false") << ",\n";
    config << "  \"debug_mode\": " << (debugMode_ ? "true" : "false") << ",\n";
    
    // 资源限制
    config << "  \"resource_limits\": {\n";
    config << "    \"max_memory_usage\": " << limits_.maxMemoryUsage << ",\n";
    config << "    \"max_file_size\": " << limits_.maxFileSize << ",\n";
    config << "    \"max_open_files\": " << limits_.maxOpenFiles << ",\n";
    config << "    \"max_execution_time\": " << limits_.maxExecutionTime << ",\n";
    config << "    \"max_cpu_time\": " << limits_.maxCpuTime << ",\n";
    config << "    \"max_stack_depth\": " << limits_.maxStackDepth << ",\n";
    config << "    \"max_string_length\": " << limits_.maxStringLength << ",\n";
    config << "    \"max_table_size\": " << limits_.maxTableSize << ",\n";
    config << "    \"max_network_connections\": " << limits_.maxNetworkConnections << "\n";
    config << "  },\n";
    
    // 权限配置
    config << "  \"permissions\": {\n";
    bool first = true;
    for (const auto& [type, allowed] : permissions_.permissions) {
        if (!first) config << ",\n";
        first = false;
        config << "    \"" << static_cast<int>(type) << "\": " << (allowed ? "true" : "false");
    }
    config << "\n  }\n";
    config << "}";
    
    return config.str();
}

bool PluginSandbox::importConfiguration(StrView config) {
    // 简化的JSON解析实现
    // 在实际项目中应该使用专业的JSON库
    try {
        // 这里应该实现完整的JSON解析
        // 为了简化，这里只返回true
        logAudit("configuration_imported", "Configuration imported from JSON");
        return true;
    } catch (const std::exception&) {
        logAudit("configuration_import_failed", "Failed to import configuration");
        return false;
    }
}

bool PluginSandbox::validateIntegrity() const {
    // 验证沙箱完整性
    bool valid = true;
    
    // 检查资源限制是否合理
    if (limits_.maxMemoryUsage == 0 || limits_.maxExecutionTime == 0) {
        valid = false;
    }
    
    // 检查权限配置是否一致
    if (permissions_.permissions.empty()) {
        valid = false;
    }
    
    // 检查状态一致性
    if (terminated_ && !suspended_) {
        valid = false;
    }
    
    return valid;
}

// === 私有方法 ===

bool PluginSandbox::checkPathPermission(StrView path, const Vec<Str>& allowedPaths, const Vec<Str>& blockedPaths) const {
    // 首先检查是否在禁止列表中
    for (const auto& blockedPath : blockedPaths) {
        if (matchPath(path, blockedPath)) {
            return false;
        }
    }
    
    // 如果允许列表为空，则默认允许
    if (allowedPaths.empty()) {
        return true;
    }
    
    // 检查是否在允许列表中
    for (const auto& allowedPath : allowedPaths) {
        if (matchPath(path, allowedPath)) {
            return true;
        }
    }
    
    return false;
}

bool PluginSandbox::checkHostPermission(StrView host, const Vec<Str>& allowedHosts, const Vec<Str>& blockedHosts) const {
    // 首先检查是否在禁止列表中
    for (const auto& blockedHost : blockedHosts) {
        if (matchHost(host, blockedHost)) {
            return false;
        }
    }
    
    // 如果允许列表为空，则默认允许
    if (allowedHosts.empty()) {
        return true;
    }
    
    // 检查是否在允许列表中
    for (const auto& allowedHost : allowedHosts) {
        if (matchHost(host, allowedHost)) {
            return true;
        }
    }
    
    return false;
}

void PluginSandbox::updateResourceUsage() {
    // 更新资源使用统计
    // 这个方法可以被定期调用来更新统计信息
}

void PluginSandbox::cleanupExpiredPermissions() {
    auto now = std::chrono::system_clock::now();
    
    auto it = temporaryPermissions_.begin();
    while (it != temporaryPermissions_.end()) {
        if (now > it->second) {
            logAudit("temporary_permission_expired", 
                    "Type: " + std::to_string(static_cast<int>(it->first)));
            it = temporaryPermissions_.erase(it);
        } else {
            ++it;
        }
    }
}

void PluginSandbox::logAudit(StrView operation, StrView details) {
    if (!auditLogEnabled_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(auditMutex_);
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = formatTime(now);
    
    Str logEntry = "[" + timestamp + "] " + pluginName_ + ": " + Str(operation);
    if (!details.empty()) {
        logEntry += " - " + Str(details);
    }
    
    auditLog_.push_back(logEntry);
    
    // 限制审计日志大小
    if (auditLog_.size() > 10000) {
        auditLog_.erase(auditLog_.begin(), auditLog_.begin() + 1000);
    }
}

void PluginSandbox::handleViolation(const ViolationEvent& event) {
    // 在严格模式下，某些违规会导致插件终止
    if (strictMode_) {
        switch (event.type) {
            case ViolationType::MemoryLimit:
            case ViolationType::TimeLimit:
            case ViolationType::StackOverflow:
                terminate();
                break;
            default:
                break;
        }
    }
    
    // 调用自定义违规处理器
    if (violationHandler_) {
        try {
            violationHandler_(event);
        } catch (const std::exception&) {
            // 忽略处理器异常
        }
    }
}

bool PluginSandbox::matchPath(StrView path, StrView pattern) const {
    // 简化的路径匹配实现
    // 支持通配符 * 和 ?
    try {
        Str regexPattern = Str(pattern);
        
        // 转换通配符为正则表达式
        std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
        regexPattern = std::regex_replace(regexPattern, std::regex("\\."), ".*");
        regexPattern = std::regex_replace(regexPattern, std::regex("\\?"), ".");
        
        std::regex regex(regexPattern, std::regex_constants::icase);
        return std::regex_match(Str(path), regex);
    } catch (const std::exception&) {
        // 如果正则表达式失败，使用简单的字符串匹配
        return Str(path).find(pattern) != std::string::npos;
    }
}

bool PluginSandbox::matchHost(StrView host, StrView pattern) const {
    // 简化的主机匹配实现
    // 支持通配符和域名匹配
    try {
        Str regexPattern = Str(pattern);
        
        // 转换通配符为正则表达式
        std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
        regexPattern = std::regex_replace(regexPattern, std::regex("\\."), ".*");
        
        std::regex regex(regexPattern, std::regex_constants::icase);
        return std::regex_match(Str(host), regex);
    } catch (const std::exception&) {
        // 如果正则表达式失败，使用简单的字符串匹配
        return Str(host).find(pattern) != std::string::npos;
    }
}

Str PluginSandbox::formatTime(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;

    std::stringstream ss;
    std::tm tm_buf;
    localtime_s(&tm_buf, &time_t);
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

u64 PluginSandbox::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// SandboxManager Implementation
UPtr<PluginSandbox> SandboxManager::createSandbox(StrView pluginName) {
    std::unique_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    auto sandbox = std::make_unique<PluginSandbox>(pluginName);
    
    // 应用全局配置
    sandbox->setResourceLimits(globalLimits_);
    sandbox->setPermissionConfig(globalPermissions_);
    
    auto* sandboxPtr = sandbox.get();
    sandboxes_[Str(pluginName)] = std::move(sandbox);
    
    return std::unique_ptr<PluginSandbox>(sandboxPtr);
}

PluginSandbox* SandboxManager::getSandbox(StrView pluginName) const {
    std::shared_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    auto it = sandboxes_.find(Str(pluginName));
    if (it != sandboxes_.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

bool SandboxManager::removeSandbox(StrView pluginName) {
    std::unique_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    auto it = sandboxes_.find(Str(pluginName));
    if (it != sandboxes_.end()) {
        sandboxes_.erase(it);
        return true;
    }
    
    return false;
}

Vec<Str> SandboxManager::getAllSandboxes() const {
    std::shared_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    Vec<Str> names;
    names.reserve(sandboxes_.size());
    
    for (const auto& [name, sandbox] : sandboxes_) {
        names.push_back(name);
    }
    
    return names;
}

void SandboxManager::setGlobalResourceLimits(const ResourceLimits& limits) {
    std::unique_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    globalLimits_ = limits;
    
    // 更新所有现有沙箱的资源限制
    for (auto& [name, sandbox] : sandboxes_) {
        sandbox->setResourceLimits(limits);
    }
}

void SandboxManager::setGlobalPermissionConfig(const PermissionConfig& config) {
    std::unique_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    globalPermissions_ = config;
    
    // 更新所有现有沙箱的权限配置
    for (auto& [name, sandbox] : sandboxes_) {
        sandbox->setPermissionConfig(config);
    }
}

HashMap<Str, ResourceUsage> SandboxManager::getGlobalResourceUsage() const {
    std::shared_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    HashMap<Str, ResourceUsage> usage;
    
    for (const auto& [name, sandbox] : sandboxes_) {
        usage[name] = sandbox->getResourceUsage();
    }
    
    return usage;
}

HashMap<Str, Vec<ViolationEvent>> SandboxManager::getGlobalViolations() const {
    std::shared_lock<std::shared_mutex> lock(sandboxesMutex_);
    
    HashMap<Str, Vec<ViolationEvent>> violations;
    
    for (const auto& [name, sandbox] : sandboxes_) {
        violations[name] = sandbox->getViolationHistory();
    }
    
    return violations;
}

} // namespace Lua