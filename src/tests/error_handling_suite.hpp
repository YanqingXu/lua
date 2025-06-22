#pragma once

#include "test_utils.hpp"
#include "lexer/lexer_error_test.hpp"
#include "parser/error_recovery_test.hpp"
#include "compiler/compiler_error_test.hpp"
#include "vm/vm_error_test.hpp"
#include "gc/gc_error_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Comprehensive Error Handling Test Suite
 * 
 * This suite coordinates all error handling tests across different modules
 * of the Lua interpreter, providing a unified interface to test error
 * detection, handling, and recovery mechanisms throughout the system.
 * 
 * Test Coverage:
 * - Lexer error handling (invalid tokens, malformed input)
 * - Parser error recovery (syntax errors, synchronization)
 * - Compiler error detection (semantic errors, type checking)
 * - VM runtime error handling (execution errors, stack management)
 * - GC error handling (memory management, collection errors)
 */
class ErrorHandlingSuite {
public:
    /**
     * @brief Run all error handling tests
     * 
     * Executes error handling tests for all modules in a logical order,
     * from lexical analysis through runtime execution and memory management.
     */
    static void runAllTests();

private:
    /**
     * @brief Run lexer error handling tests
     * 
     * Tests error detection and handling in the lexical analysis phase,
     * including invalid characters, malformed tokens, and input validation.
     */
    static void runLexerErrorTests();
    
    /**
     * @brief Run parser error handling tests
     * 
     * Tests error recovery mechanisms in the parsing phase,
     * including syntax error detection, synchronization, and recovery strategies.
     */
    static void runParserErrorTests();
    
    /**
     * @brief Run compiler error handling tests
     * 
     * Tests error detection in the compilation phase,
     * including semantic analysis, type checking, and code generation errors.
     */
    static void runCompilerErrorTests();
    
    /**
     * @brief Run VM error handling tests
     * 
     * Tests runtime error handling in the virtual machine,
     * including execution errors, stack management, and exception propagation.
     */
    static void runVMErrorTests();
    
    /**
     * @brief Run GC error handling tests
     * 
     * Tests error handling in the garbage collector,
     * including memory allocation failures, collection errors, and cleanup.
     */
    static void runGCErrorTests();
    
    /**
     * @brief Run integration error tests
     * 
     * Tests error handling across module boundaries,
     * ensuring proper error propagation and system-wide error recovery.
     */
    static void runIntegrationErrorTests();
    
    /**
     * @brief Test error propagation between modules
     * 
     * Verifies that errors are properly propagated from one module to another
     * and that the system maintains consistency during error conditions.
     */
    static void testErrorPropagation();
    
    /**
     * @brief Test system-wide error recovery
     * 
     * Tests the system's ability to recover from errors and continue
     * operation, including cleanup and state restoration.
     */
    static void testSystemErrorRecovery();
    
    /**
     * @brief Test error reporting consistency
     * 
     * Verifies that error messages are consistent across modules
     * and provide useful information for debugging.
     */
    static void testErrorReporting();
    
    /**
     * @brief Print test suite header
     */
    static void printSuiteHeader();
    
    /**
     * @brief Print test suite footer
     */
    static void printSuiteFooter();
    
    /**
     * @brief Print module test header
     * @param moduleName Name of the module being tested
     */
    static void printModuleHeader(const std::string& moduleName);
    
    /**
     * @brief Print module test footer
     * @param moduleName Name of the module that was tested
     */
    static void printModuleFooter(const std::string& moduleName);
};

} // namespace Tests
} // namespace Lua