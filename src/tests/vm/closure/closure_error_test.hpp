#ifndef LUA_TESTS_VM_CLOSURE_ERROR_TESTS_HPP
#define LUA_TESTS_VM_CLOSURE_ERROR_TESTS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

namespace Lua {
namespace Tests {

/**
 * @class ClosureErrorTests
 * @brief Test suite for closure error handling and edge cases
 * 
 * This class contains tests for various error conditions and edge cases
 * that can occur when working with closures, including compilation errors,
 * runtime errors, memory errors, and boundary conditions.
 */
class ClosureErrorTest {
public:
    /**
     * @brief Run all closure error handling tests
     * 
     * Executes the complete suite of error handling tests for closures,
     * including compilation errors, runtime errors, memory errors, and
     * edge cases.
     */
    static void runAllTests();
    
    // Test group methods
    static void runCompilationErrorTests();
    static void runRuntimeErrorTests();
    static void runMemoryErrorTests();
    static void runEdgeCaseTests();
    static void runErrorRecoveryTests();
    static void runBoundaryConditionTests();

private:
    // Compilation error tests
    static void testCompilationErrors();
    static void testSyntaxErrors();
    static void testInvalidUpvalueReferences();
    static void testCircularDependencies();
    static void testInvalidNesting();
    
    // Runtime error tests
    static void testRuntimeErrors();
    static void testUpvalueAccessErrors();
    static void testClosureInvocationErrors();
    static void testTypeErrors();
    static void testNilClosureErrors();
    
    // Memory error tests
    static void testMemoryErrors();
    static void testOutOfMemoryConditions();
    static void testMemoryCorruption();
    static void testDanglingReferences();
    static void testMemoryLeakDetection();
    
    // Edge case tests
    static void testEdgeCases();
    static void testEmptyClosures();
    static void testVeryDeepNesting();
    static void testExtremeUpvalueCounts();
    static void testLargeClosureArrays();
    static void testConcurrentAccess();
    
    // Error recovery tests
    static void testErrorRecovery();
    static void testGracefulDegradation();
    static void testErrorPropagation();
    static void testExceptionSafety();
    
    // Boundary condition tests
    static void testBoundaryConditions();
    static void testMaximumLimits();
    static void testMinimumLimits();
    static void testResourceExhaustion();
    
    // Helper methods
    static void printErrorInfo(const std::string& errorType, const std::string& details);
    
    static bool expectCompilationError(const std::string& luaCode, const std::string& expectedError = "");
    static bool expectRuntimeError(const std::string& luaCode, const std::string& expectedError = "");
    static bool compileAndExecute(const std::string& luaCode);
    static bool executeErrorTest(const std::string& luaCode, bool shouldFail = true);
    
    static void setupErrorTestEnvironment();
    static void cleanupErrorTestEnvironment();
    
    // Error handling utilities
    static std::string captureErrorMessage(const std::function<void()>& operation);
    static bool isExpectedError(const std::string& actualError, const std::string& expectedPattern);
    static void logErrorDetails(const std::string& testName, const std::string& error);
};

} // namespace Tests
} // namespace Lua

#endif // LUA_TESTS_VM_CLOSURE_ERROR_TESTS_HPP