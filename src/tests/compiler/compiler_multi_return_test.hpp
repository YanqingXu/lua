#pragma once

#include "../../test_framework/core/test_macros.hpp"
#include "../../compiler/compiler.hpp"
#include "../../parser/parser.hpp"
#include "../../lexer/lexer.hpp"
#include "../../vm/vm.hpp"
#include "../../vm/state_factory.hpp"
#include <iostream>
#include <cassert>

namespace Lua::Tests {

/**
 * @brief Multi Return Value Compilation Test Suite
 * 
 * Tests compilation of return statements with multiple values.
 * This is a sub-feature test for the compiler module.
 */
class CompilerMultiReturnTest {
public:
    /**
     * @brief Run all multi return compilation tests
     */
    static void runAllTests();

private:
    /**
     * @brief Test single return value compilation
     */
    static void testSingleReturnCompilation();

    /**
     * @brief Test multiple return value compilation
     */
    static void testMultipleReturnCompilation();

    /**
     * @brief Test empty return compilation
     */
    static void testEmptyReturnCompilation();

    /**
     * @brief Test complex return expression compilation
     */
    static void testComplexReturnCompilation();

    /**
     * @brief Helper method to test return statement compilation
     * @param code The Lua code to compile
     * @param description Description of the test case
     */
    static void testReturnCompilation(const Str& code, const Str& description);
};

} // namespace Lua::Tests