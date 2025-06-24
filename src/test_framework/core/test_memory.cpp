#include "test_memory.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Lua {
namespace TestFramework {

// MemoryLeakDetector implementation
MemoryLeakDetector& MemoryLeakDetector::getInstance() {
    static MemoryLeakDetector instance;
    return instance;
}

void MemoryLeakDetector::recordAllocation(void* ptr, usize size, const Str& file, i32 line) {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_[ptr] = {size, file, line, std::chrono::steady_clock::now()};
}

void MemoryLeakDetector::recordDeallocation(void* ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_.erase(ptr);
}

bool MemoryLeakDetector::hasLeaks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !allocations_.empty();
}

void MemoryLeakDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_.clear();
}

Str MemoryLeakDetector::getLeakReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "Memory leak report:\n";
    for (const auto& pair : allocations_) {
        const auto& info = pair.second;
        oss << "  Leaked " << info.size << " bytes at " << info.file << ":" << info.line << "\n";
    }
    return oss.str();
}

// MemoryTestUtils implementation
i32 MemoryTestUtils::timeoutMs_ = 5000;
bool MemoryTestUtils::enabled_ = true;
std::unordered_map<Str, std::chrono::steady_clock::time_point> MemoryTestUtils::testStartTimes_;
std::mutex MemoryTestUtils::testTimesMutex_;
std::atomic<bool> MemoryTestUtils::timeoutOccurred_(false);
std::thread MemoryTestUtils::timeoutThread_;
std::condition_variable MemoryTestUtils::timeoutCondition_;
std::mutex MemoryTestUtils::timeoutMutex_;
std::atomic<bool> MemoryTestUtils::shouldStopTimeoutThread_(false);

MemoryTestUtils::MemoryGuard::MemoryGuard(const Str& testName)
    : testName_(testName), hasLeak_(false) {
    MemoryLeakDetector::getInstance().reset();
}

MemoryTestUtils::MemoryGuard::~MemoryGuard() {
    hasLeak_ = MemoryLeakDetector::getInstance().hasLeaks();
    if (hasLeak_) {
        std::cerr << "Memory leak detected in test: " << testName_ << std::endl;
        std::cerr << MemoryLeakDetector::getInstance().getLeakReport() << std::endl;
    }
}

bool MemoryTestUtils::MemoryGuard::hasLeak() const {
    return hasLeak_;
}

void MemoryTestUtils::setTimeout(i32 timeoutMs) {
    timeoutMs_ = timeoutMs;
}

void MemoryTestUtils::forceGarbageCollection() {
    // Implementation for garbage collection if needed
}

usize MemoryTestUtils::getCurrentMemoryUsage() {
    // Implementation for getting current memory usage
    return 0;
}

void MemoryTestUtils::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool MemoryTestUtils::isEnabled() {
    return enabled_;
}

i32 MemoryTestUtils::getTimeoutMs() {
    return timeoutMs_;
}

void MemoryTestUtils::setTimeoutMs(i32 timeoutMs) {
    timeoutMs_ = timeoutMs;
}

void MemoryTestUtils::startMemoryCheck(const Str& testName) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(testTimesMutex_);
    testStartTimes_[testName] = std::chrono::steady_clock::now();
    
    // 启动超时监控线程（如果还没有启动）
    if (!timeoutThread_.joinable()) {
        shouldStopTimeoutThread_ = false;
        timeoutThread_ = std::thread([]() {
            while (!shouldStopTimeoutThread_) {
                std::unique_lock<std::mutex> lock(timeoutMutex_);
                timeoutCondition_.wait_for(lock, std::chrono::milliseconds(100));
                
                if (shouldStopTimeoutThread_) break;
                
                // 检查所有正在运行的测试是否超时
                std::lock_guard<std::mutex> timesLock(testTimesMutex_);
                auto now = std::chrono::steady_clock::now();
                
                static std::unordered_set<Str> reportedTimeouts;
                
                for (const auto& pair : testStartTimes_) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - pair.second).count();
                    if (elapsed > timeoutMs_ && reportedTimeouts.find(pair.first) == reportedTimeouts.end()) {
                        timeoutOccurred_ = true;
                        reportedTimeouts.insert(pair.first);
                        std::cerr << "Test timeout detected: " << pair.first 
                                  << " exceeded " << timeoutMs_ << "ms limit" << std::endl;
                    }
                }
                
                // 清理已完成测试的超时报告记录
                for (auto it = reportedTimeouts.begin(); it != reportedTimeouts.end();) {
                    if (testStartTimes_.find(*it) == testStartTimes_.end()) {
                        it = reportedTimeouts.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        });
    }
}

bool MemoryTestUtils::endMemoryCheck(const Str& testName) {
    if (!enabled_) return false;
    
    bool hasTimeout = false;
    
    {
        std::lock_guard<std::mutex> lock(testTimesMutex_);
        auto it = testStartTimes_.find(testName);
        if (it != testStartTimes_.end()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - it->second).count();
            
            if (elapsed > timeoutMs_) {
                hasTimeout = true;
                std::cerr << "Test " << testName << " completed but exceeded timeout: "
                          << elapsed << "ms > " << timeoutMs_ << "ms" << std::endl;
            }
            
            testStartTimes_.erase(it);
        }
    }
    
    // 如果没有更多测试在运行，停止超时监控线程
    {
        std::lock_guard<std::mutex> lock(testTimesMutex_);
        if (testStartTimes_.empty()) {
            shouldStopTimeoutThread_ = true;
            timeoutCondition_.notify_all();
            if (timeoutThread_.joinable()) {
                timeoutThread_.join();
            }
        }
    }
    
    return hasTimeout || timeoutOccurred_.load();
}

void MemoryTestUtils::cleanup() {
    // 停止超时监控线程
    shouldStopTimeoutThread_ = true;
    timeoutCondition_.notify_all();
    
    if (timeoutThread_.joinable()) {
        timeoutThread_.join();
    }
    
    // 清理测试时间记录
    std::lock_guard<std::mutex> lock(testTimesMutex_);
    testStartTimes_.clear();
    timeoutOccurred_ = false;
}

bool MemoryTestUtils::hasTimeoutOccurred() {
    return timeoutOccurred_.load();
}

}
}