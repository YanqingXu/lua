#include "test_suite.hpp"
#include "performance_benchmark.hpp"
#include <iostream>
#include <iomanip>

using namespace Lua::Testing;

/**
 * @brief Main test program for Lua 5.1 compatibility validation
 * 
 * This program runs comprehensive tests to validate our Lua 5.1 implementation
 * and measures performance against established benchmarks.
 */

void printHeader() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Lua 5.1 Compatibility Test Suite" << std::endl;
    std::cout << "  Phase 3 - Week 10 Validation" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
}

void printSeparator() {
    std::cout << "----------------------------------------" << std::endl;
}

bool runCompatibilityTests() {
    std::cout << "=== COMPATIBILITY TESTS ===" << std::endl;
    std::cout << std::endl;
    
    Lua51CompatibilityTestSuite testSuite;
    bool allTestsPassed = testSuite.runAllTests();
    
    std::cout << std::endl;
    std::cout << "=== COMPATIBILITY TEST REPORT ===" << std::endl;
    std::cout << testSuite.generateReport() << std::endl;
    
    return allTestsPassed;
}

bool runPerformanceBenchmarks() {
    std::cout << "=== PERFORMANCE BENCHMARKS ===" << std::endl;
    std::cout << std::endl;
    
    PerformanceBenchmarkSuite benchmarkSuite;
    bool allBenchmarksPassed = benchmarkSuite.runAllBenchmarks();
    
    std::cout << std::endl;
    std::cout << "=== PERFORMANCE BENCHMARK REPORT ===" << std::endl;
    std::cout << benchmarkSuite.generatePerformanceReport() << std::endl;
    
    return allBenchmarksPassed;
}

void runCategoryTests() {
    std::cout << "=== CATEGORY-SPECIFIC TESTS ===" << std::endl;
    std::cout << std::endl;
    
    Lua51CompatibilityTestSuite testSuite;
    
    // Test each category individually
    std::vector<TestCategory> categories = {
        TestCategory::STACK_OPERATIONS,
        TestCategory::TABLE_OPERATIONS,
        TestCategory::FUNCTION_CALLS,
        TestCategory::ERROR_HANDLING,
        TestCategory::DEBUG_HOOKS,
        TestCategory::MEMORY_MANAGEMENT,
        TestCategory::PERFORMANCE,
        TestCategory::REGRESSION
    };
    
    for (TestCategory category : categories) {
        printSeparator();
        bool categoryPassed = testSuite.runCategoryTests(category);
        std::cout << "Category result: " << (categoryPassed ? "PASS" : "FAIL") << std::endl;
        std::cout << std::endl;
    }
}

void printFinalSummary(bool compatibilityPassed, bool performancePassed) {
    std::cout << "========================================" << std::endl;
    std::cout << "           FINAL SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Compatibility Tests: " << (compatibilityPassed ? "PASS" : "FAIL") << std::endl;
    std::cout << "Performance Tests:   " << (performancePassed ? "PASS" : "FAIL") << std::endl;
    std::cout << std::endl;
    
    if (compatibilityPassed && performancePassed) {
        std::cout << "🎉 ALL TESTS PASSED! 🎉" << std::endl;
        std::cout << "Lua 5.1 compatibility target achieved!" << std::endl;
    } else {
        std::cout << "❌ SOME TESTS FAILED" << std::endl;
        std::cout << "Please review the test reports above." << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
}

int main(int argc, char* argv[]) {
    printHeader();
    
    bool runAll = true;
    bool runCompatibility = false;
    bool runPerformance = false;
    bool runCategories = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--compatibility" || arg == "-c") {
            runCompatibility = true;
            runAll = false;
        } else if (arg == "--performance" || arg == "-p") {
            runPerformance = true;
            runAll = false;
        } else if (arg == "--categories" || arg == "--cat") {
            runCategories = true;
            runAll = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --compatibility, -c    Run only compatibility tests" << std::endl;
            std::cout << "  --performance, -p      Run only performance benchmarks" << std::endl;
            std::cout << "  --categories, --cat    Run category-specific tests" << std::endl;
            std::cout << "  --help, -h             Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "If no options are specified, all tests will be run." << std::endl;
            return 0;
        }
    }
    
    bool compatibilityPassed = true;
    bool performancePassed = true;
    
    try {
        if (runAll) {
            // Run all tests
            compatibilityPassed = runCompatibilityTests();
            printSeparator();
            performancePassed = runPerformanceBenchmarks();
            printSeparator();
            runCategoryTests();
        } else {
            // Run specific test types
            if (runCompatibility) {
                compatibilityPassed = runCompatibilityTests();
            }
            if (runPerformance) {
                performancePassed = runPerformanceBenchmarks();
            }
            if (runCategories) {
                runCategoryTests();
            }
        }
        
        printFinalSummary(compatibilityPassed, performancePassed);
        
        // Return appropriate exit code
        if (compatibilityPassed && performancePassed) {
            return 0; // Success
        } else {
            return 1; // Failure
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during test execution: " << e.what() << std::endl;
        return 2; // Error
    } catch (...) {
        std::cerr << "Unknown error during test execution" << std::endl;
        return 2; // Error
    }
}

// Additional utility functions for test main

void printTestProgress(const std::string& testName, bool passed) {
    std::cout << std::left << std::setw(40) << testName;
    std::cout << " [" << (passed ? "PASS" : "FAIL") << "]" << std::endl;
}

void printBenchmarkProgress(const std::string& benchmarkName, double opsPerSec, bool passed) {
    std::cout << std::left << std::setw(30) << benchmarkName;
    std::cout << std::right << std::setw(12) << std::fixed << std::setprecision(0) << opsPerSec;
    std::cout << " ops/sec [" << (passed ? "PASS" : "FAIL") << "]" << std::endl;
}

void printMemoryUsage(double memoryMB) {
    std::cout << "Memory Usage: " << std::fixed << std::setprecision(2) << memoryMB << " MB" << std::endl;
}

void printCompatibilityPercentage(double percentage) {
    std::cout << "Lua 5.1 Compatibility: " << std::fixed << std::setprecision(1) << percentage << "%" << std::endl;
}
