#pragma once

namespace Lua {
namespace Tests {

    /**
     * @brief Test basic string interning functionality
     * @return true if test passes, false otherwise
     */
    bool testBasicStringInterning();
    
    /**
     * @brief Test memory efficiency of string pool
     * @return true if test passes, false otherwise
     */
    bool testStringPoolMemoryEfficiency();
    
    /**
     * @brief Test string pool performance
     * @return true if test passes, false otherwise
     */
    bool testStringPoolPerformance();
    
    /**
     * @brief Test string pool statistics
     * @return true if test passes, false otherwise
     */
    bool testStringPoolStatistics();
    
    /**
     * @brief Run all string pool demo tests
     * @return true if all tests pass, false otherwise
     */
    bool runStringPoolDemoTests();

} // namespace Tests
} // namespace Lua