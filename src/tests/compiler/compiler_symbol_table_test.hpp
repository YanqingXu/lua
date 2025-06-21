#pragma once

#include "../test_utils.hpp"
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
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Symbol Table Operations", testBasicOperations);
        RUN_TEST_GROUP("Scope Management", testScopeManagement);
        RUN_TEST_GROUP("Symbol Resolution", testSymbolResolution);
    }
    
private:
    static void testBasicOperations();
    static void testScopeManagement();
    static void testSymbolResolution();
    
    // Legacy test method - to be refactored
    static void testSymbolTable();
};

} // namespace Lua::Tests