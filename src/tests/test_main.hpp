#ifndef TEST_MAIN_HPP
#define TEST_MAIN_HPP

// Include all test headers
#include "lexer_test.hpp"
#include "value_test.hpp"
#include "state_test.hpp"
#include "parser_test.hpp"
#include "symbol_table_test.hpp"

namespace Lua {
namespace Tests {

// Function to run all tests
void runAllTests();

// Demo functions moved from main.cpp
void runStringPoolDemo();
void runGCIntegrationDemo();

} // namespace Tests
} // namespace Lua

#endif // TEST_MAIN_HPP