#pragma once

namespace Lua {
namespace Tests {

    /**
     * Test class for string pool demo functionality
     * Demonstrates string interning, memory efficiency, and performance
     */
    class StringPoolDemoTest {
    public:
        /**
         * Run all string pool demo tests
         */
        static bool runAllTests();
        
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

} // namespace Tests
} // namespace Lua