#pragma once

namespace Lua::Tests {

/**
 * @brief String pool demo test suite
 * 
 * Demonstrates string interning, memory efficiency, and performance
 */
class GCStringPoolTestSuite {
public:
    /**
     * @brief Run all string pool demo tests
     */
    static void runAllTests();
        
    private:
        /**
         * @brief Test basic string interning functionality
         * @return true if test passes, false otherwise
         */
        static bool testBasicStringInterning();
        
        /**
         * @brief Test memory efficiency of string pool
         * @return true if test passes, false otherwise
         */
        static bool testStringPoolMemoryEfficiency();
        
        /**
         * @brief Test string pool performance
         * @return true if test passes, false otherwise
         */
        static bool testStringPoolPerformance();
        
        /**
         * @brief Test string pool statistics
         * @return true if test passes, false otherwise
         */
        static bool testStringPoolStatistics();
    };

} // namespace Lua::Tests