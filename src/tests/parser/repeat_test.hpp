#pragma once

namespace Lua {
namespace Tests {

    class RepeatTest {
    public:
        static void runAllTests();
        
    private:
        // Test repeat-until loop syntax parsing
        static void testRepeatUntilSyntax();
        
        // Test repeat-until loop execution
        static void testRepeatUntilExecution();
    };

} // namespace Tests
} // namespace Lua