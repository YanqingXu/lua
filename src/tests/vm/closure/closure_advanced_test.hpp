#ifndef LUA_CLOSURE_ADVANCED_TESTS_HPP
#define LUA_CLOSURE_ADVANCED_TESTS_HPP

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
 * Advanced Closure Functionality Tests
 * 
 * This class contains tests for advanced closure scenarios including
 * complex nesting, multiple upvalues, closures as parameters/return values,
 * and sophisticated upvalue manipulation.
 */
class ClosureAdvancedTest {
public:
    /**
     * Run all advanced closure tests
     */
    static void runAllTests();

private:
    // Advanced scenario tests
    static void testMultipleUpvalues();
    static void testComplexUpvalueModification();
    static void testClosureAsParameter();
    static void testClosureAsReturnValue();
    static void testComplexNesting();
    static void testClosureChaining();
    static void testUpvalueSharing();
    static void testRecursiveClosures();
    
    // Helper methods
    
    static bool compileAndExecute(const std::string& luaCode);
    static bool executeClosureTest(const std::string& luaCode, const std::string& expectedResult = "");
};

} // namespace Tests
} // namespace Lua

#endif // LUA_CLOSURE_ADVANCED_TESTS_HPP