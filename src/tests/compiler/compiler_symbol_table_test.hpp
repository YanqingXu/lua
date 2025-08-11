#pragma once

#include <iostream>
#include "../../compiler/symbol_table.hpp"

namespace Lua::Tests {

/**
 * @brief Symbol Table Test Suite
 * 
 * Tests the symbol table functionality including scope management,
 * symbol definition, resolution, and nested scope handling.
 */
class CompilerSymbolTableTest {
public:
    /**
     * @brief Run all symbol table tests
     * 
     * Execute all test groups for symbol table functionality
     */
    static void runAllTests();
    
private:
    static void testBasicOperations();
    static void testScopeManagement();
    static void testSymbolResolution();
    
    // Legacy test method - to be refactored
    static void testSymbolTable();
    
    // Detailed test methods
    static void testBasicScopeOperations();
    static void testNestedScopes();
    static void testScopeManagerOperations();
    static void testUpvalueManagement();
    static void testSymbolLookup();
    static void testSymbolShadowing();
    static void testCrossScopeResolution();
    static void testVariableTypes();
};

} // namespace Lua::Tests