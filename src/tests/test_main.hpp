#ifndef TEST_MAIN_HPP
#define TEST_MAIN_HPP

// Include all test headers
#include "lexer/lexer_test.hpp"
#include "vm/test_vm.hpp"
#include "parser/test_parser.hpp"
#include "compiler/test_compiler.hpp"
#include "gc/test_gc.hpp"
#include "lib/test_lib.hpp"
#include "localization/localization_test.hpp"

namespace Lua {
namespace Tests {

// Function to run all tests
void runAllTests();

} // namespace Tests
} // namespace Lua

#endif // TEST_MAIN_HPP