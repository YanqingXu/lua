/**
 * @file test_repl_main.cpp
 * @brief Main entry point for REPL compatibility tests
 */

#include "repl_compatibility_test.hpp"
#include <iostream>
#include <exception>

/**
 * @brief Main function for REPL tests
 */
int main() {
    try {
        std::cout << "Lua 5.1 REPL Compatibility Test Suite" << std::endl;
        std::cout << "=====================================" << std::endl << std::endl;

        // Run all REPL compatibility tests
        Lua::Test::REPLCompatibilityTest::runAllTests();

        std::cout << std::endl << "All REPL tests completed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "REPL test failed with error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "REPL test failed with unknown error" << std::endl;
        return 1;
    }
}
