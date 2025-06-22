#pragma once

#include "../../vm/vm.hpp"
#include "../../vm/value.hpp"
#include "../../gc/core/garbage_collector.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Lua::Tests {

/**
 * @brief Garbage collector error handling test suite
 * 
 * Tests the garbage collector's error handling capabilities including:
 * - Memory allocation failures
 * - Circular reference handling
 * - Collection during critical operations
 * - Memory pressure scenarios
 * - Finalizer errors
 * - Collection timing issues
 */
class GCErrorTestSuite {
public:
    /**
     * @brief Run all GC error handling tests
     * 
     * Executes all test groups in this suite using the standardized
     * test framework for consistent formatting and error handling.
     */
    static void runAllTests();

private:
    // Test groups
    static void testMemoryAllocationErrors();
    static void testCircularReferenceHandling();
    static void testCollectionErrors();
    static void testMemoryPressureScenarios();
    static void testFinalizerErrors();
    static void testCollectionTiming();
    
    // Memory allocation error tests
    static void testOutOfMemoryDuringAllocation();
    static void testAllocationFailureRecovery();
    static void testMemoryFragmentation();
    static void testLargeObjectAllocation();
    
    // Circular reference tests
    static void testSimpleCircularReferences();
    static void testComplexCircularReferences();
    static void testWeakReferenceHandling();
    static void testCircularReferencesWithFinalizers();
    
    // Collection error tests
    static void testCollectionDuringAllocation();
    static void testCollectionInterruption();
    static void testIncrementalCollectionErrors();
    static void testFullCollectionErrors();
    
    // Memory pressure tests
    static void testLowMemoryScenarios();
    static void testMemoryThresholdHandling();
    static void testEmergencyCollection();
    static void testMemoryExhaustionRecovery();
    
    // Finalizer error tests
    static void testFinalizerExceptions();
    static void testFinalizerInfiniteLoop();
    static void testFinalizerMemoryAllocation();
    static void testFinalizerOrderingErrors();
    
    // Collection timing tests
    static void testConcurrentAccess();
    static void testCollectionDuringExecution();
    static void testTimingRaceConditions();
    static void testCollectionFrequencyErrors();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed);
    static bool simulateMemoryPressure(size_t targetMemory);
    static bool createCircularReferences(int count);
    static bool testGCUnderLoad();
    static bool checkMemoryLeaks();
    static bool forceGCCollection();
    static size_t getCurrentMemoryUsage();
    static bool simulateAllocationFailure();
};

} // namespace Lua::Tests