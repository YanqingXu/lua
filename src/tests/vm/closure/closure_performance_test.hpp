#ifndef LUA_CLOSURE_PERFORMANCE_TESTS_HPP
#define LUA_CLOSURE_PERFORMANCE_TESTS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <functional>

// Include necessary components for testing
#include "../../../lexer/lexer.hpp"
#include "../../../parser/parser.hpp"
#include "../../../compiler/compiler.hpp"
#include "../../../vm/vm.hpp"
#include "../../../vm/state.hpp"
#include "../../../vm/value.hpp"
#include "../../../vm/function.hpp"
#include "../../../vm/upvalue.hpp"

namespace Lua {
namespace Tests {

/**
 * Performance Tests for Closures
 * 
 * This class contains performance benchmarks and analysis for closure
 * functionality including creation overhead, invocation speed, upvalue
 * access performance, and scalability tests.
 */
class ClosurePerformanceTest {
public:
    /**
     * Run all performance tests
     */
    static void runAllTests();

private:
    // Performance benchmark tests
    static void benchmarkClosureCreation();
    static void benchmarkUpvalueAccess();
    static void benchmarkNestedClosures();
    static void benchmarkClosureInvocation();
    static void benchmarkComplexScenarios();
    
    // Scalability tests
    static void testScalability();
    static void testDeepNesting();
    static void testManyUpvalues();
    static void testLargeClosureCount();
    
    // Comparison tests
    static void comparePerformance();
    static void compareWithRegularFunctions();
    static void compareUpvalueVsGlobal();
    static void compareNestedVsFlat();
    
    // Memory performance tests
    static void measureMemoryPerformance();
    static void testMemoryAllocationSpeed();
    static void testGarbageCollectionImpact();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed, const std::string& details = "");
    static void printSectionHeader(const std::string& sectionName);
    static void printSectionFooter();
    static void printPerformanceResult(const std::string& testName, double timeMs, const std::string& unit = "ms");
    static void printThroughputResult(const std::string& testName, double operationsPerSecond);
    
    static bool compileAndExecute(const std::string& luaCode);
    static bool executePerformanceTest(const std::string& luaCode, int iterations);
    static double measureExecutionTime(const std::function<void()>& operation);
    static double measureAverageTime(const std::function<void()>& operation, int iterations);
    
    static void setupTestEnvironment();
    static void cleanupTestEnvironment();
    
    // Performance thresholds (in milliseconds)
    static constexpr double CLOSURE_CREATION_THRESHOLD = 1.0;
    static constexpr double UPVALUE_ACCESS_THRESHOLD = 0.1;
    static constexpr double NESTED_CLOSURE_THRESHOLD = 2.0;
    static constexpr double INVOCATION_THRESHOLD = 0.05;
    static constexpr double SCALABILITY_THRESHOLD = 10.0;
};

} // namespace Tests
} // namespace Lua

#endif // LUA_CLOSURE_PERFORMANCE_TESTS_HPP