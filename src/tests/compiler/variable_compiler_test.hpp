#pragma once

namespace Lua {
    namespace Tests {
        class VariableCompilerTest {
        public:
            static void runAllTests();
            
        private:
            static void testLocalVariableAccess();
            static void testGlobalVariableAccess();
            static void testVariableResolution();
            static void testScopeHandling();
            static void testRegisterAllocation();
            static void testInstructionGeneration();
            static void testErrorHandling();
        };
    }
}