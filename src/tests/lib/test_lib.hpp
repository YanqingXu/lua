#pragma once

#include "../../common/types.hpp"
#include "base_lib_test.hpp"
#include "table_lib_test.hpp"
#include "string_lib_test.hpp"
#include "math_lib_test.hpp"

#include <iostream>

namespace Lua::Tests {

/**
 * @brief Standard library test suite
 * 
 * Coordinates all standard library related tests, including:
 * - BaseLib: Basic library tests
 * - StringLib: String library tests  
 * - TableLib: Table library tests
 * - MathLib: Math library tests
 * - IOLib: IO library tests (future)
 */
class LibTestSuite {
public:
    /**
     * @brief Run all standard library tests
     * 
     * Execute all test suites in the standard library module
     */
    static void runAllTests() {
        RUN_TEST_SUITE(BaseLibTestSuite);
        //RUN_TEST_SUITE(TableLibTest);
        //RUN_TEST_SUITE(StringLibTest);
        //RUN_TEST_SUITE(MathLibTest);
        
        // TODO: Add other library test suites here when available
        // RUN_TEST_SUITE(IOLibTestSuite);
    }
};

} // namespace Lua::Tests
