#ifndef MEMORY_LEAK_DETECTOR_HPP
#define MEMORY_LEAK_DETECTOR_HPP

#include "types.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>

namespace Lua {
    struct AllocationInfo {
        usize size;
        Str file;
        i32 line;
        Str function;
        std::chrono::steady_clock::time_point timestamp;
        void* stackTrace[10];  // 简化的调用栈
        i32 stackDepth;
        
        // 默认构造函数
        AllocationInfo() 
            : size(0), file(""), line(0), function("")
            , timestamp(std::chrono::steady_clock::now()), stackDepth(0) {
            memset(stackTrace, 0, sizeof(stackTrace));
        }
        
        AllocationInfo(usize sz, const char* f, i32 l, const char* func)
            : size(sz), file(f), line(l), function(func)
            , timestamp(std::chrono::steady_clock::now()), stackDepth(0) {
            // 这里可以添加实际的栈回溯代码
            memset(stackTrace, 0, sizeof(stackTrace));
        }
    };
    
    class MemoryLeakDetector {
    private:
        static HashMap<void*, AllocationInfo> allocations_;
        static std::mutex mutex_;
        static usize totalAllocated_;
        static usize peakAllocated_;
        static bool isEnabled_;
        
    public:
        static void enable() { isEnabled_ = true; }
        static void disable() { isEnabled_ = false; }
        
        static void* trackAllocation(usize size, const char* file, i32 line, const char* function) {
            if (!isEnabled_) return malloc(size);
            
            void* ptr = malloc(size);
            if (ptr) {
                std::lock_guard<std::mutex> lock(mutex_);
                allocations_[ptr] = AllocationInfo(size, file, line, function);
                totalAllocated_ += size;
                if (totalAllocated_ > peakAllocated_) {
                    peakAllocated_ = totalAllocated_;
                }
            }
            return ptr;
        }
        
        static void trackDeallocation(void* ptr, const char* file, i32 line) {
            if (!isEnabled_ || !ptr) {
                free(ptr);
                return;
            }
            
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                totalAllocated_ -= it->second.size;
                allocations_.erase(it);
            }
            free(ptr);
        }
        
        static Vec<AllocationInfo> getLeaks() {
            std::lock_guard<std::mutex> lock(mutex_);
            Vec<AllocationInfo> leaks;
            for (const auto& pair : allocations_) {
                leaks.push_back(pair.second);
            }
            return leaks;
        }
        
        static usize getCurrentAllocated() {
            std::lock_guard<std::mutex> lock(mutex_);
            return totalAllocated_;
        }
        
        static usize getPeakAllocated() {
            std::lock_guard<std::mutex> lock(mutex_);
            return peakAllocated_;
        }
        
        static void reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            allocations_.clear();
            totalAllocated_ = 0;
            peakAllocated_ = 0;
        }
        
        static Str generateLeakReport() {
            auto leaks = getLeaks();
            if (leaks.empty()) {
                return "No memory leaks detected.";
            }
            
            std::ostringstream report;
            report << "\n=== MEMORY LEAK REPORT ===\n";
            report << "Total leaks: " << leaks.size() << "\n";
            
            usize totalLeakedBytes = 0;
            HashMap<Str, Vec<AllocationInfo>> leaksByLocation;
            
            for (const auto& leak : leaks) {
                totalLeakedBytes += leak.size;
                Str location = leak.file + ":" + std::to_string(leak.line) + " in " + leak.function;
                leaksByLocation[location].push_back(leak);
            }
            
            report << "Total leaked bytes: " << totalLeakedBytes << "\n\n";
            
            // 按位置分组显示泄漏
            for (const auto& group : leaksByLocation) {
                const auto& location = group.first;
                const auto& locationLeaks = group.second;
                
                usize locationTotal = 0;
                for (const auto& leak : locationLeaks) {
                    locationTotal += leak.size;
                }
                
                report << "Location: " << location << "\n";
                report << "  Count: " << locationLeaks.size() << " allocations\n";
                report << "  Total: " << locationTotal << " bytes\n";
                
                // 显示前几个具体的泄漏
                i32 showCount = std::min(3, static_cast<i32>(locationLeaks.size()));
                for (i32 i = 0; i < showCount; ++i) {
                    const auto& leak = locationLeaks[i];
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - leak.timestamp
                    ).count();
                    report << "    - " << leak.size << " bytes (alive for " << duration << "ms)\n";
                }
                
                if (locationLeaks.size() > 3) {
                    report << "    ... and " << (locationLeaks.size() - 3) << " more\n";
                }
                report << "\n";
            }
            
            return report.str();
        }
    };
    
    // 注意：静态成员变量的定义应该在对应的.cpp文件中
    // 这里只保留声明，避免重复定义链接错误
    
    // RAII包装器用于自动管理检测生命周期
    class MemoryLeakTestGuard {
    private:
        Str testName_;
        usize initialAllocated_;
        std::chrono::steady_clock::time_point startTime_;
        
    public:
        explicit MemoryLeakTestGuard(const Str& testName)
            : testName_(testName), startTime_(std::chrono::steady_clock::now()) {
            MemoryLeakDetector::enable();
            MemoryLeakDetector::reset();
            initialAllocated_ = MemoryLeakDetector::getCurrentAllocated();
            
            std::cout << "[MEMORY TEST] Starting: " << testName_ << std::endl;
        }
        
        ~MemoryLeakTestGuard() {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_).count();
            
            usize finalAllocated = MemoryLeakDetector::getCurrentAllocated();
            usize peakAllocated = MemoryLeakDetector::getPeakAllocated();
            
            std::cout << "[MEMORY TEST] Finished: " << testName_ << " (" << duration << "ms)" << std::endl;
            std::cout << "[MEMORY TEST] Peak usage: " << peakAllocated << " bytes" << std::endl;
            
            if (finalAllocated > initialAllocated_) {
                std::cout << "[MEMORY LEAK] Detected in test: " << testName_ << std::endl;
                std::cout << "[MEMORY LEAK] Leaked: " << (finalAllocated - initialAllocated_) << " bytes" << std::endl;
                std::cout << MemoryLeakDetector::generateLeakReport() << std::endl;
            } else {
                std::cout << "[MEMORY TEST] No leaks detected in: " << testName_ << std::endl;
            }
            
            MemoryLeakDetector::disable();
        }
    };
}

// 重定义内存分配宏以支持跟踪
#ifdef _DEBUG
    #define LEAK_TRACKED_MALLOC(size) Lua::MemoryLeakDetector::trackAllocation(size, __FILE__, __LINE__, __FUNCTION__)
    #define LEAK_TRACKED_FREE(ptr) Lua::MemoryLeakDetector::trackDeallocation(ptr, __FILE__, __LINE__)
    
    // 重载new和delete操作符（可选）
    #define LEAK_TRACKED_NEW(type) new(Lua::MemoryLeakDetector::trackAllocation(sizeof(type), __FILE__, __LINE__, __FUNCTION__)) type
#else
    #define LEAK_TRACKED_MALLOC(size) malloc(size)
    #define LEAK_TRACKED_FREE(ptr) free(ptr)
    #define LEAK_TRACKED_NEW(type) new type
#endif

// 主要的宏函数 - 在测试函数入口处使用
#define MEMORY_LEAK_TEST_GUARD(testName) \
    Lua::MemoryLeakTestGuard _memoryGuard(testName); \
    std::cout << "[TEST START] " << testName << " - Memory leak detection enabled" << std::endl

// 简化版本 - 自动使用函数名作为测试名
#define AUTO_MEMORY_LEAK_TEST_GUARD() \
    MEMORY_LEAK_TEST_GUARD(__FUNCTION__)

// 手动检查点宏
#define MEMORY_CHECKPOINT(description) \
    do { \
        usize current = Lua::MemoryLeakDetector::getCurrentAllocated(); \
        std::cout << "[MEMORY CHECKPOINT] " << description << ": " << current << " bytes allocated" << std::endl; \
    } while(0)

// 断言没有内存泄漏的宏
#define ASSERT_NO_MEMORY_LEAKS() \
    do { \
        auto leaks = Lua::MemoryLeakDetector::getLeaks(); \
        if (!leaks.empty()) { \
            std::cerr << "ASSERTION FAILED: Memory leaks detected!" << std::endl; \
            std::cerr << Lua::MemoryLeakDetector::generateLeakReport() << std::endl; \
            std::abort(); \
        } \
    } while(0)

#endif // MEMORY_LEAK_DETECTOR_HPP