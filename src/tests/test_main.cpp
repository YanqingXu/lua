#include "test_main.hpp"
#include "test_utils.hpp"
#include <iostream>

namespace Lua {
namespace Tests {
/**
 * @brief Run all Lua interpreter tests
 * 
 * This function executes all the test cases defined in the Lua interpreter test suite.
 * It handles exceptions and reports the results of each test.
 */
void runAllTests() {
    RUN_TEST_SUITE(ParserTestSuite);
}

} // namespace Tests
} // namespace Lua

//int main() {
//   Lua::Tests::runAllTests();
//   return 0;
//}