#pragma once

#include "../../common/types.hpp"
#include <chrono>

namespace Lua {
    // GC state enumeration - Lua 5.1 compatible 5-state machine
    enum class GCState : u8 {
        Pause = 0,        // GCSpause - 暂停状态，等待下次GC周期
        Propagate = 1,    // GCSpropagate - 标记传播阶段
        SweepString = 2,  // GCSsweepstring - 清理字符串表阶段
        Sweep = 3,        // GCSsweep - 清理普通对象阶段
        Finalize = 4      // GCSfinalize - 终结化阶段，处理finalizer
    };

    // Object color marking - Tri-color marking algorithm
    enum class GCColor : u8 {
        White0 = 0,  // White0 - White color for current collection cycle
        White1 = 1,  // White1 - White color for next collection cycle
        Gray   = 2,  // Gray - Marked but children not marked
        Black  = 3   // Black - Marked and children marked
    };

    // GC mark bit masks - Lua 5.1 compatible bit layout
    namespace GCMark {
        // Lua 5.1 official bit layout for 'marked' field
        constexpr u8 WHITE0BIT = 0;         // bit 0 - object is white (type 0)
        constexpr u8 WHITE1BIT = 1;         // bit 1 - object is white (type 1)
        constexpr u8 BLACKBIT = 2;          // bit 2 - object is black
        constexpr u8 FINALIZEDBIT = 3;      // bit 3 - for userdata: has been finalized
        constexpr u8 KEYWEAKBIT = 3;        // bit 3 - for tables: has weak keys
        constexpr u8 VALUEWEAKBIT = 4;      // bit 4 - for tables: has weak values
        constexpr u8 FIXEDBIT = 5;          // bit 5 - object is fixed (should not be collected)
        constexpr u8 SFIXEDBIT = 6;         // bit 6 - object is "super" fixed (only the main thread)

        // Bit masks
        constexpr u8 WHITE0 = (1 << WHITE0BIT);
        constexpr u8 WHITE1 = (1 << WHITE1BIT);
        constexpr u8 WHITEBITS = (WHITE0 | WHITE1);
        constexpr u8 BLACK = (1 << BLACKBIT);
        constexpr u8 FINALIZED = (1 << FINALIZEDBIT);
        constexpr u8 KEYWEAK = (1 << KEYWEAKBIT);
        constexpr u8 VALUEWEAK = (1 << VALUEWEAKBIT);
        constexpr u8 FIXED = (1 << FIXEDBIT);
        constexpr u8 SFIXED = (1 << SFIXEDBIT);

        // Combined masks
        constexpr u8 MASKMARKS = static_cast<u8>(~(BLACK | WHITEBITS));

        // Lua 5.1 compatible bit manipulation macros (as constexpr functions)
        constexpr u8 bitmask(u8 b) { return static_cast<u8>(1 << b); }
        constexpr u8 bit2mask(u8 b1, u8 b2) { return static_cast<u8>(bitmask(b1) | bitmask(b2)); }

        // Modern C++ bit manipulation functions
        constexpr void resetbits(u8& x, u8 m) { x &= static_cast<u8>(~m); }
        constexpr void setbits(u8& x, u8 m) { x |= m; }
        constexpr bool testbits(u8 x, u8 m) { return (x & m) != 0; }
        constexpr void l_setbit(u8& x, u8 b) { setbits(x, bitmask(b)); }
        constexpr void resetbit(u8& x, u8 b) { resetbits(x, bitmask(b)); }
        constexpr bool testbit(u8 x, u8 b) { return testbits(x, bitmask(b)); }
        constexpr void set2bits(u8& x, u8 b1, u8 b2) { setbits(x, bit2mask(b1, b2)); }
        constexpr void reset2bits(u8& x, u8 b1, u8 b2) { resetbits(x, bit2mask(b1, b2)); }
        constexpr bool test2bits(u8 x, u8 b1, u8 b2) { return testbits(x, bit2mask(b1, b2)); }
    }

    // GC object types - Lua 5.1 compatible
    enum class GCObjectType : u8 {
        String,     // LUA_TSTRING - String object
        Table,      // LUA_TTABLE - Table object
        Function,   // LUA_TFUNCTION - Function object (closure)
        Userdata,   // LUA_TUSERDATA - User data
        Thread,     // LUA_TTHREAD - Coroutine object
        Proto,      // Function prototype (internal)
        State,      // Lua state object (internal)
        Upvalue     // LUA_TUPVAL - Upvalue object
    };

    // Forward declarations for GC utility functions
    class GCObject;
    class GarbageCollector;

    /**
     * @brief Lua 5.1 compatible GC utility functions
     *
     * These functions provide the same interface as official Lua 5.1 GC operations
     * while using modern C++ features for type safety and performance.
     */
    namespace GCUtils {
        // Color checking functions (Lua 5.1 compatible) - declarations only
        bool iswhite(const GCObject* o);
        bool isblack(const GCObject* o);
        bool isgray(const GCObject* o);

        // Color manipulation functions
        void white2gray(GCObject* o);
        void gray2black(GCObject* o);
        void black2gray(GCObject* o);

        // Object state checking
        bool isfinalized(const GCObject* o);
        void markfinalized(GCObject* o);
        bool isfixed(const GCObject* o);
        void setfixed(GCObject* o);
        void unsetfixed(GCObject* o);

        // Weak table checking
        bool hasweakkeys(const GCObject* o);
        bool hasweakvalues(const GCObject* o);
        void setweakkeys(GCObject* o);
        void setweakvalues(GCObject* o);

        // Functions that need GarbageCollector implementation
        bool isdead(const GarbageCollector* g, const GCObject* o);
        void makewhite(GarbageCollector* g, GCObject* o);
        u8 luaC_white(const GarbageCollector* g);
        u8 otherwhite(const GarbageCollector* g);
    }

    // GC configuration parameters
    struct GCConfig {
        // Memory threshold configuration
        usize initialThreshold = 1024 * 1024;  // Initial GC threshold (1MB)
        usize maxThreshold = 64 * 1024 * 1024;  // Maximum GC threshold (64MB)
        double growthFactor = 2.0;               // Threshold growth factor
        
        // Incremental GC configuration
        usize stepSize = 1024;                   // Objects processed per step
        u32 stepTimeMs = 5;                      // Maximum time per step (milliseconds)
        double pauseMultiplier = 200.0;          // Pause multiplier (percentage)

        // Lua 5.1 compatible GC parameters
        i32 gcpause = 200;                       // GC暂停参数(百分比) - 对应官方gcpause
        i32 gcstepmul = 200;                     // GC步长倍数 - 对应官方gcstepmul
        usize gcstepsize = 1024;                 // 每步处理大小 - 对应官方GCSTEPSIZE
        
        // Generational GC configuration (optional)
        bool enableGenerational = false;         // Enable generational collection
        usize youngGenThreshold = 256 * 1024;   // Young generation threshold
        u32 youngGenRatio = 20;                  // Young generation collection ratio
        
        // Debugging and monitoring
        bool enableStats = true;                 // Enable statistics
        bool enableLogging = false;              // Enable logging
        u32 logLevel = 1;                        // Log level (0-3)
    };

    // GC statistics information
    struct GCStats {
        // Memory statistics
        usize totalAllocated = 0;        // Total allocated memory
        usize totalFreed = 0;            // Total freed memory
        usize currentUsage = 0;          // Current memory usage
        usize peakUsage = 0;             // Peak memory usage
        
        // Object statistics
        usize totalObjects = 0;          // Total object count
        usize liveObjects = 0;           // Live object count
        usize collectedObjects = 0;      // Collected object count
        
        // GC execution statistics
        u64 gcCycles = 0;                // GC cycle count
        u64 totalGCTime = 0;             // Total GC time (microseconds)
        u64 maxPauseTime = 0;            // Maximum pause time (microseconds)
        u64 avgPauseTime = 0;            // Average pause time (microseconds)
        
        // Generational statistics (if enabled)
        u64 youngGenCollections = 0;     // Young generation collection count
        u64 oldGenCollections = 0;       // Old generation collection count
        
        // Timestamp
        std::chrono::steady_clock::time_point lastGCTime;
        
        // Reset statistics
        void reset() {
            totalAllocated = totalFreed = currentUsage = peakUsage = 0;
            totalObjects = liveObjects = collectedObjects = 0;
            gcCycles = totalGCTime = maxPauseTime = avgPauseTime = 0;
            youngGenCollections = oldGenCollections = 0;
            lastGCTime = std::chrono::steady_clock::now();
        }
        
        // Update peak usage
        void updatePeakUsage() {
            if (currentUsage > peakUsage) {
                peakUsage = currentUsage;
            }
        }
        
        // Record GC time
        void recordGCTime(u64 timeUs) {
            gcCycles++;
            totalGCTime += timeUs;
            if (timeUs > maxPauseTime) {
                maxPauseTime = timeUs;
            }
            avgPauseTime = totalGCTime / gcCycles;
            lastGCTime = std::chrono::steady_clock::now();
        }
    };

    // Weak reference types
    enum class WeakType : u8 {
        None = 0,    // Not weak reference
        Key = 1,     // Weak key
        Value = 2,   // Weak value
        Both = 3     // Weak key-value
    };

    // Finalizer states
    enum class FinalizerState : u8 {
        None,        // No finalizer
        Pending,     // Pending execution
        Running,     // Currently running
        Completed    // Completed
    };

    // GC phase time measurement
    class GCTimer {
    private:
        std::chrono::steady_clock::time_point startTime;
        bool running = false;
        
    public:
        void start() {
            startTime = std::chrono::steady_clock::now();
            running = true;
        }
        
        u64 stop() {
            if (!running) return 0;
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            running = false;
            return static_cast<u64>(duration.count());
        }
        
        bool isRunning() const { return running; }
    };

    // Inline utility functions for backward compatibility
    namespace GCUtilsCompat {
        // Check if white (using new bit layout)
        inline bool isWhite(u8 mark) {
            return GCMark::testbits(mark, GCMark::WHITEBITS);
        }

        // Check if gray (using new bit layout)
        inline bool isGray(u8 mark) {
            return !GCMark::testbits(mark, GCMark::WHITEBITS) && !GCMark::testbit(mark, GCMark::BLACKBIT);
        }

        // Check if black (using new bit layout)
        inline bool isBlack(u8 mark) {
            return GCMark::testbit(mark, GCMark::BLACKBIT);
        }

        // Check if fixed object (using new bit layout)
        inline bool isFixed(u8 mark) {
            return GCMark::testbit(mark, GCMark::FIXEDBIT);
        }

        // Set fixed mark (using new bit layout)
        inline u8 setFixed(u8 mark, bool fixed) {
            if (fixed) {
                GCMark::l_setbit(mark, GCMark::FIXEDBIT);
            } else {
                GCMark::resetbit(mark, GCMark::FIXEDBIT);
            }
            return mark;
        }

        // Check if finalized (using new bit layout)
        inline bool isFinalized(u8 mark) {
            return GCMark::testbit(mark, GCMark::FINALIZEDBIT);
        }

        // Set finalized mark (using new bit layout)
        inline u8 setFinalized(u8 mark, bool finalized) {
            if (finalized) {
                GCMark::l_setbit(mark, GCMark::FINALIZEDBIT);
            } else {
                GCMark::resetbit(mark, GCMark::FINALIZEDBIT);
            }
            return mark;
        }

        // Flip white color (using new bit layout)
        inline GCColor flipWhite(GCColor currentWhite) {
            return (currentWhite == GCColor::White0) ? GCColor::White1 : GCColor::White0;
        }
    }

    // Constant definitions
    namespace GCConstants {
        constexpr usize DEFAULT_STEP_SIZE = 1024;
        constexpr u32 DEFAULT_STEP_TIME_MS = 5;
        constexpr double DEFAULT_PAUSE_MULTIPLIER = 200.0;
        constexpr usize MIN_THRESHOLD = 64 * 1024;      // 64KB
        constexpr usize MAX_THRESHOLD = 1024 * 1024 * 1024; // 1GB
        constexpr double MIN_GROWTH_FACTOR = 1.1;
        constexpr double MAX_GROWTH_FACTOR = 4.0;
    }
}