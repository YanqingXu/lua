#pragma once

#include "../../common/types.hpp"
#include <memory>

namespace Lua {
    // Forward declarations
    class State;
    class Table;
    class GarbageCollector;
}

namespace Lua::Tests {

/**
 * @brief Garbage collector test suite
 * 
 * This class tests the newly implemented GarbageCollector::collectGarbage()
 * method and related functionality including:
 * - Basic GC creation and state management
 * - Full garbage collection cycles
 * - Memory allocation/deallocation tracking
 * - GC triggering logic
 * - Object registration and lifecycle management
 */
class GCBasicTestSuite {
    public:
        /**
         * @brief Run all garbage collector implementation tests
         */
        static void runAllTests();
        
    private:
        /**
         * @brief Test basic GC functionality and initialization
         */
        static void testBasicGCFunctionality();
        
        /**
         * @brief Test complete garbage collection cycle
         */
        static void testGCCollectionCycle();
        
        /**
         * @brief Test memory management and tracking
         */
        static void testMemoryManagement();
        
        /**
         * @brief Test GC triggering conditions
         */
        static void testGCTriggering();
    };

} // namespace Lua::Tests
