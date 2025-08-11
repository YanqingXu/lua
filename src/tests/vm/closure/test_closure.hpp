#ifndef LUA_TEST_CLOSURE_HPP
#define LUA_TEST_CLOSURE_HPP

#include <iostream>
#include <string>



namespace Lua {
namespace Tests {

/**
 * Main Closure Test Suite
 * 
 * This class serves as the main entry point for all closure-related tests.
 * It coordinates and runs various specialized test suites for different
 * aspects of closure functionality.
 */
class ClosureTestSuite {
public:
    /**
     * @brief Run all closure tests
     *
     * Note: Test framework removed - tests disabled
     */
    static void runAllTests() {
        std::cout << "Closure tests disabled - test framework removed\n";

        // Individual test methods can be called directly if needed:
        // ClosureBasicTest::runAllTests();
        // ClosureAdvancedTest::runAllTests();
        // ClosureMemoryTest::runAllTests();
        // ClosurePerformanceTest::runAllTests();
        // ClosureErrorTest::runAllTests();
        // ClosureBoundaryTest::runAllTests();
    }
};

} // namespace Tests
} // namespace Lua

#endif // LUA_TEST_CLOSURE_HPP