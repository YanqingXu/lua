#include "test_vm.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void VMTestSuite::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "        VIRTUAL MACHINE TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all virtual machine-related tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        // 1. Value System Tests 
        printSectionHeader("Value System Tests");
        ValueTest::runAllTests();
        printSectionFooter();
        
        // 2. State Management Tests 
        printSectionHeader("State Management Tests");
        StateTest::runAllTests();
        printSectionFooter();
        
        // 总结
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL VM TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] VM TESTS FAILED" << std::endl;
        std::cout << "    Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    } catch (...) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] VM TESTS FAILED" << std::endl;
        std::cout << "    Unknown error occurred" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    }
}

void VMTestSuite::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "  " << sectionName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void VMTestSuite::printSectionFooter() {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "  [OK] Section completed" << std::endl;
}

} // namespace Tests
} // namespace Lua
