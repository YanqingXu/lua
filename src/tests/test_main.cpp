#include "test_main.hpp"
#include "parser/source_location_test.hpp"
#include <iostream>

namespace Lua {
namespace Tests {
/**
 * @brief Run all Lua interpreter tests
 * 
 * This function executes all the test cases defined in the Lua interpreter test suite.
 * It handles exceptions and reports the results of each test.
 */
void runAllTests() {
    std::cout << "=== Running Lua Interpreter Tests ===" << std::endl;
      try {
        // Run lexer tests
        // LexerTest::runAllTests();
        
        // Run all VM tests (unified)
        // VMTestSuite::runAllTests();
        
        // Run all parser tests (unified)
        // ParserTestSuite::runAllTests();
        
        // Run SourceLocation tests only
        std::cout << "Running SourceLocation tests..." << std::endl;
        SourceLocationTest::runAllTests();
        
        // Run all compiler tests (unified)
        // CompilerTest::runAllTests();
        
        // Run all GC tests (unified)
        // GCTest::runAllTests();
        
        // Run all lib tests (unified)
        // LibTestSuite::runAllTests();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\nTest execution failed with exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "\nTest execution failed with unknown exception" << std::endl;
    }
}

} // namespace Tests
} // namespace Lua

//int main() {
//    Lua::Tests::runAllTests();
//    return 0;
//}