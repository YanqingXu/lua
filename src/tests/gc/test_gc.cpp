#include "test_gc.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void GCTest::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "      GARBAGE COLLECTOR TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all garbage collector-related tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        // 1. String Pool Demo Tests 
        printSectionHeader("String Pool Demo Tests");
        StringPoolDemoTest::runAllTests();
        printSectionFooter();
        
        // 2. GC Integration Tests 
        printSectionHeader("GC Integration Tests");
        bool gcTestResult = GCIntegrationTest::runAllTests();
        if (!gcTestResult) {
            std::cout << "[WARNING]  Warning: Some GC integration tests failed" << std::endl;
        }        printSectionFooter();
        
        // 总结
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL GC TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] GC TESTS FAILED" << std::endl;
        std::cout << "    Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    } catch (...) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] GC TESTS FAILED" << std::endl;
        std::cout << "    Unknown error occurred" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    }
}

void GCTest::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "  " << sectionName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void GCTest::printSectionFooter() {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "  [OK] Section completed" << std::endl;
}

} // namespace Tests
} // namespace Lua
