#include "test_memory.hpp"
#include <iostream>
#include <sstream>

namespace Lua {
namespace TestFramework {

// MemoryLeakDetector implementation
MemoryLeakDetector& MemoryLeakDetector::getInstance() {
    static MemoryLeakDetector instance;
    return instance;
}

void MemoryLeakDetector::recordAllocation(void* ptr, size_t size, const std::string& file, int line) {
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

std::string MemoryLeakDetector::getLeakReport() const {
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
int MemoryTestUtils::timeoutMs_ = 5000;
bool MemoryTestUtils::enabled_ = true;

MemoryTestUtils::MemoryGuard::MemoryGuard(const std::string& testName)
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

size_t MemoryTestUtils::getCurrentMemoryUsage() {
    // Implementation for getting current memory usage
    return 0;
}

void MemoryTestUtils::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool MemoryTestUtils::isEnabled() {
    return enabled_;
}

int MemoryTestUtils::getTimeoutMs() {
    return timeoutMs_;
}

void MemoryTestUtils::setTimeoutMs(int timeoutMs) {
    timeoutMs_ = timeoutMs;
}

void MemoryTestUtils::startMemoryCheck(const std::string& testName) {
    // 开始内存检测的实现
    // 可以在这里记录初始内存状态
}

bool MemoryTestUtils::endMemoryCheck(const std::string& testName) {
    // 结束内存检测的实现
    // 检查是否有内存泄漏
    return false; // 暂时返回false表示没有检测到内存泄漏
}

}
}