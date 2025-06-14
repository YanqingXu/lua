#pragma once

#include "../../common/types.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace Lua {
namespace Tests {

    /**
     * @brief String library test class
     * 
     * Tests all Lua string library functions, including:
     * - Basic functions: len, sub, upper, lower, reverse
     * - Pattern matching: find, match, gmatch, gsub
     * - Formatting: format, rep
     * - Character operations: byte, char
     * - Utility functions: trim, split, join, startswith, endswith, contains
     */
    class StringLibTest {
    public:
        /**
         * @brief Run all tests
         * 
         * Execute all test cases in this test class
         */
        static void runAllTests();
        
    private:
        // Basic string function tests
        static void testLen();
        static void testSub();
        static void testUpper();
        static void testLower();
        static void testReverse();
        
        // Pattern matching function tests
        static void testFind();
        static void testMatch();
        static void testGmatch();
        static void testGsub();
        
        // Formatting function tests
        static void testFormat();
        static void testRep();
        
        // Character function tests
        static void testByte();
        static void testChar();
        
        // Utility function tests
        static void testTrim();
        static void testSplit();
        static void testJoin();
        static void testStartswith();
        static void testEndswith();
        static void testContains();
        
        // Edge case and error handling tests
        static void testEdgeCases();
        static void testErrorHandling();
        static void testUnicodeSupport();
        static void testPerformance();
        
        // Helper functions
        static void printTestResult(const Str& testName, bool passed);
        static void printTestHeader(const Str& testName);
        static bool compareStrings(const Str& expected, const Str& actual);
        static bool compareNumbers(f64 expected, f64 actual, f64 epsilon = 1e-9);
        
        // Test data generators
        static Vec<Str> getTestStrings();
        static Vec<Str> getUnicodeTestStrings();
        static Vec<Str> getPatternTestCases();
        
        // Performance test helpers
        static void measureStringOperation(const Str& operationName, 
                                         std::function<void()> operation,
                                         i32 iterations = 1000);
    };
    
    /**
     * @brief String library integration test
     * 
     * Tests string library integration with the Lua state and other components
     */
    class StringLibIntegrationTest {
    public:
        static void runAllTests();
        
    private:
        static void testLibraryRegistration();
        static void testStateIntegration();
        static void testMemoryManagement();
        static void testThreadSafety();
        static void testInteractionWithOtherLibs();
        
        static void printTestResult(const Str& testName, bool passed);
    };
    
    /**
     * @brief String formatter test class
     * 
     * Dedicated tests for the StringFormatter utility class
     */
    class StringFormatterTest {
    public:
        static void runAllTests();
        
    private:
        static void testBasicFormatting();
        static void testNumberFormatting();
        static void testStringFormatting();
        static void testComplexFormatting();
        static void testFormatSpecParsing();
        static void testErrorCases();
        
        static void printTestResult(const Str& testName, bool passed);
    };
}
}