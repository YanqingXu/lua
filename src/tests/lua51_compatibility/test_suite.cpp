#include "test_suite.hpp"
#include "../../vm/lua_state.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace Lua {
namespace Testing {

    // Lua51CompatibilityTestSuite Implementation
    
    Lua51CompatibilityTestSuite::Lua51CompatibilityTestSuite()
        : testState_(nullptr)
    {
        initializeTestSuite_();
    }
    
    Lua51CompatibilityTestSuite::~Lua51CompatibilityTestSuite() {
        cleanupTestSuite_();
    }
    
    bool Lua51CompatibilityTestSuite::runAllTests() {
        std::cout << "=== Running Lua 5.1 Compatibility Test Suite ===" << std::endl;
        std::cout << "Total registered tests: " << registeredTests_.size() << std::endl;
        
        results_.clear();
        bool allPassed = true;
        
        for (const auto& test : registeredTests_) {
            std::cout << "Running: " << test.name << "... ";
            
            auto start = std::chrono::high_resolution_clock::now();
            TestResult result = test.function();
            auto end = std::chrono::high_resolution_clock::now();
            
            result.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
            results_.push_back(result);
            
            if (result.passed) {
                std::cout << "PASS (" << std::fixed << std::setprecision(2) 
                         << result.executionTime << "ms)" << std::endl;
            } else {
                std::cout << "FAIL - " << result.errorMessage << std::endl;
                allPassed = false;
            }
        }
        
        // Print summary
        i32 total, passed, failed;
        getStatistics(total, passed, failed);
        
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total: " << total << ", Passed: " << passed 
                  << ", Failed: " << failed << std::endl;
        std::cout << "Compatibility: " << std::fixed << std::setprecision(1) 
                  << calculateCompatibilityPercentage() << "%" << std::endl;
        
        return allPassed;
    }
    
    bool Lua51CompatibilityTestSuite::runCategoryTests(TestCategory category) {
        std::cout << "=== Running " << categoryToString_(category) << " Tests ===" << std::endl;
        
        bool allPassed = true;
        i32 categoryTests = 0;
        
        for (const auto& test : registeredTests_) {
            if (test.category == category) {
                categoryTests++;
                std::cout << "Running: " << test.name << "... ";
                
                TestResult result = test.function();
                results_.push_back(result);
                
                if (result.passed) {
                    std::cout << "PASS" << std::endl;
                } else {
                    std::cout << "FAIL - " << result.errorMessage << std::endl;
                    allPassed = false;
                }
            }
        }
        
        std::cout << "Category tests completed: " << categoryTests << std::endl;
        return allPassed;
    }
    
    TestResult Lua51CompatibilityTestSuite::runSingleTest(const std::string& testName) {
        for (const auto& test : registeredTests_) {
            if (test.name == testName) {
                return test.function();
            }
        }
        
        return TestResult(testName, false, "Test not found");
    }
    
    void Lua51CompatibilityTestSuite::registerTest(const std::string& name, TestCategory category, TestFunction testFunc) {
        registeredTests_.emplace_back(name, category, testFunc);
    }
    
    std::string Lua51CompatibilityTestSuite::generateReport() const {
        std::ostringstream report;
        
        report << "=== Lua 5.1 Compatibility Test Report ===\n\n";
        
        // Overall statistics
        i32 total, passed, failed;
        getStatistics(total, passed, failed);
        
        report << "Overall Statistics:\n";
        report << "  Total Tests: " << total << "\n";
        report << "  Passed: " << passed << "\n";
        report << "  Failed: " << failed << "\n";
        report << "  Compatibility: " << std::fixed << std::setprecision(1) 
               << calculateCompatibilityPercentage() << "%\n\n";
        
        // Category breakdown
        report << "Category Breakdown:\n";
        for (i32 cat = 0; cat < 8; ++cat) {
            TestCategory category = static_cast<TestCategory>(cat);
            i32 catTotal = 0, catPassed = 0;
            
            for (const auto& result : results_) {
                // Find corresponding test entry to get category
                for (const auto& test : registeredTests_) {
                    if (test.name == result.testName && test.category == category) {
                        catTotal++;
                        if (result.passed) catPassed++;
                        break;
                    }
                }
            }
            
            if (catTotal > 0) {
                double catPercentage = (catTotal > 0) ? (100.0 * catPassed / catTotal) : 0.0;
                report << "  " << categoryToString_(category) << ": " 
                       << catPassed << "/" << catTotal << " (" 
                       << std::fixed << std::setprecision(1) << catPercentage << "%)\n";
            }
        }
        
        // Failed tests details
        if (failed > 0) {
            report << "\nFailed Tests:\n";
            for (const auto& result : results_) {
                if (!result.passed) {
                    report << "  " << result.testName << ": " << result.errorMessage << "\n";
                }
            }
        }
        
        return report.str();
    }
    
    double Lua51CompatibilityTestSuite::calculateCompatibilityPercentage() const {
        if (results_.empty()) return 0.0;
        
        i32 passed = 0;
        for (const auto& result : results_) {
            if (result.passed) passed++;
        }
        
        return 100.0 * passed / results_.size();
    }
    
    void Lua51CompatibilityTestSuite::getStatistics(i32& totalTests, i32& passedTests, i32& failedTests) const {
        totalTests = static_cast<i32>(results_.size());
        passedTests = 0;
        failedTests = 0;
        
        for (const auto& result : results_) {
            if (result.passed) {
                passedTests++;
            } else {
                failedTests++;
            }
        }
    }
    
    void Lua51CompatibilityTestSuite::initializeTestSuite_() {
        testState_ = createTestState();
        registerAllTests_();
    }
    
    void Lua51CompatibilityTestSuite::cleanupTestSuite_() {
        if (testState_) {
            cleanupTestState(testState_);
            testState_ = nullptr;
        }
    }
    
    void Lua51CompatibilityTestSuite::registerAllTests_() {
        registerStackOperationTests_();
        registerTableOperationTests_();
        registerFunctionCallTests_();
        registerErrorHandlingTests_();
        registerDebugHookTests_();
        registerMemoryManagementTests_();
        registerPerformanceTests_();
        registerRegressionTests_();
    }
    
    std::string Lua51CompatibilityTestSuite::categoryToString_(TestCategory category) const {
        switch (category) {
            case TestCategory::STACK_OPERATIONS: return "Stack Operations";
            case TestCategory::TABLE_OPERATIONS: return "Table Operations";
            case TestCategory::FUNCTION_CALLS: return "Function Calls";
            case TestCategory::ERROR_HANDLING: return "Error Handling";
            case TestCategory::DEBUG_HOOKS: return "Debug Hooks";
            case TestCategory::MEMORY_MANAGEMENT: return "Memory Management";
            case TestCategory::PERFORMANCE: return "Performance";
            case TestCategory::REGRESSION: return "Regression";
            default: return "Unknown";
        }
    }
    
    void Lua51CompatibilityTestSuite::registerStackOperationTests_() {
        registerTest("Stack Push/Pop", TestCategory::STACK_OPERATIONS, StackOperationTests::testPushPop);
        registerTest("Stack Manipulation", TestCategory::STACK_OPERATIONS, StackOperationTests::testStackManipulation);
        registerTest("Type Checking", TestCategory::STACK_OPERATIONS, StackOperationTests::testTypeChecking);
        registerTest("Stack Overflow", TestCategory::STACK_OPERATIONS, StackOperationTests::testStackOverflow);
        registerTest("Stack Underflow", TestCategory::STACK_OPERATIONS, StackOperationTests::testStackUnderflow);
        registerTest("Stack Resize", TestCategory::STACK_OPERATIONS, StackOperationTests::testStackResize);
    }
    
    void Lua51CompatibilityTestSuite::registerTableOperationTests_() {
        registerTest("Table Creation", TestCategory::TABLE_OPERATIONS, TableOperationTests::testTableCreation);
        registerTest("Table Access", TestCategory::TABLE_OPERATIONS, TableOperationTests::testTableAccess);
        registerTest("Table Modification", TestCategory::TABLE_OPERATIONS, TableOperationTests::testTableModification);
        registerTest("Metatables", TestCategory::TABLE_OPERATIONS, TableOperationTests::testMetatables);
        registerTest("Raw Operations", TestCategory::TABLE_OPERATIONS, TableOperationTests::testRawOperations);
        registerTest("Table Traversal", TestCategory::TABLE_OPERATIONS, TableOperationTests::testTableTraversal);
    }
    
    void Lua51CompatibilityTestSuite::registerFunctionCallTests_() {
        registerTest("Basic Calls", TestCategory::FUNCTION_CALLS, FunctionCallTests::testBasicCalls);
        registerTest("Protected Calls", TestCategory::FUNCTION_CALLS, FunctionCallTests::testProtectedCalls);
        registerTest("C Function Calls", TestCategory::FUNCTION_CALLS, FunctionCallTests::testCFunctionCalls);
        registerTest("Coroutines", TestCategory::FUNCTION_CALLS, FunctionCallTests::testCoroutines);
        registerTest("Tail Calls", TestCategory::FUNCTION_CALLS, FunctionCallTests::testTailCalls);
        registerTest("Multiple Returns", TestCategory::FUNCTION_CALLS, FunctionCallTests::testMultipleReturns);
    }
    
    void Lua51CompatibilityTestSuite::registerErrorHandlingTests_() {
        registerTest("Error Throw", TestCategory::ERROR_HANDLING, ErrorHandlingTests::testErrorThrow);
        registerTest("Error Catch", TestCategory::ERROR_HANDLING, ErrorHandlingTests::testErrorCatch);
        registerTest("Error Propagation", TestCategory::ERROR_HANDLING, ErrorHandlingTests::testErrorPropagation);
        registerTest("Panic Function", TestCategory::ERROR_HANDLING, ErrorHandlingTests::testPanicFunction);
        registerTest("Error Messages", TestCategory::ERROR_HANDLING, ErrorHandlingTests::testErrorMessages);
        registerTest("Error Recovery", TestCategory::ERROR_HANDLING, ErrorHandlingTests::testErrorRecovery);
    }
    
    void Lua51CompatibilityTestSuite::registerDebugHookTests_() {
        registerTest("Hook Registration", TestCategory::DEBUG_HOOKS, DebugHookTests::testHookRegistration);
        registerTest("Call Hooks", TestCategory::DEBUG_HOOKS, DebugHookTests::testCallHooks);
        registerTest("Return Hooks", TestCategory::DEBUG_HOOKS, DebugHookTests::testReturnHooks);
        registerTest("Line Hooks", TestCategory::DEBUG_HOOKS, DebugHookTests::testLineHooks);
        registerTest("Count Hooks", TestCategory::DEBUG_HOOKS, DebugHookTests::testCountHooks);
        registerTest("Debug Info", TestCategory::DEBUG_HOOKS, DebugHookTests::testDebugInfo);
    }
    
    void Lua51CompatibilityTestSuite::registerMemoryManagementTests_() {
        registerTest("Garbage Collection", TestCategory::MEMORY_MANAGEMENT, MemoryManagementTests::testGarbageCollection);
        registerTest("Memory Leaks", TestCategory::MEMORY_MANAGEMENT, MemoryManagementTests::testMemoryLeaks);
        registerTest("Large Allocations", TestCategory::MEMORY_MANAGEMENT, MemoryManagementTests::testLargeAllocations);
        registerTest("Fragmentation", TestCategory::MEMORY_MANAGEMENT, MemoryManagementTests::testFragmentation);
        registerTest("GC Pressure", TestCategory::MEMORY_MANAGEMENT, MemoryManagementTests::testGCPressure);
        registerTest("Weak References", TestCategory::MEMORY_MANAGEMENT, MemoryManagementTests::testWeakReferences);
    }
    
    void Lua51CompatibilityTestSuite::registerPerformanceTests_() {
        registerTest("VM Execution Speed", TestCategory::PERFORMANCE, PerformanceTests::testVMExecutionSpeed);
        registerTest("Memory Allocation Speed", TestCategory::PERFORMANCE, PerformanceTests::testMemoryAllocationSpeed);
        registerTest("Table Operation Speed", TestCategory::PERFORMANCE, PerformanceTests::testTableOperationSpeed);
        registerTest("Function Call Speed", TestCategory::PERFORMANCE, PerformanceTests::testFunctionCallSpeed);
        registerTest("Debug Hook Overhead", TestCategory::PERFORMANCE, PerformanceTests::testDebugHookOverhead);
        registerTest("GC Performance", TestCategory::PERFORMANCE, PerformanceTests::testGCPerformance);
    }
    
    void Lua51CompatibilityTestSuite::registerRegressionTests_() {
        registerTest("Phase 1 Compatibility", TestCategory::REGRESSION, RegressionTests::testPhase1Compatibility);
        registerTest("Phase 2 Compatibility", TestCategory::REGRESSION, RegressionTests::testPhase2Compatibility);
        registerTest("Basic Arithmetic", TestCategory::REGRESSION, RegressionTests::testBasicArithmetic);
        registerTest("String Operations", TestCategory::REGRESSION, RegressionTests::testStringOperations);
        registerTest("Control Flow", TestCategory::REGRESSION, RegressionTests::testControlFlow);
        registerTest("Library Functions", TestCategory::REGRESSION, RegressionTests::testLibraryFunctions);
    }

} // namespace Testing
} // namespace Lua
