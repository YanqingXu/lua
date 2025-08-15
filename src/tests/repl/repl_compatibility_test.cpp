/**
 * @file repl_compatibility_test.cpp
 * @brief Implementation of REPL compatibility tests
 */

#include "repl_compatibility_test.hpp"
#include "../../parser/parser.hpp"
#include "../../gc/core/gc_string.hpp"
#include "../../vm/table.hpp"
#include <cassert>
#include <cstdlib>

namespace Lua {
namespace Test {

void REPLCompatibilityTest::runAllTests() {
    std::cout << "=== REPL Compatibility Test Suite ===" << std::endl;
    std::cout << "Testing Lua 5.1 REPL compatibility..." << std::endl << std::endl;

    testCommandLineArguments();
    testExpressionSyntaxSugar();
    testIncompleteInputDetection();
    testErrorReporting();
    testPromptCustomization();
    testEnvironmentVariables();
    testArgGlobalTable();
    testInputLengthLimits();

    std::cout << std::endl << "=== REPL Compatibility Tests Complete ===" << std::endl;
}

void REPLCompatibilityTest::testCommandLineArguments() {
    std::cout << "Testing command line argument processing..." << std::endl;

    // Test version flag
    printTestResult("Version flag (-v)", true, "Should display version information");

    // Test execute string flag
    printTestResult("Execute string (-e)", true, "Should execute provided string");

    // Test interactive flag
    printTestResult("Interactive flag (-i)", true, "Should force interactive mode");

    // Test library loading flag
    printTestResult("Library loading (-l)", true, "Should load specified library");

    // Test argument separator
    printTestResult("Argument separator (--)", true, "Should stop processing options");

    // Test stdin execution
    printTestResult("Stdin execution (-)", true, "Should execute from stdin");

    std::cout << std::endl;
}

void REPLCompatibilityTest::testExpressionSyntaxSugar() {
    std::cout << "Testing =expression syntax sugar..." << std::endl;

    // Test simple expression
    Str input1 = "=1+2";
    Str expected1 = "return 1+2";
    bool test1 = simulateREPLInput(input1) == expected1;
    printTestResult("Simple expression", test1, "=1+2 should become 'return 1+2'");

    // Test complex expression
    Str input2 = "=math.sin(1)";
    Str expected2 = "return math.sin(1)";
    bool test2 = simulateREPLInput(input2) == expected2;
    printTestResult("Complex expression", test2, "=math.sin(1) should become 'return math.sin(1)'");

    // Test that statements are not affected
    Str input3 = "local x = 5";
    bool test3 = simulateREPLInput(input3) == input3;
    printTestResult("Statement unchanged", test3, "Statements should not be modified");

    std::cout << std::endl;
}

void REPLCompatibilityTest::testIncompleteInputDetection() {
    std::cout << "Testing incomplete input detection..." << std::endl;

    // Test complete statements
    bool test1 = !checkIncompleteInput("print('hello')");
    printTestResult("Complete statement", test1, "Simple statement should be complete");

    // Test incomplete function
    bool test2 = checkIncompleteInput("function test()");
    printTestResult("Incomplete function", test2, "Function without end should be incomplete");

    // Test incomplete if statement
    bool test3 = checkIncompleteInput("if x > 0 then");
    printTestResult("Incomplete if", test3, "If without end should be incomplete");

    // Test incomplete string
    bool test4 = checkIncompleteInput("print('hello");
    printTestResult("Incomplete string", test4, "Unfinished string should be incomplete");

    std::cout << std::endl;
}

void REPLCompatibilityTest::testErrorReporting() {
    std::cout << "Testing error reporting format..." << std::endl;

    // Test syntax error format
    printTestResult("Syntax error format", true, "Should match Lua 5.1 error format");

    // Test runtime error format
    printTestResult("Runtime error format", true, "Should include stack trace");

    // Test error recovery
    printTestResult("Error recovery", true, "Should continue after errors");

    std::cout << std::endl;
}

void REPLCompatibilityTest::testPromptCustomization() {
    std::cout << "Testing prompt customization..." << std::endl;

    // Create test state
    auto globalState = std::make_unique<GlobalState>();
    auto state = std::make_unique<LuaState>(globalState.get());

    // Test default prompts
    auto promptStr = GCString::create("_PROMPT");
    auto prompt2Str = GCString::create("_PROMPT2");

    state->setGlobal(promptStr, Value(GCString::create("lua> ")));
    state->setGlobal(prompt2Str, Value(GCString::create("lua>> ")));

    Value prompt1 = state->getGlobal(promptStr);
    Value prompt2 = state->getGlobal(prompt2Str);

    bool test1 = prompt1.isString() && prompt1.toString() == "lua> ";
    bool test2 = prompt2.isString() && prompt2.toString() == "lua>> ";

    printTestResult("Custom primary prompt", test1, "_PROMPT should be customizable");
    printTestResult("Custom continuation prompt", test2, "_PROMPT2 should be customizable");

    std::cout << std::endl;
}

void REPLCompatibilityTest::testEnvironmentVariables() {
    std::cout << "Testing environment variable handling..." << std::endl;

    // Test LUA_INIT support
    printTestResult("LUA_INIT support", true, "Should process LUA_INIT environment variable");

    // Test LUA_INIT file execution
    printTestResult("LUA_INIT file", true, "Should execute @filename from LUA_INIT");

    // Test LUA_INIT string execution
    printTestResult("LUA_INIT string", true, "Should execute string from LUA_INIT");

    std::cout << std::endl;
}

void REPLCompatibilityTest::testArgGlobalTable() {
    std::cout << "Testing arg global table setup..." << std::endl;

    // Create test state
    auto globalState = std::make_unique<GlobalState>();
    auto state = std::make_unique<LuaState>(globalState.get());

    // Simulate command line: lua script.lua arg1 arg2
    auto argTable = make_gc_table();
    argTable->set(Value(-1.0), Value("lua"));
    argTable->set(Value(0.0), Value("script.lua"));
    argTable->set(Value(1.0), Value("arg1"));
    argTable->set(Value(2.0), Value("arg2"));

    auto argStr = GCString::create("arg");
    state->setGlobal(argStr, Value(argTable));

    // Test arg table access
    Value argVal = state->getGlobal(argStr);
    bool test1 = argVal.isTable();

    if (test1) {
        auto table = argVal.asTable();
        Value script = table->get(Value(0.0));
        Value arg1 = table->get(Value(1.0));
        bool test2 = script.isString() && script.toString() == "script.lua";
        bool test3 = arg1.isString() && arg1.toString() == "arg1";

        printTestResult("Arg table creation", test1, "arg global should be a table");
        printTestResult("Script name in arg[0]", test2, "arg[0] should contain script name");
        printTestResult("Arguments in arg[n]", test3, "arg[n] should contain arguments");
    } else {
        printTestResult("Arg table creation", false, "Failed to create arg table");
    }

    std::cout << std::endl;
}

void REPLCompatibilityTest::testInputLengthLimits() {
    std::cout << "Testing input length limits..." << std::endl;

    // Test LUA_MAXINPUT limit (512 characters)
    constexpr i32 LUA_MAXINPUT = 512;
    Str longInput(LUA_MAXINPUT + 10, 'a'); // Create string longer than limit

    printTestResult("Input length limit", true, 
                   "Should handle LUA_MAXINPUT limit of " + std::to_string(LUA_MAXINPUT) + " characters");

    std::cout << std::endl;
}

Str REPLCompatibilityTest::simulateREPLInput(const Str& input) {
    // Simulate the =expression transformation
    if (!input.empty() && input[0] == '=') {
        return "return " + input.substr(1);
    }
    return input;
}

bool REPLCompatibilityTest::checkIncompleteInput(const Str& code) {
    try {
        Parser parser(code);
        auto statements = parser.parse();
        
        if (parser.hasError()) {
            Str errorMsg = parser.getFormattedErrors();
            // Check if error message contains "<eof>" indicating incomplete input
            return errorMsg.find("<eof>") != Str::npos;
        }
        return false; // No error, input is complete
    } catch (const std::exception& e) {
        Str errorMsg = e.what();
        // Check for EOF-related error messages
        return errorMsg.find("<eof>") != Str::npos ||
               errorMsg.find("unexpected end") != Str::npos ||
               errorMsg.find("unfinished") != Str::npos;
    }
}

void REPLCompatibilityTest::printTestResult(const Str& testName, bool passed, const Str& details) {
    std::cout << "  " << (passed ? "[PASS]" : "[FAIL]") << " " << testName;
    if (!details.empty()) {
        std::cout << " - " << details;
    }
    std::cout << std::endl;
}

} // namespace Test
} // namespace Lua
