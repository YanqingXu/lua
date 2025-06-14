#include "test_parser.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void ParserTestSuite::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "          PARSER TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all parser-related tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        // 1. Basic Parser Tests
        printSectionHeader("Basic Parser Tests");
        ParserTest::runAllTests();
        printSectionFooter();
        
        // 2. Function Definition Tests
        printSectionHeader("Function Definition Tests");
        FunctionTest::runAllTests();
        printSectionFooter();
        
        // 3. If Statement Tests
        printSectionHeader("If Statement Tests");
        IfStatementTest::runAllTests();
        printSectionFooter();
        
        // 4. For-In Loop Tests
        printSectionHeader("For-In Loop Tests");
        ForInTest::runAllTests();
        printSectionFooter();
        
        // 5. Repeat-Until Loop Tests
        printSectionHeader("Repeat-Until Loop Tests");
        RepeatTest::runAllTests();
        printSectionFooter();
        
        // 6. Source Location Tests
        printSectionHeader("Source Location Tests");
        SourceLocationTest::runAllTests();
        printSectionFooter();
        
        // 7. Parse Error Tests
        printSectionHeader("Parse Error Tests");
        ParseErrorTest::runAllTests();
        printSectionFooter();

		// 8. Error Reporter Tests
		printSectionHeader("Error Reporter Tests");
		ErrorReporterTest::runAllTests();
		printSectionFooter();
		
		// 9. Enhanced Parser Error Tests
		printSectionHeader("Enhanced Parser Error Tests");
		EnhancedParserErrorTest::runAllTests();
		printSectionFooter();
        
        // Summary
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL PARSER TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] PARSER TESTS FAILED" << std::endl;
        std::cout << "    Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    } catch (...) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] PARSER TESTS FAILED" << std::endl;
        std::cout << "    Unknown error occurred" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw; // Re-throw to let caller handle
    }
}

void ParserTestSuite::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "  " << sectionName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void ParserTestSuite::printSectionFooter() {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "  [OK] Section completed" << std::endl;
}

} // namespace Tests
} // namespace Lua
