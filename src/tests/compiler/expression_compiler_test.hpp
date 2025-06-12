#pragma once

namespace Lua {
namespace Tests {

    class ExpressionCompilerTest {
    public:
        static void runAllTests();
        
    private:
        static void testLiteralCompilation();
        static void testBinaryExpressionCompilation();
        static void testTableConstructorCompilation();
        static void testConstantFolding();
        static void testUnaryExpressionCompilation();
    };

} // namespace Tests
} // namespace Lua