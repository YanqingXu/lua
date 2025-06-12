#include "test_main.hpp"
#include "lexer_test.hpp"
#include "parser_test.hpp"
#include "value_test.hpp"
#include "state_test.hpp"
#include "symbol_table_test.hpp"
#include "literal_compiler_test.hpp"
#include "variable_compiler_test.hpp"
#include "binary_expression_test.hpp"
#include "function_test.hpp"
#include "forin_test.hpp"
#include "repeat_test.hpp"
#include "expression_compiler_test.hpp"
#include "gc_integration_test.hpp"
#include "string_pool_demo_test.hpp"
#include "conditional_compilation_test.hpp"
#include <iostream>

namespace Lua {
namespace Tests {

// Demo functions moved from main.cpp
void runStringPoolDemo() {
    try {
        Lua::Tests::runStringPoolDemoTests();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

void runGCIntegrationDemo() {
    try {
        Lua::Tests::runGCIntegrationTests();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

void runAllTests() {
    std::cout << "=== Running Lua Interpreter Tests ===" << std::endl;
    
    try {
        // // Run lexer tests
        // testLexer("local x = 42 + 3.14");
        
        // // Run value tests
        // testValues();
        
        // // Run state tests
        // testState();
        // testExecute();
        
        // // Run parser tests
        // testParser();
        // testStatements();
        // testWhileLoop();
        // testASTVisitor();
        
        // // Run symbol table tests
        // testSymbolTable();
        
        // // Run for-in loop tests
        // runForInTests();
        
        // // Run repeat-until loop tests
        // runRepeatUntilTests();
        
        // // Run function tests
        // runFunctionTests();
        
        // Run literal compiler tests
        // LiteralCompilerTest::runAllTests();
        
        // Run variable compiler tests
        // VariableCompilerTest::runAllTests();
        
        // Run binary expression tests
        // BinaryExpressionTest::runAllTests();
        
        // Run expression compiler tests
        // ExpressionCompilerTest::runAllTests();

        // Run string pool demo tests
        // runStringPoolDemo();

        // Run GC integration tests
        // runGCIntegrationDemo();

		// Run Conditional Compilation tests
		ConditionalCompilationTest::runAllTests();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\nTest execution failed with exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "\nTest execution failed with unknown exception" << std::endl;
    }
}

} // namespace Tests
} // namespace Lua