#pragma once

#include "../../test_framework/core/test_macros.hpp"
#include "../../vm/vm.hpp"
#include "../../compiler/compiler.hpp"
#include "../../parser/parser.hpp"
#include "../../lexer/lexer.hpp"
#include <string>
#include <exception>

namespace Lua {
namespace Tests {

/**
 * @brief VM Error Handling Test Suite
 * 
 * Tests the virtual machine's error handling capabilities including:
 * - Runtime errors (division by zero, nil access, etc.)
 * - Stack overflow and underflow
 * - Memory allocation errors
 * - Bytecode execution errors
 * - Error propagation and recovery
 * - Exception handling mechanisms
 */
class VMErrorTest {
public:
    /**
     * @brief Run all VM error handling tests
     * 
     * Executes all test groups in this suite using the standardized
     * test framework for consistent formatting and error handling.
     */
    static void runAllTests();

private:
    // Test groups
    static void testRuntimeErrors();
    static void testStackErrors();
    static void testMemoryErrors();
    static void testBytecodeErrors();
    static void testErrorPropagation();
    static void testExceptionHandling();
    
    // Runtime error tests
    static void testDivisionByZero();
    static void testNilAccess();
    static void testInvalidOperations();
    static void testTypeErrors();
    static void testIndexOutOfBounds();
    
    // Stack error tests
    static void testStackOverflow();
    static void testStackUnderflow();
    static void testInvalidStackOperations();
    static void testStackCorruption();
    
    // Memory error tests
    static void testOutOfMemory();
    static void testMemoryLeaks();
    static void testInvalidMemoryAccess();
    static void testGarbageCollectionErrors();
    
    // Bytecode error tests
    static void testInvalidBytecode();
    static void testCorruptedBytecode();
    static void testUnsupportedInstructions();
    static void testBytecodeVersionMismatch();
    
    // Error propagation tests
    static void testErrorInNestedCalls();
    static void testErrorInCoroutines();
    static void testErrorInMetamethods();
    static void testErrorRecoveryMechanisms();
    
    // Exception handling tests
    static void testCppExceptionHandling();
    static void testLuaErrorHandling();
    static void testMixedExceptionTypes();
    static void testExceptionCleanup();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed);
    static bool executeAndCheckError(const std::string& source, bool expectError = true);
    static bool containsRuntimeError(const std::string& source, const std::string& errorType);
    static int countRuntimeErrors(const std::string& source);
    static bool checkStackState(VM* vm, int expectedSize);
    static bool simulateMemoryPressure();
};

} // namespace Tests
} // namespace Lua