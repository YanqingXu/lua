#pragma once

#include "../common/types.hpp"
#include "lua_state.hpp"

namespace Lua {
    
    // Forward declaration
    struct CallInfo;
    class LuaState;
    
    /**
     * @brief Call stack management class
     * 
     * This class provides optimized management of the CallInfo stack,
     * following Lua 5.1 design patterns with performance enhancements.
     * It handles dynamic resizing, debugging support, and memory optimization.
     */
    class CallStack {
    public:
        /**
         * @brief Construct a new Call Stack object
         * @param L Lua state to operate on
         * @param initialSize Initial size of CallInfo array
         */
        explicit CallStack(LuaState* L, i32 initialSize = 8);
        
        /**
         * @brief Destroy the Call Stack object
         */
        ~CallStack();
        
        // Stack operations
        /**
         * @brief Push a new call frame onto the stack
         * @return CallInfo* Pointer to the new call frame
         * @throws LuaException if stack overflow or allocation failure
         */
        CallInfo* push();
        
        /**
         * @brief Pop the current call frame from the stack
         * @throws LuaException if stack underflow
         */
        void pop();
        
        /**
         * @brief Get the current call frame
         * @return CallInfo* Pointer to current call frame
         */
        CallInfo* getCurrent() const { return current_; }
        
        /**
         * @brief Get the base call frame
         * @return CallInfo* Pointer to base call frame
         */
        CallInfo* getBase() const { return base_; }
        
        /**
         * @brief Get call frame at specific level
         * @param level Level from current (0 = current, 1 = caller, etc.)
         * @return CallInfo* Pointer to call frame or nullptr if invalid
         */
        CallInfo* getFrame(i32 level) const;
        
        // Stack information
        /**
         * @brief Get current call depth
         * @return i32 Number of active call frames
         */
        i32 getDepth() const;
        
        /**
         * @brief Check if stack is empty (only base frame)
         * @return bool True if only base frame exists
         */
        bool isEmpty() const { return current_ == base_; }
        
        /**
         * @brief Check if stack is at capacity
         * @return bool True if stack needs to grow
         */
        bool isFull() const { return current_ + 1 >= end_; }
        
        /**
         * @brief Get current stack size
         * @return i32 Total number of CallInfo slots
         */
        i32 getSize() const { return size_; }
        
        /**
         * @brief Get stack utilization ratio
         * @return f64 Ratio of used slots to total slots
         */
        f64 getUtilization() const;
        
        // Memory management
        /**
         * @brief Resize the call stack
         * @param newSize New size for the CallInfo array
         * @throws LuaException if allocation failure
         */
        void resize(i32 newSize);
        
        /**
         * @brief Shrink stack to optimal size
         * Reduces memory usage when stack is underutilized
         */
        void shrink();
        
        /**
         * @brief Ensure stack has capacity for additional frames
         * @param additionalFrames Number of additional frames needed
         */
        void ensureCapacity(i32 additionalFrames = 1);
        
        // Validation and debugging
        /**
         * @brief Validate stack consistency
         * @return bool True if stack is in valid state
         */
        bool validate() const;
        
        /**
         * @brief Dump stack contents for debugging
         * @param maxFrames Maximum number of frames to dump
         */
        void dumpStack(i32 maxFrames = 10) const;
        
        /**
         * @brief Print stack statistics
         */
        void printStatistics() const;
        
        // Iterator support for debugging
        /**
         * @brief Get iterator to first call frame
         * @return CallInfo* Iterator to first frame
         */
        CallInfo* begin() const { return base_; }
        
        /**
         * @brief Get iterator past last call frame
         * @return CallInfo* Iterator past last frame
         */
        CallInfo* end() const { return current_ + 1; }
        
        // Performance optimization
        /**
         * @brief Reset stack to initial state
         * Keeps allocated memory but resets to base frame
         */
        void reset();
        
        /**
         * @brief Get memory usage in bytes
         * @return usize Total memory used by call stack
         */
        usize getMemoryUsage() const;
        
        // Constants
        static constexpr i32 MIN_STACK_SIZE = 4;
        static constexpr i32 MAX_STACK_SIZE = 1000;
        static constexpr i32 DEFAULT_STACK_SIZE = 8;
        static constexpr f64 SHRINK_THRESHOLD = 0.25;  // Shrink when utilization < 25%
        static constexpr f64 GROW_FACTOR = 1.5;        // Grow by 50%
        
    private:
        LuaState* L_;                    // Lua state reference
        CallInfo* base_;                 // CallInfo array base
        CallInfo* end_;                  // Array end position
        CallInfo* current_;              // Current CallInfo
        i32 size_;                       // Array size
        
        // Statistics (for optimization)
        mutable i32 maxDepthReached_;    // Maximum depth reached
        mutable i32 resizeCount_;        // Number of resizes
        
        // Helper methods
        /**
         * @brief Allocate new CallInfo array
         * @param size Size of array to allocate
         * @return CallInfo* Pointer to allocated array
         */
        CallInfo* allocateArray_(i32 size);
        
        /**
         * @brief Deallocate CallInfo array
         * @param array Array to deallocate
         */
        void deallocateArray_(CallInfo* array);
        
        /**
         * @brief Copy CallInfo data during resize
         * @param dest Destination array
         * @param src Source array
         * @param count Number of elements to copy
         */
        void copyCallInfos_(CallInfo* dest, const CallInfo* src, i32 count);
        
        /**
         * @brief Update pointers after resize
         * @param oldBase Old base pointer
         * @param newBase New base pointer
         */
        void updatePointers_(CallInfo* oldBase, CallInfo* newBase);
        
        /**
         * @brief Calculate optimal size for given depth
         * @param depth Required depth
         * @return i32 Optimal size
         */
        i32 calculateOptimalSize_(i32 depth) const;
        
        // Prevent copying
        CallStack(const CallStack&) = delete;
        CallStack& operator=(const CallStack&) = delete;
    };
    
} // namespace Lua
