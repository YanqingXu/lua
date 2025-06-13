#include "test_lib.hpp"
#include "table_lib_test.hpp"
#include "../../lib/lib_init.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void LibTestSuite::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "        STANDARD LIBRARY TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all standard library tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        // 1. Table Library Tests
        printSectionHeader("Table Library Tests");
        TableLibTest::runAllTests();
        printSectionFooter();
        
        // TODO: Add other library tests when implemented
        // 2. String Library Tests
        // printSectionHeader("String Library Tests");
        // StringLibTest::runAllTests();
        // printSectionFooter();
        
        // 3. Math Library Tests  
        // printSectionHeader("Math Library Tests");
        // MathLibTest::runAllTests();
        // printSectionFooter();
        
        // 总结
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL LIBRARY TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] LIBRARY TESTS FAILED" << std::endl;
        std::cout << "    Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    } catch (...) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] LIBRARY TESTS FAILED" << std::endl;
        std::cout << "    Unknown error occurred" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    }
}

void LibTestSuite::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "  " << sectionName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void LibTestSuite::printSectionFooter() {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "  [OK] Section completed" << std::endl;
}

} // namespace Tests
} // namespace Lua
