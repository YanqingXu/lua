#pragma once

#include "../../common/types.hpp"
#include "../../lib/lib_define.hpp"
#include "../../lib/lib_module.hpp"
#include "../../lib/lib_context.hpp"
#include "../../lib/lib_func_registry.hpp"
#include "../../lib/lib_manager.hpp"
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>

namespace Lua {
    namespace Tests {

        /**
         * Modern comprehensive test framework for the standard library
         * Following development standards with English comments and type safety
         */
        class ComprehensiveTestSuite {
        public:
            /**
             * Run all test suites
             */
            static void runAllTests();
            
            /**
             * Test core framework components
             */
            static void testCoreFramework();
            
            /**
             * Test standard library modules
             */
            static void testStandardLibraries();
            
            /**
             * Test performance benchmarks
             */
            static void testPerformance();
            
            /**
             * Test thread safety
             */
            static void testThreadSafety();
            
            /**
             * Test integration scenarios
             */
            static void testIntegration();
            
        private:
            /**
             * Test LibContext functionality
             */
            static void testLibContext();
            
            /**
             * Test LibFuncRegistry functionality
             */
            static void testLibFuncRegistry();
            
            /**
             * Test LibraryManager functionality
             */
            static void testLibraryManager();
            
            /**
             * Test BaseLib implementation
             */
            static void testBaseLib();
            
            /**
             * Test StringLib implementation
             */
            static void testStringLib();
            
            /**
             * Test MathLib implementation
             */
            static void testMathLib();
            
            /**
             * Performance benchmark helper
             */
            template<typename Func>
            static std::chrono::milliseconds benchmarkFunction(const Str& name, Func&& func, i32 iterations = 1000);
            
            /**
             * Thread safety test helper
             */
            template<typename TestFunc>
            static bool testConcurrentAccess(const Str& testName, TestFunc&& testFunc, i32 threadCount = 4, i32 iterationsPerThread = 100);
            
            /**
             * Test result tracking
             */
            static void reportTestResult(const Str& testName, bool passed, const Str& details = "");
            
            static i32 totalTests_;
            static i32 passedTests_;
            static std::vector<Str> failedTests_;
        };

        /**
         * Utility macros for testing
         */
        #define TEST_ASSERT(condition, message) \
            do { \
                if (!(condition)) { \
                    ComprehensiveTestSuite::reportTestResult(__func__, false, message); \
                    return; \
                } \
            } while(0)

        #define TEST_ASSERT_EQ(expected, actual, message) \
            do { \
                if ((expected) != (actual)) { \
                    Str details = Str(message) + " (expected: " + std::to_string(expected) + ", actual: " + std::to_string(actual) + ")"; \
                    ComprehensiveTestSuite::reportTestResult(__func__, false, details); \
                    return; \
                } \
            } while(0)

        #define TEST_ASSERT_TRUE(condition, message) TEST_ASSERT(condition, message)
        #define TEST_ASSERT_FALSE(condition, message) TEST_ASSERT(!(condition), message)

    } // namespace Tests
} // namespace Lua
