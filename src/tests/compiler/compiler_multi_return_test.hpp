#pragma once

namespace Lua {
    /**
     * @brief Multi Return Value Compilation Test Class
     * 
     * Tests compilation of return statements with multiple values including:
     * - Single return value compilation
     * - Multiple return value compilation
     * - Empty return compilation
     * - Complex expression return compilation
     * - Instruction generation verification
     */
    class CompilerMultiReturnTest {
    public:
        /**
         * @brief Run all multi return value compilation tests
         */
        static void runAllTests();

    private:
        /**
         * @brief Test single return value compilation
         */
        static void testSingleReturnCompilation();

        /**
         * @brief Test multiple return value compilation
         */
        static void testMultipleReturnCompilation();

        /**
         * @brief Test empty return compilation
         */
        static void testEmptyReturnCompilation();

        /**
         * @brief Test complex return expressions compilation
         */
        static void testComplexReturnCompilation();

        /**
         * @brief Helper function to test return statement compilation
         * @param code The code to compile
         * @param description Description of the test
         */
        static void testReturnCompilation(const Str& code, const Str& description);
    };
}