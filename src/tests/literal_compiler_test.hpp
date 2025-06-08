#pragma once

#include "../types.hpp"

namespace Lua {
    namespace Tests {
        class LiteralCompilerTest {
        public:
            static void runAllTests();
            
        private:
            // Test methods for different literal types
            static void testNilLiteral();
            static void testBooleanLiterals();
            static void testNumberLiterals();
            static void testStringLiterals();
            static void testComplexLiterals();
            
            // Test constant table management
            static void testLiteralConstantTable();
            
            // Test register allocation
            static void testRegisterAllocation();
            
            // Test instruction generation
            static void testInstructionGeneration();
            
            // Test error handling
            static void testErrorHandling();
        };
    }
}