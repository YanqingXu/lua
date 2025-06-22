#include "memory_leak_detector.hpp"

namespace Lua {
    // 静态成员变量定义
    HashMap<void*, AllocationInfo> MemoryLeakDetector::allocations_;
    std::mutex MemoryLeakDetector::mutex_;
    usize MemoryLeakDetector::totalAllocated_ = 0;
    usize MemoryLeakDetector::peakAllocated_ = 0;
    bool MemoryLeakDetector::isEnabled_ = false;
}