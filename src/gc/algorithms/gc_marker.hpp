#pragma once

#include "../core/gc_object.hpp"
#include "../utils/gc_types.hpp"
#include "../../common/types.hpp"
#include <stack>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class LuaState;

    /**
     * @brief Tri-color marking algorithm implementation
     * 
     * This class implements the core tri-color marking algorithm used in
     * mark-and-sweep garbage collection. It manages the marking phase of
     * garbage collection by traversing all reachable objects from root set.
     * 
     * The algorithm uses three colors:
     * - White: Objects that may be garbage (not yet visited)
     * - Gray: Objects that are reachable but whose children haven't been scanned
     * - Black: Objects that are reachable and whose children have been scanned
     */
    class GCMarker {
    public:
        /**
         * @brief Construct a new GCMarker
         */
        GCMarker() = default;
        
        /**
         * @brief Destroy the GCMarker
         */
        ~GCMarker() = default;
        
        // Non-copyable and non-movable
        GCMarker(const GCMarker&) = delete;
        GCMarker& operator=(const GCMarker&) = delete;
        GCMarker(GCMarker&&) = delete;
        GCMarker& operator=(GCMarker&&) = delete;
        
        /**
         * @brief Start the marking phase from root objects
         * 
         * @param rootObjects Vector of root objects to start marking from
         * @param currentWhite Current white color for this collection cycle
         */
        void markFromRoots(const Vec<GCObject*>& rootObjects, GCColor currentWhite);
        
        /**
         * @brief Mark a single object as reachable
         * 
         * @param object Object to mark
         * @param currentWhite Current white color for this collection cycle
         */
        void markObject(GCObject* object, GCColor currentWhite);
        
        /**
         * @brief Process all gray objects until none remain
         * 
         * This method processes the gray stack, marking all gray objects
         * as black and adding their children to the gray stack.
         * 
         * @param currentWhite Current white color for this collection cycle
         */
        void processGrayObjects(GCColor currentWhite);
        
        /**
         * @brief Check if marking phase is complete
         * 
         * @return true if no gray objects remain
         */
        bool isMarkingComplete() const;
        
        /**
         * @brief Reset the marker for a new collection cycle
         */
        void reset();
        
        /**
         * @brief Get the number of objects marked in this cycle
         * 
         * @return Number of marked objects
         */
        usize getMarkedObjectCount() const { return markedObjectCount; }
        
        /**
         * @brief Get the current size of gray stack
         *
         * @return Size of gray stack
         */
        usize getGrayStackSize() const { return grayStack.size(); }

        // === Incremental Marking Support ===

        /**
         * @brief 检查是否有灰色对象需要处理
         * @return true 如果有灰色对象
         */
        bool hasGrayObjects() const { return !grayStack.empty(); }

        /**
         * @brief 处理一个灰色对象 - 增量标记支持
         * @param currentWhite 当前白色标记
         * @return 处理的内存量
         */
        isize propagateOne(GCColor currentWhite);

        /**
         * @brief 处理所有灰色对象 - 对应官方propagateall
         * @param currentWhite 当前白色标记
         * @return 处理的内存量
         */
        isize propagateAll(GCColor currentWhite);
        
    private:
        /**
         * @brief Add an object to the gray stack
         * 
         * @param object Object to add to gray stack
         */
        void addToGrayStack(GCObject* object);
        
        /**
         * @brief Mark all children of an object
         * 
         * @param object Parent object whose children to mark
         * @param currentWhite Current white color for this collection cycle
         */
        void markChildren(GCObject* object, GCColor currentWhite);
        
        /**
         * @brief Check if an object is white (unmarked)
         * 
         * @param object Object to check
         * @param currentWhite Current white color for this collection cycle
         * @return true if object is white
         */
        bool isWhite(GCObject* object, GCColor currentWhite) const;
        
        /**
         * @brief Set object color to gray
         * 
         * @param object Object to mark as gray
         */
        void setGray(GCObject* object);
        
        /**
         * @brief Set object color to black
         *
         * @param object Object to mark as black
         */
        void setBlack(GCObject* object);

        /**
         * @brief 计算对象大小 - 用于增量GC的处理量统计
         * @param object 要计算大小的对象
         * @return 对象占用的内存大小
         */
        isize calculateObjectSize(GCObject* object);

    private:
        // Stack of gray objects waiting to be processed
        Vec<GCObject*> grayStack;
        
        // Set of objects already added to gray stack (to avoid duplicates)
        HashSet<GCObject*> graySet;
        
        // Statistics
        usize markedObjectCount = 0;
        
        // Maximum gray stack size reached in this cycle
        usize maxGrayStackSize = 0;
    };
    
} // namespace Lua