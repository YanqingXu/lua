#pragma once

#include "../test_utils.hpp"

namespace Lua::Tests {

/**
 * @brief Compiler Variable Test Suite
 * 
 * Tests variable compilation functionality including:
 * - Local and global variable access
 * - Variable resolution and scope handling
 * - Register allocation for variables
 * - Instruction generation for variable operations
 * - Error handling in variable compilation
 */
class CompilerVariableTest {
public:
    /**
     * @brief Run all variable compiler tests
     * 
     * Executes all test groups for variable compilation functionality
     */
    static void runAllTests();
    
private:
    // Test group functions
    static void testBasicVariableOperations();
    static void testScopeAndResolution();
    static void testCompilerIntegration();
    
    // Individual test methods
    static void testLocalVariableAccess();
    static void testGlobalVariableAccess();
    static void testVariableResolution();
    static void testScopeHandling();
    static void testRegisterAllocation();
    static void testInstructionGeneration();
    static void testErrorHandling();
};

} // namespace Lua::Tests