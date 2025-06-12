#ifndef TEST_MAIN_HPP
#define TEST_MAIN_HPP

// Include all test headers
#include "lexer/lexer_test.hpp"
#include "vm/value_test.hpp"
#include "vm/state_test.hpp"
#include "parser/parser_test.hpp"
#include "compiler/symbol_table_test.hpp"
#include "compiler/literal_compiler_test.hpp"
#include "compiler/variable_compiler_test.hpp"
#include "compiler/binary_expression_test.hpp"
#include "compiler/expression_compiler_test.hpp"
#include "compiler/conditional_compilation_test.hpp"
#include "parser/function_test.hpp"
#include "parser/forin_test.hpp"
#include "parser/repeat_test.hpp"
#include "parser/if_statement_test.hpp"
#include "gc/gc_integration_test.hpp"
#include "gc/string_pool_demo_test.hpp"

namespace Lua {
namespace Tests {

// Function to run all tests
void runAllTests();

} // namespace Tests
} // namespace Lua

#endif // TEST_MAIN_HPP