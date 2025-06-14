#pragma once

// Include all parser test headers
#include "parser_test.hpp"
#include "function_test.hpp"
#include "forin_test.hpp"
#include "repeat_test.hpp"
#include "if_statement_test.hpp"
#include "source_location_test.hpp"
#include "parse_error_test.hpp"
#include "error_reporter_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Parser Test Suite
 * 
 * This class provides a unified interface to run all parser-related tests.
 * It includes tests for basic parsing, function definitions, control structures,
 * and various statement types.
 */
class ParserTestSuite {
public:
    /**
     * @brief Run all parser tests
     * 
     * Executes all parser-related test suites in a logical order.
     * Tests are run from basic parsing to complex language constructs.
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
