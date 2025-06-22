#ifndef LUA_TEST_FRAMEWORK_TEST_MEMORY_HPP
#define LUA_TEST_FRAMEWORK_TEST_MEMORY_HPP

#include "../../common/types.hpp"
#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <thread>

namespace Lua {
namespace TestFramework {

struct AllocationInfo {
    size_t size;
    std::string file;
    int line;
    std::chrono::steady_clock::time_point timestamp;
};

class MemoryLeakDetector {
public:
    static MemoryLeakDetector& getInstance();
    void recordAllocation(void* ptr, size_t size, const std::string& file, int line);
    void recordDeallocation(void* ptr);
    bool hasLeaks() const;
    void reset();
    std::string getLeakReport() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
};

class MemoryTestUtils {
public:
    class MemoryGuard {
    public:
        explicit MemoryGuard(const std::string& testName);
        ~MemoryGuard();
        bool hasLeak() const;

    private:
        std::string testName_;
        bool hasLeak_;
        MemoryGuard(const MemoryGuard&) = delete;
        MemoryGuard& operator=(const MemoryGuard&) = delete;
    };

    static void setTimeout(i32 timeoutMs);
    static void forceGarbageCollection();
    static size_t getCurrentMemoryUsage();
    static void setEnabled(bool enabled);
    static bool isEnabled();
    static int getTimeoutMs();
    static void setTimeoutMs(int timeoutMs);
    static void startMemoryCheck(const std::string& testName);
    static bool endMemoryCheck(const std::string& testName);

private:
    static int timeoutMs_;
    static bool enabled_;
};

}
}

#endif