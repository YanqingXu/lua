#include "test_main.hpp"
#include "lexer/lexer_test.hpp"
#include "parser/parser_test.hpp"
#include "vm/value_test.hpp"
#include "vm/state_test.hpp"
#include "compiler/symbol_table_test.hpp"
#include "compiler/literal_compiler_test.hpp"
#include "compiler/variable_compiler_test.hpp"
#include "compiler/binary_expression_test.hpp"
#include "parser/function_test.hpp"
#include "parser/forin_test.hpp"
#include "parser/repeat_test.hpp"
#include "compiler/expression_compiler_test.hpp"
#include "gc/gc_integration_test.hpp"
#include "gc/string_pool_demo_test.hpp"
#include "compiler/conditional_compilation_test.hpp"
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
        // ExpressionCompilerTest::runAllTests();        // Run string pool demo tests
        // StringPoolDemoTest::runAllTests();

        // Run GC integration tests
        // GCIntegrationTest::runAllTests();

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