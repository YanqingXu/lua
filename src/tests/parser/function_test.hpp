#pragma once

namespace Lua {
namespace Tests {

    class FunctionTest {
    public:
        static void runAllTests();
        
    private:
        // Test function definition syntax parsing
        static void testFunctionSyntax();
        
        // Test function parameter parsing
        static void testFunctionParameters();
        
        // Test function body parsing
        static void testFunctionBody();
        
        // Test function compilation
        static void testFunctionCompilation();
    };

} // namespace Tests
} // namespace Lua