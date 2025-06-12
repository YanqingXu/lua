#ifndef IF_STATEMENT_TEST_HPP
#define IF_STATEMENT_TEST_HPP

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

#endif // IF_STATEMENT_TEST_HPP