#pragma once

namespace Lua {
    class IfStatementTest {
    public:
        static void runAllTests();
        
    private:
        static void testSimpleIfStatement();
        static void testIfElseStatement();
        static void testNestedIfStatement();
        static void testIfWithComplexCondition();
        static void testIfStatementExecution();
    };
}