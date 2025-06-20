#ifndef BASE_LIB_TEST_HPP
#define BASE_LIB_TEST_HPP

#include "../test_utils.hpp"
#include "../../common/types.hpp"
#include <string>

namespace Lua::Tests {

/**
 * @brief Base library test suite
 * 
 * Tests all Lua base library functions organized into logical groups:
 * - Type Conversion: tonumber, tostring, type
 * - Table Access: rawget, rawset, rawlen, rawequal  
 * - Iterator Functions: pairs, ipairs, next
 * - Metatable Functions: getmetatable, setmetatable
 * - Utility Functions: print, select, unpack
 */
class BaseLibTestSuite {
public:
    /**
     * @brief Run all base library tests
     * 
     * Execute all test groups in this test suite
     */
    static void runAllTests();
    
private:
    // Test group methods
    static void runTypeConversionTests();
    static void runTableAccessTests();
    static void runIteratorTests();
    static void runMetatableTests();
    static void runUtilityTests();
    
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
    static void testSelect();
    static void testUnpack();
};

} // namespace Lua::Tests

#endif // BASE_LIB_TEST_HPP