#pragma once

#include "../../common/types.hpp"
#include <chrono>

namespace Lua {
    // GC state enumeration - Based on Lua 5.1 GC state machine
    enum class GCState : u8 {
        Pause,      // GC pause state
        Propagate,  // Mark propagation phase
        Sweep,      // Sweep phase
        Finalize,   // Finalizer execution phase
        Atomic      // Atomic mark phase
    };

    // Object color marking - Tri-color marking algorithm
    enum class GCColor : u8 {
        White0 = 0,  // White0 - White color for current collection cycle
        White1 = 1,  // White1 - White color for next collection cycle
        Gray   = 2,  // Gray - Marked but children not marked
        Black  = 3   // Black - Marked and children marked
    };

    // GC mark bit masks
    namespace GCMark {
        constexpr u8 COLOR_MASK = 0x03;     // Color bit mask (bit 0-1)
        constexpr u8 FIXED_MASK = 0x04;     // Fixed object bit (bit 2)
        constexpr u8 FINALIZED_MASK = 0x08; // Finalized bit (bit 3)
        constexpr u8 WEAK_MASK = 0x10;      // Weak reference bit (bit 4)
        constexpr u8 SEPARATED_MASK = 0x20; // Separated bit (bit 5)
        constexpr u8 RESERVED_MASK = 0xC0;  // Reserved bits (bit 6-7)
    }

    // GC object types
    enum class GCObjectType : u8 {
        String,     // String object
        Table,      // Table object
        Function,   // Function object
        Userdata,   // User data
        Thread,     // Coroutine object
        Proto,      // Function prototype
        State       // Lua state object
    };

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

    // Inline utility functions
    namespace GCUtils {
        // Get object color
        inline GCColor getColor(u8 mark) {
            return static_cast<GCColor>(mark & GCMark::COLOR_MASK);
        }
        
        // Set object color
        inline u8 setColor(u8 mark, GCColor color) {
            return (mark & ~GCMark::COLOR_MASK) | static_cast<u8>(color);
        }
        
        // Check if white
        inline bool isWhite(u8 mark) {
            GCColor color = getColor(mark);
            return color == GCColor::White0 || color == GCColor::White1;
        }
        
        // Check if gray
        inline bool isGray(u8 mark) {
            return getColor(mark) == GCColor::Gray;
        }
        
        // Check if black
        inline bool isBlack(u8 mark) {
            return getColor(mark) == GCColor::Black;
        }
        
        // Check if fixed object
        inline bool isFixed(u8 mark) {
            return (mark & GCMark::FIXED_MASK) != 0;
        }
        
        // Set fixed mark
        inline u8 setFixed(u8 mark, bool fixed) {
            return fixed ? (mark | GCMark::FIXED_MASK) : (mark & ~GCMark::FIXED_MASK);
        }
        
        // Check if finalized
        inline bool isFinalized(u8 mark) {
            return (mark & GCMark::FINALIZED_MASK) != 0;
        }
        
        // Set finalized mark
        inline u8 setFinalized(u8 mark, bool finalized) {
            return finalized ? (mark | GCMark::FINALIZED_MASK) : (mark & ~GCMark::FINALIZED_MASK);
        }
        
        // Flip white color
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