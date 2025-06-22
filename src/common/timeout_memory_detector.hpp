#ifndef TIMEOUT_MEMORY_DETECTOR_HPP
#define TIMEOUT_MEMORY_DETECTOR_HPP

#include "memory_leak_detector.hpp"
#include <thread>
#include <atomic>
#include <future>
#include <signal.h>
#include <setjmp.h>

namespace Lua {
    // 递归深度检测器
    class RecursionDetector {
    private:
        static thread_local i32 recursionDepth_;
        static constexpr i32 MAX_RECURSION_DEPTH = 1000;
        
    public:
        class Guard {
        public:
            Guard() {
                ++recursionDepth_;
                if (recursionDepth_ > MAX_RECURSION_DEPTH) {
                    throw std::runtime_error("Maximum recursion depth exceeded: " + std::to_string(recursionDepth_));
                }
            }
            
            ~Guard() {
                --recursionDepth_;
            }
        };
        
        static i32 getCurrentDepth() { return recursionDepth_; }
        static void reset() { recursionDepth_ = 0; }
    };
    
    // 注意：thread_local静态成员变量的定义应该在对应的.cpp文件中
    
    // 超时检测器
    class TimeoutDetector {
    private:
        Atom<bool> timeoutOccurred_{false};
        Atom<bool> testRunning_{false};
        std::thread timeoutThread_;
        std::chrono::milliseconds timeoutDuration_;
        Str testName_;
        
    public:
        explicit TimeoutDetector(const Str& testName, std::chrono::milliseconds timeout = std::chrono::milliseconds(30000))
            : timeoutDuration_(timeout), testName_(testName) {
            testRunning_ = true;
            timeoutOccurred_ = false;
            
            // 启动超时监控线程
            timeoutThread_ = std::thread([this]() {
                std::this_thread::sleep_for(timeoutDuration_);
                if (testRunning_.load()) {
                    timeoutOccurred_ = true;
                    std::cerr << "\n[TIMEOUT ERROR] Test '" << testName_ << "' exceeded timeout of " 
                              << timeoutDuration_.count() << "ms" << std::endl;
                    std::cerr << "[TIMEOUT ERROR] Possible infinite loop or recursion detected!" << std::endl;
                    std::cerr << "[TIMEOUT ERROR] Current recursion depth: " << RecursionDetector::getCurrentDepth() << std::endl;
                    
                    // 强制终止测试
                    std::abort();
                }
            });
        }
        
        ~TimeoutDetector() {
            testRunning_ = false;
            if (timeoutThread_.joinable()) {
                timeoutThread_.join();
            }
            
            if (!timeoutOccurred_) {
                std::cout << "[TIMEOUT CHECK] Test '" << testName_ << "' completed within timeout" << std::endl;
            }
        }
        
        bool hasTimedOut() const { return timeoutOccurred_.load(); }
    };
    
    // 死循环检测器 - 通过监控程序计数器变化
    class DeadlockDetector {
    private:
        Atom<usize> operationCounter_{0};
        Atom<bool> monitoring_{false};
        std::thread monitorThread_;
        Str testName_;
        static constexpr usize CHECK_INTERVAL_MS = 5000;  // 5秒检查一次
        static constexpr usize MAX_STALL_COUNT = 3;       // 最多允许3次无变化
        
    public:
        explicit DeadlockDetector(const Str& testName) : testName_(testName) {
            monitoring_ = true;
            operationCounter_ = 0;
            
            monitorThread_ = std::thread([this]() {
                usize lastCounter = 0;
                usize stallCount = 0;
                
                while (monitoring_.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
                    
                    if (!monitoring_.load()) break;
                    
                    usize currentCounter = operationCounter_.load();
                    if (currentCounter == lastCounter) {
                        stallCount++;
                        std::cout << "[DEADLOCK WARNING] No progress detected in '" << testName_ 
                                  << "' for " << (stallCount * CHECK_INTERVAL_MS) << "ms" << std::endl;
                        
                        if (stallCount >= MAX_STALL_COUNT) {
                            std::cerr << "\n[DEADLOCK ERROR] Test '" << testName_ 
                                      << "' appears to be in a deadlock or infinite loop!" << std::endl;
                            std::cerr << "[DEADLOCK ERROR] No operations detected for " 
                                      << (stallCount * CHECK_INTERVAL_MS) << "ms" << std::endl;
                            monitoring_ = false;
                            std::abort();
                        }
                    } else {
                        stallCount = 0;  // 重置计数器
                    }
                    
                    lastCounter = currentCounter;
                }
            });
        }
        
        ~DeadlockDetector() {
            monitoring_ = false;
            if (monitorThread_.joinable()) {
                monitorThread_.join();
            }
        }
        
        void recordOperation() {
            operationCounter_.fetch_add(1);
        }
    };
    
    // 综合测试守卫 - 集成内存泄漏、超时、递归和死循环检测
    class ComprehensiveTestGuard {
    private:
        UPtr<MemoryLeakTestGuard> memoryGuard_;
        UPtr<TimeoutDetector> timeoutDetector_;
        UPtr<DeadlockDetector> deadlockDetector_;
        UPtr<RecursionDetector::Guard> recursionGuard_;
        Str testName_;
        std::chrono::steady_clock::time_point startTime_;
        
    public:
        explicit ComprehensiveTestGuard(const Str& testName, 
                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(30000))
            : testName_(testName), startTime_(std::chrono::steady_clock::now()) {
            
            std::cout << "\n[COMPREHENSIVE TEST] Starting: " << testName_ << std::endl;
            std::cout << "[COMPREHENSIVE TEST] Timeout: " << timeout.count() << "ms" << std::endl;
            
            // 初始化各种检测器
            memoryGuard_ = Lua::make_unique<MemoryLeakTestGuard>(testName);
            timeoutDetector_ = Lua::make_unique<TimeoutDetector>(testName, timeout);
            deadlockDetector_ = Lua::make_unique<DeadlockDetector>(testName);
            recursionGuard_ = Lua::make_unique<RecursionDetector::Guard>();
            
            // 重置递归计数器
            RecursionDetector::reset();
        }
        
        ~ComprehensiveTestGuard() {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_).count();
            
            std::cout << "[COMPREHENSIVE TEST] Completed: " << testName_ 
                      << " in " << duration << "ms" << std::endl;
            std::cout << "[COMPREHENSIVE TEST] Max recursion depth reached: " 
                      << RecursionDetector::getCurrentDepth() << std::endl;
            
            // 析构顺序很重要 - 先停止监控，再清理内存检测
            recursionGuard_.reset();
            deadlockDetector_.reset();
            timeoutDetector_.reset();
            memoryGuard_.reset();
        }
        
        // 手动记录操作，用于死循环检测
        void recordOperation() {
            if (deadlockDetector_) {
                deadlockDetector_->recordOperation();
            }
        }
    };
}

// 增强的宏定义
#ifdef _DEBUG
    // 综合测试守卫宏 - 包含所有检测功能
    #define COMPREHENSIVE_TEST_GUARD(testName, timeoutMs) \
        Lua::ComprehensiveTestGuard _comprehensiveGuard(testName, std::chrono::milliseconds(timeoutMs))
    
    // 自动使用函数名的综合测试守卫
    #define AUTO_COMPREHENSIVE_TEST_GUARD(timeoutMs) \
        COMPREHENSIVE_TEST_GUARD(__FUNCTION__, timeoutMs)
    
    // 默认30秒超时的综合测试守卫
    #define AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT() \
        AUTO_COMPREHENSIVE_TEST_GUARD(30000)
    
    // 递归检测守卫
    #define RECURSION_GUARD() \
        Lua::RecursionDetector::Guard _recursionGuard
    
    // 记录操作宏（用于死循环检测）
    #define RECORD_OPERATION() \
        do { \
            if (auto* guard = dynamic_cast<Lua::ComprehensiveTestGuard*>(&_comprehensiveGuard)) { \
                guard->recordOperation(); \
            } \
        } while(0)
    
    // 在循环中使用的操作记录宏
    #define LOOP_OPERATION_RECORD(counter) \
        do { \
            if ((counter) % 1000 == 0) { \
                RECORD_OPERATION(); \
            } \
        } while(0)
        
#else
    // Release模式下禁用所有检测
    #define COMPREHENSIVE_TEST_GUARD(testName, timeoutMs)
    #define AUTO_COMPREHENSIVE_TEST_GUARD(timeoutMs)
    #define AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT()
    #define RECURSION_GUARD()
    #define RECORD_OPERATION()
    #define LOOP_OPERATION_RECORD(counter)
#endif

#endif // TIMEOUT_MEMORY_DETECTOR_HPP