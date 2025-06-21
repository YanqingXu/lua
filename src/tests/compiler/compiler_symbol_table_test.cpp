#include "compiler_symbol_table_test.hpp"

namespace Lua::Tests {

void CompilerSymbolTableTest::runAllTests() {
    RUN_TEST_GROUP("Basic Symbol Table Operations", testBasicOperations);
    RUN_TEST_GROUP("Scope Management", testScopeManagement);
    RUN_TEST_GROUP("Symbol Resolution", testSymbolResolution);
}

void CompilerSymbolTableTest::testBasicOperations() {
    RUN_TEST(CompilerSymbolTableTest, testSymbolTable);
}

void CompilerSymbolTableTest::testScopeManagement() {
    RUN_TEST(CompilerSymbolTableTest, testBasicScopeOperations);
    RUN_TEST(CompilerSymbolTableTest, testNestedScopes);
    RUN_TEST(CompilerSymbolTableTest, testScopeManagerOperations);
    RUN_TEST(CompilerSymbolTableTest, testUpvalueManagement);
}

void CompilerSymbolTableTest::testSymbolResolution() {
    RUN_TEST(CompilerSymbolTableTest, testSymbolLookup);
    RUN_TEST(CompilerSymbolTableTest, testSymbolShadowing);
    RUN_TEST(CompilerSymbolTableTest, testCrossScopeResolution);
    RUN_TEST(CompilerSymbolTableTest, testVariableTypes);
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
    TestUtils::printTestResult("Found 'localVar' in local scope", localVar.has_value());

    // Should still find global variable
    auto globalVar = symbolTable.resolve("globalVar");
    TestUtils::printTestResult("Found 'globalVar' from local scope", globalVar.has_value());

    // Test nested scope
    TestUtils::printInfo("Testing nested scope");
    symbolTable.enterScope(); // Enter a nested scope
    
    symbolTable.define("innerVar", SymbolType::Variable);
    // Try to define a variable that shadows an outer scope variable
    symbolTable.define("x", SymbolType::Variable);

    auto innerVar = symbolTable.resolve("innerVar");
    TestUtils::printTestResult("Found 'innerVar' in nested scope", innerVar.has_value());

    // Should find the inner 'x'
    auto innerX = symbolTable.resolve("x");
    TestUtils::printTestResult("Found shadowed 'x' in nested scope", innerX.has_value());

    // Leave the nested scope
    symbolTable.leaveScope();

    // Should now find the outer 'x'
    auto outerX = symbolTable.resolve("x");
    TestUtils::printTestResult("Found outer 'x' after leaving nested scope", outerX.has_value());

    // Leave the local scope
    symbolTable.leaveScope();

    // Should not find local variables anymore
    auto notFound = symbolTable.resolve("localVar");
    TestUtils::printTestResult("'localVar' not found after leaving local scope", !notFound.has_value());

    TestUtils::printInfo("Symbol Table Test completed!");
}

void CompilerSymbolTableTest::testBasicScopeOperations() {
        TestUtils::printInfo("Testing basic scope operations");
        
        SymbolTable symbolTable;
        
        // Test initial state
        TestUtils::printTestResult("Initial scope level is 0", symbolTable.getCurrentScopeLevel() == 0);
        
        // Test entering and leaving scopes
        symbolTable.enterScope();
        TestUtils::printTestResult("Scope level after entering is 1", symbolTable.getCurrentScopeLevel() == 1);
        
        symbolTable.enterScope();
        TestUtils::printTestResult("Scope level after entering again is 2", symbolTable.getCurrentScopeLevel() == 2);
        
        symbolTable.leaveScope();
        TestUtils::printTestResult("Scope level after leaving is 1", symbolTable.getCurrentScopeLevel() == 1);
        
        symbolTable.leaveScope();
        TestUtils::printTestResult("Scope level after leaving again is 0", symbolTable.getCurrentScopeLevel() == 0);
    }
    
void CompilerSymbolTableTest::testNestedScopes() {
        TestUtils::printInfo("Testing nested scopes");
        
        SymbolTable symbolTable;
        
        // Define in global scope
        bool globalDefined = symbolTable.define("global", SymbolType::Variable);
        TestUtils::printTestResult("Global variable defined", globalDefined);
        
        // Enter first nested scope
        symbolTable.enterScope();
        bool localDefined = symbolTable.define("local", SymbolType::Variable);
        TestUtils::printTestResult("Local variable defined", localDefined);
        
        // Enter second nested scope
        symbolTable.enterScope();
        bool innerDefined = symbolTable.define("inner", SymbolType::Variable);
        TestUtils::printTestResult("Inner variable defined", innerDefined);
        
        // Test resolution from innermost scope
        auto globalSymbol = symbolTable.resolve("global");
        auto localSymbol = symbolTable.resolve("local");
        auto innerSymbol = symbolTable.resolve("inner");
        
        TestUtils::printTestResult("Global symbol found from inner scope", globalSymbol.has_value());
        TestUtils::printTestResult("Local symbol found from inner scope", localSymbol.has_value());
        TestUtils::printTestResult("Inner symbol found from inner scope", innerSymbol.has_value());
        
        // Leave inner scope
        symbolTable.leaveScope();
        auto innerNotFound = symbolTable.resolve("inner");
        TestUtils::printTestResult("Inner symbol not found after leaving scope", !innerNotFound.has_value());
        
        // Leave local scope
        symbolTable.leaveScope();
        auto localNotFound = symbolTable.resolve("local");
        TestUtils::printTestResult("Local symbol not found after leaving scope", !localNotFound.has_value());
        
        // Global should still be accessible
        auto globalStillFound = symbolTable.resolve("global");
        TestUtils::printTestResult("Global symbol still found", globalStillFound.has_value());
    }
    
void CompilerSymbolTableTest::testScopeManagerOperations() {
        TestUtils::printInfo("Testing ScopeManager operations");
        
        ScopeManager scopeManager;
        
        // Test initial state
        TestUtils::printTestResult("Initial scope level is 0", scopeManager.getCurrentScopeLevel() == 0);
        TestUtils::printTestResult("Initial local count is 0", scopeManager.getLocalCount() == 0);
        
        // Test entering scope
        scopeManager.enterScope();
        TestUtils::printTestResult("Scope level after entering is 1", scopeManager.getCurrentScopeLevel() == 1);
        
        // Test defining local variables
        bool local1Defined = scopeManager.defineLocal("local1", 0);
        bool local2Defined = scopeManager.defineLocal("local2", 1);
        TestUtils::printTestResult("Local1 defined successfully", local1Defined);
        TestUtils::printTestResult("Local2 defined successfully", local2Defined);
        TestUtils::printTestResult("Local count is 2", scopeManager.getLocalCount() == 2);
        
        // Test finding variables
        Variable* var1 = scopeManager.findVariable("local1");
        Variable* var2 = scopeManager.findVariable("local2");
        TestUtils::printTestResult("Local1 found", var1 != nullptr);
        TestUtils::printTestResult("Local2 found", var2 != nullptr);
        
        if (var1) {
            TestUtils::printTestResult("Local1 has correct stack index", var1->stackIndex == 0);
        }
        
        // Test scope validation
        TestUtils::printTestResult("Current scope is valid", scopeManager.validateCurrentScope());
        
        // Test exiting scope
        scopeManager.exitScope();
        TestUtils::printTestResult("Scope level after exiting is 0", scopeManager.getCurrentScopeLevel() == 0);
        
        // Variables should not be found after exiting scope
        Variable* varNotFound = scopeManager.findVariable("local1");
        TestUtils::printTestResult("Local1 not found after exiting scope", varNotFound == nullptr);
    }
    
void CompilerSymbolTableTest::testUpvalueManagement() {
        TestUtils::printInfo("Testing upvalue management");
        
        ScopeManager scopeManager;
        
        // Enter outer scope and define a variable
        scopeManager.enterScope();
        bool outerVarDefined = scopeManager.defineLocal("outerVar", 0);
        TestUtils::printTestResult("Outer variable defined", outerVarDefined);
        
        // Enter inner scope
        scopeManager.enterScope();
        
        // Mark outer variable as captured
        bool marked = scopeManager.markAsCaptured("outerVar");
        TestUtils::printTestResult("Outer variable marked as captured", marked);
        
        // Add upvalue
        int upvalueIndex = scopeManager.addUpvalue("outerVar", true, 0);
        TestUtils::printTestResult("Upvalue added successfully", upvalueIndex >= 0);
        
        // Test upvalue queries
        TestUtils::printTestResult("Variable is upvalue", scopeManager.isUpvalue("outerVar"));
        TestUtils::printTestResult("Variable is free variable", scopeManager.isFreeVariable("outerVar"));
        
        // Get upvalues
        const auto& upvalues = scopeManager.getUpvalues();
        TestUtils::printTestResult("Upvalue list has one entry", upvalues.size() == 1);
        
        if (!upvalues.empty()) {
            TestUtils::printTestResult("Upvalue has correct name", upvalues[0].name == "outerVar");
            TestUtils::printTestResult("Upvalue is local", upvalues[0].isLocal);
        }
        
        scopeManager.exitScope();
        scopeManager.exitScope();
    }
    
void CompilerSymbolTableTest::testSymbolLookup() {
        TestUtils::printInfo("Testing symbol lookup");
        
        SymbolTable symbolTable;
        
        // Define symbols of different types
        symbolTable.define("var", SymbolType::Variable);
        symbolTable.define("func", SymbolType::Function);
        symbolTable.define("param", SymbolType::Parameter);
        
        // Test resolution
        auto varSymbol = symbolTable.resolve("var");
        auto funcSymbol = symbolTable.resolve("func");
        auto paramSymbol = symbolTable.resolve("param");
        auto notFound = symbolTable.resolve("nonexistent");
        
        TestUtils::printTestResult("Variable symbol found", varSymbol.has_value());
        TestUtils::printTestResult("Function symbol found", funcSymbol.has_value());
        TestUtils::printTestResult("Parameter symbol found", paramSymbol.has_value());
        TestUtils::printTestResult("Nonexistent symbol not found", !notFound.has_value());
        
        // Test symbol types
        if (varSymbol.has_value()) {
            TestUtils::printTestResult("Variable has correct type", varSymbol->type == SymbolType::Variable);
        }
        if (funcSymbol.has_value()) {
            TestUtils::printTestResult("Function has correct type", funcSymbol->type == SymbolType::Function);
        }
    }
    
void CompilerSymbolTableTest::testSymbolShadowing() {
        TestUtils::printInfo("Testing symbol shadowing");
        
        SymbolTable symbolTable;
        
        // Define in global scope
        symbolTable.define("x", SymbolType::Variable);
        auto globalX = symbolTable.resolve("x");
        TestUtils::printTestResult("Global x defined", globalX.has_value());
        
        // Enter local scope and shadow
        symbolTable.enterScope();
        symbolTable.define("x", SymbolType::Parameter);
        auto localX = symbolTable.resolve("x");
        
        TestUtils::printTestResult("Local x found", localX.has_value());
        if (localX.has_value()) {
            TestUtils::printTestResult("Local x is parameter type", localX->type == SymbolType::Parameter);
            TestUtils::printTestResult("Local x has correct scope level", localX->scopeLevel == 1);
        }
        
        // Leave scope and check global is accessible again
        symbolTable.leaveScope();
        auto globalXAgain = symbolTable.resolve("x");
        TestUtils::printTestResult("Global x accessible after leaving scope", globalXAgain.has_value());
        if (globalXAgain.has_value()) {
            TestUtils::printTestResult("Global x is variable type", globalXAgain->type == SymbolType::Variable);
        }
    }
    
void CompilerSymbolTableTest::testCrossScopeResolution() {
        TestUtils::printInfo("Testing cross-scope resolution");
        
        SymbolTable symbolTable;
        
        // Define in global scope
        symbolTable.define("global1", SymbolType::Variable);
        symbolTable.define("global2", SymbolType::Function);
        
        // Enter first level
        symbolTable.enterScope();
        symbolTable.define("local1", SymbolType::Variable);
        
        // Enter second level
        symbolTable.enterScope();
        symbolTable.define("local2", SymbolType::Parameter);
        
        // Test resolution from deepest level
        TestUtils::printTestResult("Global1 found from deep scope", symbolTable.resolve("global1").has_value());
        TestUtils::printTestResult("Global2 found from deep scope", symbolTable.resolve("global2").has_value());
        TestUtils::printTestResult("Local1 found from deep scope", symbolTable.resolve("local1").has_value());
        TestUtils::printTestResult("Local2 found from deep scope", symbolTable.resolve("local2").has_value());
        
        // Test current scope check
        TestUtils::printTestResult("Local2 is in current scope", symbolTable.isDefinedInCurrentScope("local2"));
        TestUtils::printTestResult("Local1 is not in current scope", !symbolTable.isDefinedInCurrentScope("local1"));
        TestUtils::printTestResult("Global1 is not in current scope", !symbolTable.isDefinedInCurrentScope("global1"));
        
        symbolTable.leaveScope();
        symbolTable.leaveScope();
    }
    
void CompilerSymbolTableTest::testVariableTypes() {
        TestUtils::printInfo("Testing variable types");
        
        ScopeManager scopeManager;
        
        // Enter scope and define different types
        scopeManager.enterScope();
        scopeManager.defineLocal("localVar", 0);
        
        // Test local variable queries
        TestUtils::printTestResult("Variable is in current scope", scopeManager.isInCurrentScope("localVar"));
        TestUtils::printTestResult("Variable is local", scopeManager.isLocalVariable("localVar"));
        TestUtils::printTestResult("Variable is not free", !scopeManager.isFreeVariable("localVar"));
        
        // Enter inner scope
        scopeManager.enterScope();
        
        // Now localVar should be a free variable from inner scope
        TestUtils::printTestResult("Variable is not in current scope", !scopeManager.isInCurrentScope("localVar"));
        TestUtils::printTestResult("Variable is not local from inner scope", !scopeManager.isLocalVariable("localVar"));
        TestUtils::printTestResult("Variable is free from inner scope", scopeManager.isFreeVariable("localVar"));
        
        scopeManager.exitScope();
        scopeManager.exitScope();
    }

} // namespace Lua::Tests