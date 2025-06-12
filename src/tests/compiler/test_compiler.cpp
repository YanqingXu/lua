#include "test_compiler.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void CompilerTest::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "          COMPILER TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all compiler-related tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
		// 1. Symbol Table Tests 
        printSectionHeader("Symbol Table Tests");
        SymbolTableTest::runAllTests();
        printSectionFooter();
        
        // 2. Literal Compiler Tests 
        printSectionHeader("Literal Compiler Tests");
        LiteralCompilerTest::runAllTests();
        printSectionFooter();
        
        // 3. Variable Compiler Tests 
        printSectionHeader("Variable Compiler Tests");
        VariableCompilerTest::runAllTests();
        printSectionFooter();
        
        // 4. Binary Expression Tests 
        printSectionHeader("Binary Expression Tests");
        BinaryExpressionTest::runAllTests();
        printSectionFooter();
        
        // 5. Expression Compiler Tests 
        printSectionHeader("Expression Compiler Tests");
        ExpressionCompilerTest::runAllTests();
        printSectionFooter();
        
        // 6. Conditional Compilation Tests 
        printSectionHeader("Conditional Compilation Tests");
        ConditionalCompilationTest::runAllTests();
        printSectionFooter();
        
        // 总结
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL COMPILER TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] COMPILER TESTS FAILED" << std::endl;
        std::cout << "    Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    } catch (...) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] COMPILER TESTS FAILED" << std::endl;
        std::cout << "    Unknown error occurred" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    }
}

void CompilerTest::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "  " << sectionName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void CompilerTest::printSectionFooter() {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "  [OK] Section completed" << std::endl;
}

} // namespace Tests
} // namespace Lua
