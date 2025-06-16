#pragma once

// Include all GC test headers
#include "gc_integration_test.hpp"
#include "string_pool_demo_test.hpp"
#include "garbage_collector_test.hpp"
#include <string>

namespace Lua {
namespace Tests {

/**
 * @brief Garbage Collector Test Suite
 * 
 * This class provides a unified interface to run all garbage collector-related tests.
 * It includes tests for GC integration, string pool management, and memory management.
 */
class GCTest {
public:
    /**
     * @brief Run all GC tests
     * 
     * Executes all garbage collector-related test suites in a logical order.
     * Tests are run from basic string pool functionality to complex GC integration.
     */
    static void runAllTests();
    
private:
    /**
     * @brief Print section header for test organization
     * @param sectionName Name of the test section
     */
    static void printSectionHeader(const std::string& sectionName);
    
    /**
     * @brief Print section footer
     */
    static void printSectionFooter();
};

} // namespace Tests
} // namespace Lua
