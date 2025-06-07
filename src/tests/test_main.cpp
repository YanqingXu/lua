#include "test_main.hpp"
#include "forin_test.hpp"
#include <iostream>

namespace Lua {
namespace Tests {

void runAllTests() {
    std::cout << "=== Running Lua Interpreter Tests ===" << std::endl;
    
    try {
        // Run lexer tests
        testLexer("local x = 42 + 3.14");
        
        // Run value tests
        testValues();
        
        // Run state tests
        testState();
        testExecute();
        
        // Run parser tests
        testParser();
        testStatements();
        testWhileLoop();
        testASTVisitor();
        
        // Run symbol table tests
        testSymbolTable();
        
        // Run for-in loop tests
        runForInTests();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\nTest execution failed with exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "\nTest execution failed with unknown exception" << std::endl;
    }
}

} // namespace Tests
} // namespace Lua