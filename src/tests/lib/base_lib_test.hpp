#ifndef BASE_LIB_TEST_HPP
#define BASE_LIB_TEST_HPP

#include "../../common/types.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

    /**
     * @brief Base library test class
     * 
     * Tests all Lua base library functions, including:
     * - print: Print function
     * - tonumber: Convert to number
     * - tostring: Convert to string
     * - type: Get type
     * - ipairs: Array iterator
     * - pairs: Table iterator
     * - next: Next key-value pair
     * - getmetatable: Get metatable
     * - setmetatable: Set metatable
     * - rawget: Raw get
     * - rawset: Raw set
     * - rawlen: Raw length
     * - rawequal: Raw equal
     * - pcall: Protected call
     * - xpcall: Extended protected call
     * - error: Throw error
     * - assert: Assertion
     * - select: Select arguments
     * - unpack: Unpack table
     */
    class BaseLibTest {
    public:
        /**
         * @brief Run all tests
         * 
         * Execute all test cases in this test class
         */
        static void runAllTests();
        
    private:
        // Individual test methods
        static void testPrint();
        static void testTonumber();
        static void testTostring();
        static void testType();
        static void testIpairs();
        static void testPairs();
        static void testNext();
        static void testGetmetatable();
        static void testSetmetatable();
        static void testRawget();
        static void testRawset();
        static void testRawlen();
        static void testRawequal();
        static void testPcall();
        static void testXpcall();
        static void testError();
        static void testAssert();
        static void testSelect();
        static void testUnpack();
        
        // Helper methods
        static void printTestResult(const std::string& testName, bool passed);
    };

} // namespace Tests
} // namespace Lua

#endif // BASE_LIB_TEST_HPP