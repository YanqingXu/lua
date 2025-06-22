#include "compiler_error_test.hpp"

namespace Lua {
namespace Tests {

void CompilerErrorTest::runAllTests() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::GROUP, "Compiler Error Handling Tests", 
                               "Testing compiler error detection and handling");
    
    // Run test groups
    RUN_TEST_GROUP("Semantic Errors", testSemanticErrors);
    RUN_TEST_GROUP("Type Errors", testTypeErrors);
    RUN_TEST_GROUP("Scope Errors", testScopeErrors);
    RUN_TEST_GROUP("Symbol Table Errors", testSymbolTableErrors);
    RUN_TEST_GROUP("Code Generation Errors", testCodeGenerationErrors);
    RUN_TEST_GROUP("Error Recovery", testErrorRecovery);
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::GROUP, "Compiler Error Handling Tests completed");
}

void CompilerErrorTest::testSemanticErrors() {
    SAFE_RUN_TEST(CompilerErrorTest, testUndefinedVariables);
    SAFE_RUN_TEST(CompilerErrorTest, testUndefinedFunctions);
    SAFE_RUN_TEST(CompilerErrorTest, testRedefinitionErrors);
}

void CompilerErrorTest::testTypeErrors() {
    SAFE_RUN_TEST(CompilerErrorTest, testInvalidOperations);
    SAFE_RUN_TEST(CompilerErrorTest, testTypeMismatch);
    SAFE_RUN_TEST(CompilerErrorTest, testInvalidAssignments);
}

void CompilerErrorTest::testScopeErrors() {
    SAFE_RUN_TEST(CompilerErrorTest, testVariableOutOfScope);
    SAFE_RUN_TEST(CompilerErrorTest, testFunctionScopeErrors);
    SAFE_RUN_TEST(CompilerErrorTest, testNestedScopeErrors);
}

void CompilerErrorTest::testSymbolTableErrors() {
    SAFE_RUN_TEST(CompilerErrorTest, testSymbolTableOverflow);
    SAFE_RUN_TEST(CompilerErrorTest, testInvalidSymbolOperations);
    SAFE_RUN_TEST(CompilerErrorTest, testSymbolTableCorruption);
}

void CompilerErrorTest::testCodeGenerationErrors() {
    SAFE_RUN_TEST(CompilerErrorTest, testInvalidBytecode);
    SAFE_RUN_TEST(CompilerErrorTest, testCodeGenerationFailure);
    SAFE_RUN_TEST(CompilerErrorTest, testOptimizationErrors);
}

void CompilerErrorTest::testErrorRecovery() {
    SAFE_RUN_TEST(CompilerErrorTest, testMultipleErrors);
    SAFE_RUN_TEST(CompilerErrorTest, testErrorCascading);
    SAFE_RUN_TEST(CompilerErrorTest, testRecoveryAfterErrors);
}

// Individual test implementations
void CompilerErrorTest::testUndefinedVariables() {
    std::string source = R"(
        local x = y + 1  -- y is undefined
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Undefined variables detection", hasError);
}

void CompilerErrorTest::testUndefinedFunctions() {
    std::string source = R"(
        local x = undefinedFunc(1, 2)
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Undefined functions detection", hasError);
}

void CompilerErrorTest::testRedefinitionErrors() {
    std::string source = R"(
        local x = 1
        local x = 2  -- redefinition in same scope
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Variable redefinition detection", hasError);
}

void CompilerErrorTest::testInvalidOperations() {
    std::string source = R"(
        local x = "string" + 123  -- invalid operation
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Invalid operations detection", hasError);
}

void CompilerErrorTest::testTypeMismatch() {
    std::string source = R"(
        function test()
            return "string"
        end
        local x = test() + 1  -- type mismatch
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Type mismatch detection", hasError);
}

void CompilerErrorTest::testInvalidAssignments() {
    std::string source = R"(
        local function test()
            return 1, 2, 3
        end
        local x, y = test(), test()  -- invalid multiple assignment
        return x + y
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Invalid assignments detection", hasError);
}

void CompilerErrorTest::testVariableOutOfScope() {
    std::string source = R"(
        do
            local x = 1
        end
        return x  -- x is out of scope
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Variable out of scope detection", hasError);
}

void CompilerErrorTest::testFunctionScopeErrors() {
    std::string source = R"(
        function outer()
            local x = 1
            function inner()
                return y  -- y not defined in any accessible scope
            end
            return inner()
        end
        return outer()
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Function scope errors detection", hasError);
}

void CompilerErrorTest::testNestedScopeErrors() {
    std::string source = R"(
        local x = 1
        do
            do
                local y = x  -- valid
                do
                    return z  -- z undefined in nested scope
                end
            end
        end
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Nested scope errors detection", hasError);
}

void CompilerErrorTest::testSymbolTableOverflow() {
    std::string source = "local ";
    
    // Create many variable declarations to potentially overflow symbol table
    for (int i = 0; i < 1000; ++i) {
        source += "x" + std::to_string(i) + ", ";
    }
    source += "final = 1";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Symbol table overflow handling", hasError);
}

void CompilerErrorTest::testInvalidSymbolOperations() {
    std::string source = R"(
        local x = 1
        -- Attempt to use x as both variable and function
        local y = x()
        return y
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Invalid symbol operations detection", hasError);
}

void CompilerErrorTest::testSymbolTableCorruption() {
    std::string source = R"(
        local x = 1
        do
            local x = 2  -- shadow outer x
            do
                local x = 3  -- shadow again
                return x + undefinedVar  -- error in deeply nested scope
            end
        end
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Symbol table corruption resistance", hasError);
}

void CompilerErrorTest::testInvalidBytecode() {
    std::string source = R"(
        -- Complex expression that might generate invalid bytecode
        local x = (function() return 1, 2, 3 end)() + 
                  (function() return "string" end)()
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Invalid bytecode detection", hasError);
}

void CompilerErrorTest::testCodeGenerationFailure() {
    std::string source = R"(
        -- Recursive function that might cause code generation issues
        function factorial(n)
            if n <= 1 then
                return 1
            else
                return n * factorial(n - 1) * undefinedVar
            end
        end
        return factorial(5)
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Code generation failure handling", hasError);
}

void CompilerErrorTest::testOptimizationErrors() {
    std::string source = R"(
        -- Code that might cause optimization errors
        local x = 1
        while true do
            x = x + undefinedVar
            if x > 100 then
                break
            end
        end
        return x
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Optimization error handling", hasError);
}

void CompilerErrorTest::testMultipleErrors() {
    std::string source = R"(
        local x = undefinedVar1 + undefinedVar2
        local y = anotherUndefined()
        return x + y + yetAnotherUndefined
    )";
    
    int errorCount = countCompilationErrors(source);
    bool hasMultipleErrors = errorCount >= 2;
    printTestResult("Multiple errors detection", hasMultipleErrors);
}

void CompilerErrorTest::testErrorCascading() {
    std::string source = R"(
        local x = undefinedVar
        local y = x + 1  -- This might cause cascading error
        local z = y * 2  -- And this too
        return z
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Error cascading handling", hasError);
}

void CompilerErrorTest::testRecoveryAfterErrors() {
    std::string source = R"(
        local x = undefinedVar  -- Error here
        local y = 42            -- Should still be processed
        return y                -- Should still be processed
    )";
    
    bool hasError = compileAndCheckError(source);
    printTestResult("Recovery after errors", hasError);
}

// Helper method implementations
void CompilerErrorTest::printTestResult(const std::string& testName, bool passed) {
    TestUtils::printTestResult(testName, passed);
}

bool CompilerErrorTest::compileAndCheckError(const std::string& source, bool expectError) {
    Parser parser(source);

    // Parse the source as statements
    auto statements = parser.parse();

    if (parser.hasError() && expectError) {
        return true; // Expected error occurred during parsing
    }

    // Try to compile if parsing succeeded
    if (!parser.hasError() && !statements.empty()) {
        Compiler compiler;
        auto bytecode = compiler.compile(statements);

        if (expectError) {
            // If we expected an error but compilation succeeded, test failed
            return false;
        }
        else {
            // If we didn't expect an error and compilation succeeded, test passed
            return true;
        }
    }

    return expectError; // Return whether we expected this outcome
}

bool CompilerErrorTest::containsSpecificError(const std::string& source, const std::string& errorType) {
    Parser parser(source);

    auto statements = parser.parse();

    if (!parser.hasError() && !statements.empty()) {
        Compiler compiler;
        auto bytecode = compiler.compile(statements);
    }

    return false; // No error occurred
}

int CompilerErrorTest::countCompilationErrors(const std::string& source) {
    Parser parser(source);

    auto statements = parser.parse();

    if (parser.hasError()) {
        return static_cast<int>(parser.getErrorCount()); // Return actual error count
    }

    if (!statements.empty()) {
        Compiler compiler;
        auto bytecode = compiler.compile(statements);
    }

    return 0; // No errors
}

} // namespace Tests
} // namespace Lua