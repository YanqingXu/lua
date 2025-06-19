#ifndef LUA_CLOSURE_BOUNDARY_TEST_HPP
#define LUA_CLOSURE_BOUNDARY_TEST_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <exception>
#include <chrono>

// Include necessary components for testing
#include "../../../lexer/lexer.hpp"
#include "../../../parser/parser.hpp"
#include "../../../compiler/compiler.hpp"
#include "../../../vm/vm.hpp"
#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../../vm/function.hpp"
#include "../../../vm/upvalue.hpp"
#include "../../../common/defines.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Closure Boundary Condition Tests
 * 
 * This class contains comprehensive tests for closure boundary conditions
 * as defined in the closure_boundary_implementation.md document.
 * 
 * Tests cover five core boundary checks:
 * 1. Upvalue count limits (MAX_UPVALUES_PER_CLOSURE = 255)
 * 2. Function nesting depth limits (MAX_FUNCTION_NESTING_DEPTH = 200)
 * 3. Upvalue lifecycle boundaries
 * 4. Resource exhaustion handling (MAX_CLOSURE_MEMORY_SIZE = 1MB)
 * 5. Invalid upvalue index access
 */
class ClosureBoundaryTest {
public:
    /**
     * @brief Run all closure boundary condition tests
     * 
     * Executes the complete suite of boundary condition tests for closures,
     * covering all five core boundary checks defined in the implementation.
     */
    static void runAllTests();

private:
    // Test group methods
    static void runUpvalueCountLimitTests();
    static void runNestingDepthLimitTests();
    static void runLifecycleBoundaryTests();
    static void runResourceExhaustionTests();
    static void runInvalidIndexAccessTests();
    static void runIntegrationBoundaryTests();

    // 1. Upvalue count limit tests
    static void testMaxUpvalueCount();
    static void testExcessiveUpvalueCount();
    static void testUpvalueCountValidation();
    static void testRuntimeUpvalueCountCheck();

    // 2. Function nesting depth limit tests
    static void testMaxNestingDepth();
    static void testExcessiveNestingDepth();
    static void testNestingDepthTracking();
    static void testExceptionSafeDepthRecovery();

    // 3. Upvalue lifecycle boundary tests
    static void testUpvalueLifecycleValidation();
    static void testDestroyedUpvalueAccess();
    static void testSafeUpvalueAccess();
    static void testUpvalueStateTransitions();

    // 4. Resource exhaustion tests
    static void testMemoryUsageEstimation();
    static void testMemoryExhaustionRecovery();
    static void testLargeClosureMemoryLimit();
    static void testMemoryAllocationFailure();

    // 5. Invalid upvalue index access tests
    static void testValidUpvalueIndexCheck();
    static void testInvalidUpvalueIndexAccess();
    static void testUpvalueIndexBoundaryValidation();
    static void testNonLuaFunctionUpvalueAccess();

    // Integration and stress tests
    static void testCombinedBoundaryConditions();
    static void testStressBoundaryScenarios();
    static void testBoundaryErrorRecovery();
    static void testPerformanceUnderBoundaryConditions();

    // Helper methods for test execution
    static bool expectCompilationError(const std::string& luaCode, const std::string& expectedError = "");
    static bool expectRuntimeError(const std::string& luaCode, const std::string& expectedError = "");
    static bool executeSuccessfulTest(const std::string& luaCode, const std::string& expectedResult = "");
    static bool executeBoundaryTest(const std::string& luaCode, bool shouldFail = true);

    // Code generation helpers
    static std::string generateCodeWithManyUpvalues(int upvalueCount);
    static std::string generateDeeplyNestedCode(int nestingDepth);
    static std::string generateLargeClosureCode();
    static std::string generateInvalidIndexAccessCode();

    // Test environment management
    static void setupBoundaryTestEnvironment();
    static void cleanupBoundaryTestEnvironment();

    // Error handling and validation
    static bool validateErrorMessage(const std::string& actualError, const std::string& expectedPattern);
    static std::string captureExceptionMessage(const std::function<void()>& operation);
    static void logBoundaryTestResult(const std::string& testName, bool passed, const std::string& details = "");

    // Memory and performance monitoring
    static size_t measureMemoryUsage();
    static void monitorBoundaryPerformance(const std::string& testName, const std::function<void()>& testOperation);

    // Boundary constants validation
    static void validateBoundaryConstants();
    static void testBoundaryConstantConsistency();
};

} // namespace Tests
} // namespace Lua

#endif // LUA_CLOSURE_BOUNDARY_TEST_HPP
