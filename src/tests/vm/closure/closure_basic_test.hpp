#ifndef LUA_CLOSURE_BASIC_TESTS_HPP
#define LUA_CLOSURE_BASIC_TESTS_HPP

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
 * Basic Closure Functionality Tests
 * 
 * This class contains tests for core closure functionality including
 * basic creation, upvalue capture, nested closures, and invocation.
 */
class ClosureBasicTest {
public:
    /**
     * Run all basic closure tests
     */
    static void runAllTests();

private:
    // Core functionality tests
    static void testBasicClosureCreation();
    static void testUpvalueCapture();
    static void testNestedClosures();
    static void testClosureInvocation();
    static void testSimpleUpvalueModification();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed, const std::string& details = "");
    static void printSectionHeader(const std::string& sectionName);
    static void printSectionFooter();
    
    static bool compileAndExecute(const std::string& luaCode);
    static bool executeClosureTest(const std::string& luaCode, const std::string& expectedResult = "");
    
    static void setupTestEnvironment();
    static void cleanupTestEnvironment();
};

} // namespace Tests
} // namespace Lua

#endif // LUA_CLOSURE_BASIC_TESTS_HPP