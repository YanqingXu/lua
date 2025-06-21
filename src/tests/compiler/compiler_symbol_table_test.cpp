#include "compiler_symbol_table_test.hpp"

namespace Lua::Tests {

void CompilerSymbolTableTest::testBasicOperations() {
    RUN_TEST(CompilerSymbolTableTest, testSymbolTable);
}

void CompilerSymbolTableTest::testScopeManagement() {
    // Implementation for scope management tests
}

void CompilerSymbolTableTest::testSymbolResolution() {
    // Implementation for symbol resolution tests
}

void CompilerSymbolTableTest::testSymbolTable() {
    TestUtils::printInfo("Testing symbol table basic functionality");

    SymbolTable symbolTable;

    // Test global scope
    TestUtils::printInfo("Testing global scope");
    symbolTable.define("print", SymbolType::Function);
    symbolTable.define("globalVar", SymbolType::Variable);

    auto printSymbol = symbolTable.resolve("print");
    if (printSymbol) {
        TestUtils::printTestResult("Found 'print' symbol", true);
    } else {
        TestUtils::printTestResult("Found 'print' symbol", false);
    }

    // Test local scope
    TestUtils::printInfo("Testing local scope");
    symbolTable.enterScope(); // Enter a new scope

    symbolTable.define("localVar", SymbolType::Variable);
    symbolTable.define("x", SymbolType::Parameter);

    // Should find local variable
    auto localVar = symbolTable.resolve("localVar");
    TestUtils::printTestResult("Found 'localVar' in local scope", localVar != nullptr);

    // Should still find global variable
    auto globalVar = symbolTable.resolve("globalVar");
    TestUtils::printTestResult("Found 'globalVar' from local scope", globalVar != nullptr);

    // Test nested scope
    TestUtils::printInfo("Testing nested scope");
    symbolTable.enterScope(); // Enter a nested scope
    
    symbolTable.define("innerVar", SymbolType::Variable);
    // Try to define a variable that shadows an outer scope variable
    symbolTable.define("x", SymbolType::Variable);

    auto innerVar = symbolTable.resolve("innerVar");
    TestUtils::printTestResult("Found 'innerVar' in nested scope", innerVar != nullptr);

    // Should find the inner 'x'
    auto innerX = symbolTable.resolve("x");
    TestUtils::printTestResult("Found shadowed 'x' in nested scope", innerX != nullptr);

    // Leave the nested scope
    symbolTable.leaveScope();

    // Should now find the outer 'x'
    auto outerX = symbolTable.resolve("x");
    TestUtils::printTestResult("Found outer 'x' after leaving nested scope", outerX != nullptr);

    // Leave the local scope
    symbolTable.leaveScope();

    // Should not find local variables anymore
    auto notFound = symbolTable.resolve("localVar");
    TestUtils::printTestResult("'localVar' not found after leaving local scope", notFound == nullptr);

    TestUtils::printInfo("Symbol Table Test completed!");
}

} // namespace Tests
} // namespace Lua