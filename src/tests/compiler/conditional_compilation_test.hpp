#pragma once

namespace Lua {
namespace Tests {

    /**
     * Test class for conditional statement compilation functionality
     * Tests if-then-else statements, short-circuit logical operators, and nested conditions
     */
    class ConditionalCompilationTest {
    public:
        /**
         * Run all conditional compilation tests
         */
        static void runAllTests();
        
    private:
        /**
         * Test simple if statement compilation
         */
        static void testSimpleIfStatement();
        
        /**
         * Test if-else statement compilation
         */
        static void testIfElseStatement();
        
        /**
         * Test nested if statement compilation
         */
        static void testNestedIfStatement();
        
        /**
         * Test short-circuit AND operator compilation
         */
        static void testShortCircuitAnd();
        
        /**
         * Test short-circuit OR operator compilation
         */
        static void testShortCircuitOr();
        
        /**
         * Test complex conditional expressions
         */
        static void testComplexConditions();
    };

} // namespace Tests
} // namespace Lua