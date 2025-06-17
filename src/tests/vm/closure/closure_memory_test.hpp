#ifndef LUA_CLOSURE_MEMORY_TESTS_HPP
#define LUA_CLOSURE_MEMORY_TESTS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>

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
 * Memory and Lifecycle Tests for Closures
 * 
 * This class contains tests for closure and upvalue memory management,
 * garbage collection behavior, lifecycle management, and memory leak detection.
 */
class ClosureMemoryTest {
public:
    /**
     * Run all memory and lifecycle tests
     */
    static void runAllTests();

private:
    // Memory and lifecycle tests
    static void testClosureLifecycle();
    static void testUpvalueLifecycle();
    static void testGarbageCollection();
    static void testMemoryLeaks();
    static void testUpvalueReferences();
    static void testClosureReferences();
    static void testCircularReferences();
    static void testWeakReferences();
    
    // Memory measurement tests
    static void measureClosureMemoryUsage();
    static void measureUpvalueMemoryUsage();
    static void testMemoryGrowth();
    static void testMemoryFragmentation();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed, const std::string& details = "");
    static void printSectionHeader(const std::string& sectionName);
    static void printSectionFooter();
    static void printMemoryResult(const std::string& testName, size_t memoryBytes);
    
    static bool compileAndExecute(const std::string& luaCode);
    static bool executeClosureTest(const std::string& luaCode, const std::string& expectedResult = "");
    static size_t measureMemoryUsage();
    static void forceGarbageCollection();
    
    static void setupTestEnvironment();
    static void cleanupTestEnvironment();
};

} // namespace Tests
} // namespace Lua

#endif // LUA_CLOSURE_MEMORY_TESTS_HPP