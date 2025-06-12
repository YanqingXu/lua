#ifndef TEST_MAIN_HPP
#define TEST_MAIN_HPP

// Include all test headers
#include "lexer/lexer_test.hpp"
#include "vm/value_test.hpp"
#include "vm/state_test.hpp"
#include "parser/parser_test.hpp"
#include "compiler/symbol_table_test.hpp"

namespace Lua {
namespace Tests {

// Function to run all tests
void runAllTests();

} // namespace Tests
} // namespace Lua

#endif // TEST_MAIN_HPP