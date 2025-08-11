#ifndef LUA_TESTS_MAIN_NEW_HPP
#define LUA_TESTS_MAIN_NEW_HPP

#include <iostream>

namespace Lua {
namespace Tests {

/**
 * @brief Main test class - simplified without test framework
 *
 * Note: Test framework has been removed to streamline the project.
 * Individual tests can be added here as needed without framework dependencies.
 */
class MainTestSuite {
public:
    /**
     * @brief Run all test modules
     *
     * Currently empty since test framework and dependent tests have been removed.
     * Add specific tests here as needed without framework dependencies.
     */
    static void runAllTests() {
        std::cout << "Test framework has been removed to streamline the project.\n";
        std::cout << "Add specific tests here as needed without framework dependencies.\n";
    }
};

} // namespace Tests
} // namespace Lua

#endif // LUA_TESTS_MAIN_NEW_HPP