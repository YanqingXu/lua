#include "test_lib.hpp"
#include "table_lib_test.hpp"
#include "base_lib_test.hpp"
#include "math_lib_test.hpp"
#include <iostream>

namespace Lua {
namespace Tests {

void LibTestSuite::runAllTests() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "                    STANDARD LIBRARY TEST SUITE" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Running all standard library tests..." << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    try {
        // Run base library tests
        BaseLibTest baseTest;
        baseTest.runAllTests();
        
        // Run table library tests
        TableLibTest tableTest;
        tableTest.runAllTests();
        
        // Run math library tests
        MathLibTest::runAllTests();
        
        // TODO: Add other library tests here
        // StringLibTest stringTest;
        // stringTest.runAllTests();
        
        // IOLibTest ioTest;
        // ioTest.runAllTests();
        
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "                ALL STANDARD LIBRARY TESTS PASSED!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "                STANDARD LIBRARY TESTS FAILED!" << std::endl;
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        throw;
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
