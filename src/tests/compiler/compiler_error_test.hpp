#ifndef COMPILER_ERROR_TEST_HPP
#define COMPILER_ERROR_TEST_HPP

#include <iostream>
#include <string>
#include "../../compiler/compiler.hpp"
#include "../../parser/parser.hpp"
#include "../../lexer/lexer.hpp"
#include "../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Compiler Error Handling Test Class
 * 
 * This class tests the error handling mechanisms in the compiler,
 * including semantic error detection, type checking errors,
 * and compilation recovery strategies.
 * 
 * Test Coverage:
 * - Semantic error detection
 * - Type checking errors
 * - Undefined variable/function errors
 * - Scope resolution errors
 * - Symbol table errors
 * - Code generation errors
 * - Recovery and continuation after errors
 */
class CompilerErrorTest {
public:
    /**
     * @brief Run all compiler error handling tests
     * 
     * Executes all test groups for compiler error handling functionality.
     */
    static void runAllTests();
    
private:
    // Test groups
    static void testSemanticErrors();
    static void testTypeErrors();
    static void testScopeErrors();
    static void testSymbolTableErrors();
    static void testCodeGenerationErrors();
    static void testErrorRecovery();
    
    // Individual test methods
    static void testUndefinedVariables();
    static void testUndefinedFunctions();
    static void testRedefinitionErrors();
    
    static void testInvalidOperations();
    static void testTypeMismatch();
    static void testInvalidAssignments();
    
    static void testVariableOutOfScope();
    static void testFunctionScopeErrors();
    static void testNestedScopeErrors();
    
    static void testSymbolTableOverflow();
    static void testInvalidSymbolOperations();
    static void testSymbolTableCorruption();
    
    static void testInvalidBytecode();
    static void testCodeGenerationFailure();
    static void testOptimizationErrors();
    
    static void testMultipleErrors();
    static void testErrorCascading();
    static void testRecoveryAfterErrors();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed);
    static bool compileAndCheckError(const std::string& source, bool expectError = true);
    static bool containsSpecificError(const std::string& source, const std::string& errorType);
    static int countCompilationErrors(const std::string& source);
};

} // namespace Tests
} // namespace Lua

#endif // COMPILER_ERROR_TEST_HPP