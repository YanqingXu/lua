#ifndef TABLE_LIB_TEST_HPP
#define TABLE_LIB_TEST_HPP

#include "../../common/types.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

    /**
     * @brief Table library test class
     * 
     * Tests all Lua table library functions, including:
     * - table.concat: Concatenate table elements
     * - table.insert: Insert element into table
     * - table.remove: Remove element from table
     * - table.sort: Sort table elements
     * - table.pack: Pack arguments into table
     * - table.unpack: Unpack table elements
     * - table.move: Move table elements
     */
    class TableLibTest {
    public:
        /**
         * @brief Run all tests
         * 
         * Execute all test cases in this test class
         */
        void runAllTests();
        
    private:
        // Individual test methods
        void testConcat();
        void testInsert();
        void testRemove();
        void testSort();
        void testPack();
        void testUnpack();
        void testMove();
        
        // Error handling tests
        void testErrorHandling();
        
        // Helper methods
        void printTestResult(const std::string& testName, bool passed);
        void printSectionHeader(const std::string& sectionName);
        void printSectionFooter();
    };

} // namespace Tests
} // namespace Lua

#endif // TABLE_LIB_TEST_HPP