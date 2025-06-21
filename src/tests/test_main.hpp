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
#include "plugin/plugin_integration_test.hpp"

namespace Lua {
namespace Tests {

	/**
	 * Function to run all tests across all modules
	 * 
	 * This is the main entry point for the entire test suite.
	 * It runs tests for all major modules in the Lua compiler.
	 * 
	 * Test Hierarchy:
	 * MAIN (runAllTests) -> MODULE (ParserTestSuite, etc.) -> SUITE -> GROUP -> INDIVIDUAL
	 */
	void runAllTests() {
		// Run parser module tests
		// RUN_TEST_MODULE("Parser Module", ParserTestSuite);
		
		// TODO: Add other modules as they are implemented
		// RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
		// RUN_TEST_MODULE("VM Module", VMTestSuite);
		RUN_TEST_MODULE("Compiler Module", CompilerTestSuite);
		// RUN_TEST_MODULE("GC Module", GCTestSuite);
		// RUN_TEST_MODULE("Library Module", LibTestSuite);
	}

} // namespace Tests
} // namespace Lua

#endif // TEST_MAIN_HPP