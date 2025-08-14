#pragma once

#include "../../common/types.hpp"
#include "../../vm/lua_state.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Lua {
namespace Testing {

    /**
     * @brief Lua 5.1 Compatibility Test Suite
     * 
     * Comprehensive testing framework for verifying Lua 5.1 compatibility
     * across all implemented features including stack operations, table operations,
     * function calls, error handling, and debug hooks.
     */
    
    // Test result structure
    struct TestResult {
        std::string testName;
        bool passed;
        std::string errorMessage;
        double executionTime; // in milliseconds
        
        TestResult(const std::string& name, bool success, const std::string& error = "", double time = 0.0)
            : testName(name), passed(success), errorMessage(error), executionTime(time) {}
    };
    
    // Test category enumeration
    enum class TestCategory {
        STACK_OPERATIONS,
        TABLE_OPERATIONS,
        FUNCTION_CALLS,
        ERROR_HANDLING,
        DEBUG_HOOKS,
        MEMORY_MANAGEMENT,
        PERFORMANCE,
        REGRESSION
    };
    
    // Test function type
    typedef std::function<TestResult()> TestFunction;
    
    /**
     * @brief Main Test Suite Manager
     */
    class Lua51CompatibilityTestSuite {
    public:
        Lua51CompatibilityTestSuite();
        ~Lua51CompatibilityTestSuite();
        
        // Test execution
        /**
         * @brief Run all compatibility tests
         * @return bool True if all tests pass
         */
        bool runAllTests();
        
        /**
         * @brief Run tests for specific category
         * @param category Test category to run
         * @return bool True if all tests in category pass
         */
        bool runCategoryTests(TestCategory category);
        
        /**
         * @brief Run single test by name
         * @param testName Name of test to run
         * @return TestResult Test execution result
         */
        TestResult runSingleTest(const std::string& testName);
        
        // Test registration
        /**
         * @brief Register a test function
         * @param name Test name
         * @param category Test category
         * @param testFunc Test function
         */
        void registerTest(const std::string& name, TestCategory category, TestFunction testFunc);
        
        // Results and reporting
        /**
         * @brief Get all test results
         * @return std::vector<TestResult> All test results
         */
        const std::vector<TestResult>& getResults() const { return results_; }
        
        /**
         * @brief Generate detailed test report
         * @return std::string Formatted test report
         */
        std::string generateReport() const;
        
        /**
         * @brief Calculate overall compatibility percentage
         * @return double Compatibility percentage (0.0 - 100.0)
         */
        double calculateCompatibilityPercentage() const;
        
        /**
         * @brief Get test statistics
         * @param totalTests Output: total number of tests
         * @param passedTests Output: number of passed tests
         * @param failedTests Output: number of failed tests
         */
        void getStatistics(i32& totalTests, i32& passedTests, i32& failedTests) const;
        
    private:
        struct TestEntry {
            std::string name;
            TestCategory category;
            TestFunction function;
            
            TestEntry(const std::string& n, TestCategory c, TestFunction f)
                : name(n), category(c), function(f) {}
        };
        
        std::vector<TestEntry> registeredTests_;
        std::vector<TestResult> results_;
        LuaState* testState_;
        
        // Internal helper methods
        void initializeTestSuite_();
        void cleanupTestSuite_();
        void registerAllTests_();
        std::string categoryToString_(TestCategory category) const;
        
        // Test category implementations
        void registerStackOperationTests_();
        void registerTableOperationTests_();
        void registerFunctionCallTests_();
        void registerErrorHandlingTests_();
        void registerDebugHookTests_();
        void registerMemoryManagementTests_();
        void registerPerformanceTests_();
        void registerRegressionTests_();
    };
    
    /**
     * @brief Stack Operations Test Class
     */
    class StackOperationTests {
    public:
        static TestResult testPushPop();
        static TestResult testStackManipulation();
        static TestResult testTypeChecking();
        static TestResult testStackOverflow();
        static TestResult testStackUnderflow();
        static TestResult testStackResize();
    };
    
    /**
     * @brief Table Operations Test Class
     */
    class TableOperationTests {
    public:
        static TestResult testTableCreation();
        static TestResult testTableAccess();
        static TestResult testTableModification();
        static TestResult testMetatables();
        static TestResult testRawOperations();
        static TestResult testTableTraversal();
    };
    
    /**
     * @brief Function Call Test Class
     */
    class FunctionCallTests {
    public:
        static TestResult testBasicCalls();
        static TestResult testProtectedCalls();
        static TestResult testCFunctionCalls();
        static TestResult testCoroutines();
        static TestResult testTailCalls();
        static TestResult testMultipleReturns();
    };
    
    /**
     * @brief Error Handling Test Class
     */
    class ErrorHandlingTests {
    public:
        static TestResult testErrorThrow();
        static TestResult testErrorCatch();
        static TestResult testErrorPropagation();
        static TestResult testPanicFunction();
        static TestResult testErrorMessages();
        static TestResult testErrorRecovery();
    };
    
    /**
     * @brief Debug Hooks Test Class
     */
    class DebugHookTests {
    public:
        static TestResult testHookRegistration();
        static TestResult testCallHooks();
        static TestResult testReturnHooks();
        static TestResult testLineHooks();
        static TestResult testCountHooks();
        static TestResult testDebugInfo();
    };
    
    /**
     * @brief Memory Management Test Class
     */
    class MemoryManagementTests {
    public:
        static TestResult testGarbageCollection();
        static TestResult testMemoryLeaks();
        static TestResult testLargeAllocations();
        static TestResult testFragmentation();
        static TestResult testGCPressure();
        static TestResult testWeakReferences();
    };
    
    /**
     * @brief Performance Test Class
     */
    class PerformanceTests {
    public:
        static TestResult testVMExecutionSpeed();
        static TestResult testMemoryAllocationSpeed();
        static TestResult testTableOperationSpeed();
        static TestResult testFunctionCallSpeed();
        static TestResult testDebugHookOverhead();
        static TestResult testGCPerformance();
    };
    
    /**
     * @brief Regression Test Class
     */
    class RegressionTests {
    public:
        static TestResult testPhase1Compatibility();
        static TestResult testPhase2Compatibility();
        static TestResult testBasicArithmetic();
        static TestResult testStringOperations();
        static TestResult testControlFlow();
        static TestResult testLibraryFunctions();
    };
    
    // Global test utilities
    /**
     * @brief Create test Lua state with standard configuration
     * @return LuaState* Test Lua state
     */
    LuaState* createTestState();
    
    /**
     * @brief Cleanup test Lua state
     * @param state Lua state to cleanup
     */
    void cleanupTestState(LuaState* state);
    
    /**
     * @brief Execute Lua code and capture result
     * @param state Lua state
     * @param code Lua code to execute
     * @param expectedResult Expected result (optional)
     * @return TestResult Test execution result
     */
    TestResult executeLuaCode(LuaState* state, const std::string& code, const std::string& expectedResult = "");
    
    /**
     * @brief Measure execution time of a function
     * @param func Function to measure
     * @return double Execution time in milliseconds
     */
    double measureExecutionTime(std::function<void()> func);
    
    /**
     * @brief Compare floating point values with tolerance
     * @param a First value
     * @param b Second value
     * @param tolerance Comparison tolerance
     * @return bool True if values are approximately equal
     */
    bool approximatelyEqual(double a, double b, double tolerance = 1e-9);
    
    /**
     * @brief Format test result for display
     * @param result Test result
     * @return std::string Formatted result string
     */
    std::string formatTestResult(const TestResult& result);
    
} // namespace Testing
} // namespace Lua
