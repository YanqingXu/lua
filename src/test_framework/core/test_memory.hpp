#ifndef LUA_TEST_FRAMEWORK_TEST_MEMORY_HPP
#define LUA_TEST_FRAMEWORK_TEST_MEMORY_HPP

#include "../../common/types.hpp"
#include <memory>
#include <chrono>
#include <thread>
#include <condition_variable>

namespace Lua {
namespace TestFramework {

struct AllocationInfo {
    usize size;
    Str file;
    i32 line;
    std::chrono::steady_clock::time_point timestamp;
};

class MemoryLeakDetector {
public:
    static MemoryLeakDetector& getInstance();
    void recordAllocation(void* ptr, usize size, const Str& file, i32 line);
    void recordDeallocation(void* ptr);
    bool hasLeaks() const;
    void reset();
    Str getLeakReport() const;

private:
    mutable Mtx mutex_;
    HashMap<void*, AllocationInfo> allocations_;
};

class MemoryTestUtils {
public:
    class MemoryGuard {
    public:
        explicit MemoryGuard(const Str& testName);
        ~MemoryGuard();
        bool hasLeak() const;

    private:
        Str testName_;
        bool hasLeak_;
        MemoryGuard(const MemoryGuard&) = delete;
        MemoryGuard& operator=(const MemoryGuard&) = delete;
    };

    static void setTimeout(i32 timeoutMs);
    static void forceGarbageCollection();
    static usize getCurrentMemoryUsage();
    static void setEnabled(bool enabled);
    static bool isEnabled();
    static i32 getTimeoutMs();
    static void setTimeoutMs(i32 timeoutMs);
    static void startMemoryCheck(const Str& testName);
    static bool endMemoryCheck(const Str& testName);
    static void cleanup(); // 清理资源，停止超时监控线程
    static bool hasTimeoutOccurred(); // 检查是否发生过超时

private:
    static i32 timeoutMs_;
    static bool enabled_;
    static HashMap<Str, std::chrono::steady_clock::time_point> testStartTimes_;
    static Mtx testTimesMutex_;
    static Atom<bool> timeoutOccurred_;
    static std::thread timeoutThread_;
    static std::condition_variable timeoutCondition_;
    static Mtx timeoutMutex_;
    static Atom<bool> shouldStopTimeoutThread_;
};

}
}

#endif