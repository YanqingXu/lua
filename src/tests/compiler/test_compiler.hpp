#pragma once

// Include all compiler test headers
#include "symbol_table_test.hpp"
#include "literal_compiler_test.hpp"
#include "variable_compiler_test.hpp"
#include "binary_expression_test.hpp"
#include "expression_compiler_test.hpp"
#include "conditional_compilation_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Compiler Test Suite
 * 
 * This class provides a unified interface to run all compiler-related tests.
 * It includes tests for symbol table, literal compilation, variable compilation,
 * binary expressions, expression compilation, and conditional compilation.
 */
class CompilerTest {
public:
    /**
     * @brief Run all compiler tests
     * 
     * Executes all compiler-related test suites in a logical order.
     * Tests are run from basic components to more complex compilation features.
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
