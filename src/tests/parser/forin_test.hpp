#pragma once

namespace Lua {
namespace Tests {

    class ForInTest {
    public:
        static void runAllTests();
        
    private:
        // Test for-in loop syntax parsing
        static void testForInSyntax();
        
        // Test for-in loop execution
        static void testForInExecution();
    };

} // namespace Tests
} // namespace Lua