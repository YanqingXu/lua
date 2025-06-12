#include "test_main.hpp"
#include <iostream>

/**
 * @brief Simple test runner to verify the new test organization structure
 * 
 * This file demonstrates how to use the newly organized test structure.
 * It can be compiled and run independently to test specific modules.
 */

int main() {
    std::cout << "=== Testing New Test Organization Structure ===" << std::endl;
    
    try {
        // Test 1: Run only compiler tests
        std::cout << "\n--- Testing Compiler Module ---" << std::endl;
        Lua::Tests::CompilerTest::runAllTests();
        
        // Test 2: Run only GC tests  
        std::cout << "\n--- Testing GC Module ---" << std::endl;
        Lua::Tests::GCTest::runAllTests();
        
        // Test 3: Run only Parser tests
        std::cout << "\n--- Testing Parser Module ---" << std::endl;
        Lua::Tests::ParserTestSuite::runAllTests();
        
        // Test 4: Run only VM tests
        std::cout << "\n--- Testing VM Module ---" << std::endl;
        Lua::Tests::VMTestSuite::runAllTests();
        
        // Test 5: Run a single test class
        std::cout << "\n--- Testing Single Lexer Test ---" << std::endl;
        Lua::Tests::LexerTest::runAllTests();
        
        std::cout << "\n=== Test Organization Structure Verification Complete ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\nTest failed with unknown exception" << std::endl;
        return 1;
    }
}
