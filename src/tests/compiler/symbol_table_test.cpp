#include "symbol_table_test.hpp"

namespace Lua {
namespace Tests {

void SymbolTableTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Symbol Table Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testSymbolTable();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Symbol Table Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void SymbolTableTest::testSymbolTable() {
    std::cout << "\nSymbol Table Test:" << std::endl;

    SymbolTable symbolTable;

    // Test global scope
    std::cout << "Testing global scope:" << std::endl;
    symbolTable.define("print", SymbolType::Function);
    symbolTable.define("globalVar", SymbolType::Variable);

    auto printSymbol = symbolTable.resolve("print");
    if (printSymbol) {
        std::cout << "Found 'print' in scope level: " << printSymbol->scopeLevel << std::endl;
    }

    // Test local scope
    std::cout << "\nTesting local scope:" << std::endl;
    symbolTable.enterScope(); // Enter a new scope

    symbolTable.define("localVar", SymbolType::Variable);
    symbolTable.define("x", SymbolType::Parameter);

    // Should find local variable
    auto localVar = symbolTable.resolve("localVar");
    if (localVar) {
        std::cout << "Found 'localVar' in scope level: " << localVar->scopeLevel << std::endl;
    }

    // Should still find global variable
    auto globalVar = symbolTable.resolve("globalVar");
    if (globalVar) {
        std::cout << "Found 'globalVar' in scope level: " << globalVar->scopeLevel << std::endl;
    }

    // Test nested scope
    std::cout << "\nTesting nested scope:" << std::endl;
    symbolTable.enterScope(); // Enter a nested scope
    
    symbolTable.define("innerVar", SymbolType::Variable);
    // Try to define a variable that shadows an outer scope variable
    symbolTable.define("x", SymbolType::Variable);

    auto innerVar = symbolTable.resolve("innerVar");
    if (innerVar) {
        std::cout << "Found 'innerVar' in scope level: " << innerVar->scopeLevel << std::endl;
    }

    // Should find the inner 'x'
    auto innerX = symbolTable.resolve("x");
    if (innerX) {
        std::cout << "Found 'x' in scope level: " << innerX->scopeLevel << std::endl;
    }

    // Leave the nested scope
    symbolTable.leaveScope();

    // Should now find the outer 'x'
    auto outerX = symbolTable.resolve("x");
    if (outerX) {
        std::cout << "Found 'x' in scope level: " << outerX->scopeLevel << std::endl;
    }

    // Leave the local scope
    symbolTable.leaveScope();

    // Should not find local variables anymore
    auto notFound = symbolTable.resolve("localVar");
    if (!notFound) {
        std::cout << "'localVar' is no longer accessible" << std::endl;
    }

    std::cout << "\nSymbol Table Test completed!" << std::endl;
}

} // namespace Tests
} // namespace Lua