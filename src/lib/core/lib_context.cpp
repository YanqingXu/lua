#include "lib_context.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace Lua {
    namespace Lib {

        // === Copy constructor and assignment operator ===

        LibContext::LibContext(const LibContext& other) {
            copyFrom(other);
        }

        LibContext& LibContext::operator=(const LibContext& other) {
            if (this != &other) {
                copyFrom(other);
            }
            return *this;
        }

        void LibContext::copyFrom(const LibContext& other) {
            std::unique_lock<std::shared_mutex> thisLock(mutex_, std::defer_lock);
            std::shared_lock<std::shared_mutex> otherLock(other.mutex_, std::defer_lock);
            std::lock(thisLock, otherLock);

            config_ = other.config_;
            dependencies_ = other.dependencies_;
            environment_ = other.environment_;
            logLevel_ = other.logLevel_;
            detailedLogging_ = other.detailedLogging_;
            maxCacheSize_ = other.maxCacheSize_;
            loadTimeout_ = other.loadTimeout_;
            asyncLoading_ = other.asyncLoading_;
            safeMode_ = other.safeMode_;
            sandboxLevel_ = other.sandboxLevel_;
            trustedPaths_ = other.trustedPaths_;
            performanceMonitoring_ = other.performanceMonitoring_;
        }

        // === Configuration management ===

        bool LibContext::hasConfig(StrView key) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return config_.find(Str(key)) != config_.end();
        }

        void LibContext::removeConfig(StrView key) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            config_.erase(Str(key));
        }

        void LibContext::clearConfig() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            config_.clear();
        }

        // === Batch configuration ===

        void LibContext::setConfigFromFile(StrView filename) {
            std::ifstream file{Str(filename)};
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open config file: " + Str(filename));
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            setConfigFromString(buffer.str());
        }

        void LibContext::setConfigFromString(StrView configStr) {
            // Simple configuration parsing (format: key=value, one per line)
            std::istringstream stream{Str(configStr)};
            Str line;
            
            while (std::getline(stream, line)) {
                // Skip comments and empty lines
                if (line.empty() || line[0] == '#' || line[0] == ';') {
                    continue;
                }

                size_t pos = line.find('=');
                if (pos != Str::npos) {
                    Str key = line.substr(0, pos);
                    Str value = line.substr(pos + 1);
                    
                    // Remove leading and trailing whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    // Try to parse different types
                    if (value == "true" || value == "false") {
                        setConfig(key, value == "true");
                    } else if (std::all_of(value.begin(), value.end(), 
                                         [](char c) { return std::isdigit(c) || c == '-'; })) {
                        setConfig(key, std::stoi(value));
                    } else if (value.find('.') != Str::npos &&
                               std::all_of(value.begin(), value.end(),
                                         [](char c) { return std::isdigit(c) || c == '.' || c == '-'; })) {
                        setConfig(key, std::stod(value));
                    } else {
                        setConfig(key, value);
                    }
                }
            }
        }

        void LibContext::mergeConfig(const LibContext& other) {
            std::unique_lock<std::shared_mutex> thisLock(mutex_, std::defer_lock);
            std::shared_lock<std::shared_mutex> otherLock(other.mutex_, std::defer_lock);
            std::lock(thisLock, otherLock);

            for (const auto& [key, value] : other.config_) {
                config_[key] = value;
            }
        }

        // === Environment variables ===

        void LibContext::setEnvironment(StrView key, StrView value) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            environment_[Str(key)] = Str(value);
        }

        std::optional<Str> LibContext::getEnvironment(StrView key) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = environment_.find(Str(key));
            return it != environment_.end() ? std::make_optional(it->second) : std::nullopt;
        }

        std::unordered_map<Str, Str> LibContext::getAllEnvironment() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return environment_;
        }

        // === Logging configuration ===

        void LibContext::setLogLevel(LogLevel level) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            logLevel_ = level;
        }

        LogLevel LibContext::getLogLevel() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return logLevel_;
        }

        void LibContext::enableDetailedLogging(bool enable) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            detailedLogging_ = enable;
        }

        bool LibContext::isDetailedLoggingEnabled() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return detailedLogging_;
        }

        // === Performance configuration ===

        void LibContext::setMaxCacheSize(size_t size) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            maxCacheSize_ = size;
        }

        size_t LibContext::getMaxCacheSize() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return maxCacheSize_;
        }

        void LibContext::setLoadTimeout(std::chrono::milliseconds timeout) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            loadTimeout_ = timeout;
        }

        std::chrono::milliseconds LibContext::getLoadTimeout() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return loadTimeout_;
        }

        void LibContext::enableAsyncLoading(bool enable) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            asyncLoading_ = enable;
        }

        bool LibContext::isAsyncLoadingEnabled() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return asyncLoading_;
        }

        // === Security configuration ===

        void LibContext::enableSafeMode(bool enable) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            safeMode_ = enable;
        }

        bool LibContext::isSafeModeEnabled() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return safeMode_;
        }

        void LibContext::setSandboxLevel(SandboxLevel level) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            sandboxLevel_ = level;
        }

        SandboxLevel LibContext::getSandboxLevel() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return sandboxLevel_;
        }

        void LibContext::addTrustedPath(StrView path) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            trustedPaths_.emplace_back(path);
        }

        std::vector<Str> LibContext::getTrustedPaths() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return trustedPaths_;
        }

        bool LibContext::isPathTrusted(StrView path) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            Str pathStr(path);
            return std::any_of(trustedPaths_.begin(), trustedPaths_.end(),
                              [&pathStr](const Str& trustedPath) {
                                  return pathStr.find(trustedPath) == 0;
                              });
        }

        // === Debugging and statistics ===

        void LibContext::enablePerformanceMonitoring(bool enable) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            performanceMonitoring_ = enable;
        }

        bool LibContext::isPerformanceMonitoringEnabled() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return performanceMonitoring_;
        }

        LibContext::ContextStats LibContext::getStats() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            ContextStats stats;
            stats.configCount = config_.size();
            stats.dependencyCount = dependencies_.size();
            stats.environmentCount = environment_.size();
            stats.trustedPathCount = trustedPaths_.size();
            return stats;
        }

    } // namespace Lib
} // namespace Lua
