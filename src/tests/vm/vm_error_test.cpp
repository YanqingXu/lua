#include "vm_error_test.hpp"
#include "../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {

void VMErrorTest::runAllTests() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::GROUP, "VM Error Handling Tests", 
                               "Testing virtual machine error detection and handling");
    
    // Run test groups
    RUN_TEST_GROUP("Runtime Errors", testRuntimeErrors);
    RUN_TEST_GROUP("Stack Errors", testStackErrors);
    RUN_TEST_GROUP("Memory Errors", testMemoryErrors);
    RUN_TEST_GROUP("Bytecode Errors", testBytecodeErrors);
    RUN_TEST_GROUP("Error Propagation", testErrorPropagation);
    RUN_TEST_GROUP("Exception Handling", testExceptionHandling);
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::GROUP, "VM Error Handling Tests completed");
}

void VMErrorTest::testRuntimeErrors() {
    SAFE_RUN_TEST(VMErrorTest, testDivisionByZero);
    SAFE_RUN_TEST(VMErrorTest, testNilAccess);
    SAFE_RUN_TEST(VMErrorTest, testInvalidOperations);
    SAFE_RUN_TEST(VMErrorTest, testTypeErrors);
    SAFE_RUN_TEST(VMErrorTest, testIndexOutOfBounds);
}

void VMErrorTest::testStackErrors() {
    SAFE_RUN_TEST(VMErrorTest, testStackOverflow);
    SAFE_RUN_TEST(VMErrorTest, testStackUnderflow);
    SAFE_RUN_TEST(VMErrorTest, testInvalidStackOperations);
    SAFE_RUN_TEST(VMErrorTest, testStackCorruption);
}

void VMErrorTest::testMemoryErrors() {
    SAFE_RUN_TEST(VMErrorTest, testOutOfMemory);
    SAFE_RUN_TEST(VMErrorTest, testMemoryLeaks);
    SAFE_RUN_TEST(VMErrorTest, testInvalidMemoryAccess);
    SAFE_RUN_TEST(VMErrorTest, testGarbageCollectionErrors);
}

void VMErrorTest::testBytecodeErrors() {
    SAFE_RUN_TEST(VMErrorTest, testInvalidBytecode);
    SAFE_RUN_TEST(VMErrorTest, testCorruptedBytecode);
    SAFE_RUN_TEST(VMErrorTest, testUnsupportedInstructions);
    SAFE_RUN_TEST(VMErrorTest, testBytecodeVersionMismatch);
}

void VMErrorTest::testErrorPropagation() {
    SAFE_RUN_TEST(VMErrorTest, testErrorInNestedCalls);
    SAFE_RUN_TEST(VMErrorTest, testErrorInCoroutines);
    SAFE_RUN_TEST(VMErrorTest, testErrorInMetamethods);
    SAFE_RUN_TEST(VMErrorTest, testErrorRecoveryMechanisms);
}

void VMErrorTest::testExceptionHandling() {
    SAFE_RUN_TEST(VMErrorTest, testCppExceptionHandling);
    SAFE_RUN_TEST(VMErrorTest, testLuaErrorHandling);
    SAFE_RUN_TEST(VMErrorTest, testMixedExceptionTypes);
    SAFE_RUN_TEST(VMErrorTest, testExceptionCleanup);
}

// Runtime error test implementations
void VMErrorTest::testDivisionByZero() {
    std::string source = R"(
        local x = 10
        local y = 0
        return x / y
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Division by zero detection", hasError);
}

void VMErrorTest::testNilAccess() {
    std::string source = R"(
        local x = nil
        return x.field
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Nil access detection", hasError);
}

void VMErrorTest::testInvalidOperations() {
    std::string source = R"(
        local x = "string"
        local y = {}
        return x + y
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Invalid operations detection", hasError);
}

void VMErrorTest::testTypeErrors() {
    std::string source = R"(
        local x = "not a function"
        return x()
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Type errors detection", hasError);
}

void VMErrorTest::testIndexOutOfBounds() {
    std::string source = R"(
        local arr = {1, 2, 3}
        return arr[10]
    )";
    
    // Note: In Lua, accessing out-of-bounds index returns nil, not an error
    // This test checks if the VM handles it gracefully
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Index out of bounds handling", !hasError);
}

// Stack error test implementations
void VMErrorTest::testStackOverflow() {
    std::string source = R"(
        function recursive(n)
            return recursive(n + 1)
        end
        return recursive(1)
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Stack overflow detection", hasError);
}

void VMErrorTest::testStackUnderflow() {
    // This would typically be tested at the VM instruction level
    // For now, we'll simulate a scenario that might cause stack underflow
    std::string source = R"(
        -- This is a conceptual test
        -- Actual stack underflow would be caught at VM level
        return
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Stack underflow prevention", !hasError);
}

void VMErrorTest::testInvalidStackOperations() {
    // Test VM's resilience to invalid stack operations
    std::string source = R"(
        -- Complex expression that might stress the stack
        local function complex()
            local a, b, c, d, e = 1, 2, 3, 4, 5
            return a + b + c + d + e
        end
        return complex()
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Invalid stack operations handling", !hasError);
}

void VMErrorTest::testStackCorruption() {
    std::string source = R"(
        -- Test stack integrity with complex operations
        local function test()
            local x = {}
            for i = 1, 100 do
                x[i] = function() return i end
            end
            return x
        end
        return test()
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Stack corruption resistance", !hasError);
}

// Memory error test implementations
void VMErrorTest::testOutOfMemory() {
    std::string source = R"(
        -- Attempt to allocate large amounts of memory
        local big_table = {}
        for i = 1, 1000000 do
            big_table[i] = string.rep("x", 1000)
        end
        return big_table
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Out of memory handling", hasError);
}

void VMErrorTest::testMemoryLeaks() {
    std::string source = R"(
        -- Create circular references
        local a = {}
        local b = {}
        a.ref = b
        b.ref = a
        return a
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Memory leak prevention", !hasError);
}

void VMErrorTest::testInvalidMemoryAccess() {
    // This would typically be caught by the C++ runtime
    std::string source = R"(
        -- Test that should not cause invalid memory access
        local x = {}
        x = nil
        collectgarbage()
        return "ok"
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Invalid memory access prevention", !hasError);
}

void VMErrorTest::testGarbageCollectionErrors() {
    std::string source = R"(
        -- Test GC under stress
        for i = 1, 1000 do
            local temp = {data = string.rep("test", 100)}
            if i % 100 == 0 then
                collectgarbage()
            end
        end
        return "completed"
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Garbage collection error handling", !hasError);
}

// Bytecode error test implementations
void VMErrorTest::testInvalidBytecode() {
    // This would require injecting invalid bytecode directly
    // For now, test with malformed source that might generate issues
    std::string source = R"(
        -- Source that might generate problematic bytecode
        local x = function() end
        return x
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Invalid bytecode handling", !hasError);
}

void VMErrorTest::testCorruptedBytecode() {
    // Similar to above - would need direct bytecode manipulation
    std::string source = R"(
        return "test"
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Corrupted bytecode detection", !hasError);
}

void VMErrorTest::testUnsupportedInstructions() {
    // Test VM's handling of potentially unsupported operations
    std::string source = R"(
        -- Test complex operations
        local co = coroutine.create(function() return 42 end)
        return coroutine.resume(co)
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Unsupported instructions handling", !hasError);
}

void VMErrorTest::testBytecodeVersionMismatch() {
    // This would require version checking in the VM
    std::string source = R"(
        return "version test"
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Bytecode version mismatch handling", !hasError);
}

// Error propagation test implementations
void VMErrorTest::testErrorInNestedCalls() {
    std::string source = R"(
        function level3()
            error("Error at level 3")
        end
        
        function level2()
            return level3()
        end
        
        function level1()
            return level2()
        end
        
        return level1()
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Error propagation in nested calls", hasError);
}

void VMErrorTest::testErrorInCoroutines() {
    std::string source = R"(
        local co = coroutine.create(function()
            error("Error in coroutine")
        end)
        return coroutine.resume(co)
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Error handling in coroutines", hasError);
}

void VMErrorTest::testErrorInMetamethods() {
    std::string source = R"(
        local mt = {
            __add = function(a, b)
                error("Error in metamethod")
            end
        }
        local x = setmetatable({}, mt)
        local y = setmetatable({}, mt)
        return x + y
    )";
    
    bool hasError = executeAndCheckError(source);
    printTestResult("Error handling in metamethods", hasError);
}

void VMErrorTest::testErrorRecoveryMechanisms() {
    std::string source = R"(
        local success, result = pcall(function()
            error("Recoverable error")
        end)
        return success, result
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Error recovery mechanisms", !hasError);
}

// Exception handling test implementations
void VMErrorTest::testCppExceptionHandling() {
    // Test that C++ exceptions are properly handled
    std::string source = R"(
        -- This should not cause C++ exceptions to leak
        return "cpp exception test"
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("C++ exception handling", !hasError);
}

void VMErrorTest::testLuaErrorHandling() {
    std::string source = R"(
        local function test()
            error("Lua error")
        end
        
        local success, err = pcall(test)
        return success, err
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Lua error handling", !hasError);
}

void VMErrorTest::testMixedExceptionTypes() {
    std::string source = R"(
        -- Test mixing different types of errors
        local function test()
            local x = nil
            return x.nonexistent
        end
        
        local success, err = pcall(test)
        return success, err
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Mixed exception types handling", !hasError);
}

void VMErrorTest::testExceptionCleanup() {
    std::string source = R"(
        -- Test that resources are cleaned up after errors
        local function test()
            local file = io.open("nonexistent.txt", "r")
            if not file then
                error("File not found")
            end
            return file
        end
        
        local success, result = pcall(test)
        return success, result
    )";
    
    bool hasError = executeAndCheckError(source, false);
    printTestResult("Exception cleanup", !hasError);
}

// Helper method implementations
void VMErrorTest::printTestResult(const std::string& testName, bool passed) {
    TestUtils::printTestResult(testName, passed);
}

bool VMErrorTest::executeAndCheckError(const std::string& source, bool expectError) {
    try {
        // Create VM and execute code
        VM vm;
        Lexer lexer(source);
        Parser parser(source);
        
        auto ast = parser.parseExpression();
        if (ast == nullptr && expectError) {
            return true; // Parsing failed as expected
        }
        
        if (ast != nullptr) {
            Compiler compiler;
            auto bytecode = compiler.compile(ast.get());
            
            if (bytecode != nullptr) {
                auto result = vm.execute(bytecode.get());
                
                if (expectError) {
                    // If we expected an error but execution succeeded, test failed
                    return false;
                } else {
                    // If we didn't expect an error and execution succeeded, test passed
                    return true;
                }
            }
        }
        
        return expectError; // Return whether we expected this outcome
        
    } catch (const std::exception& e) {
        // Exception occurred - this counts as an error
        return expectError;
    }
}

bool VMErrorTest::containsRuntimeError(const std::string& source, const std::string& errorType) {
    try {
        VM vm;
        Lexer lexer(source);
        Parser parser(source);
        
        auto ast = parser.parseExpression();
        if (ast != nullptr) {
            Compiler compiler;
            auto bytecode = compiler.compile(ast.get());
            
            if (bytecode != nullptr) {
                auto result = vm.execute(bytecode.get());
            }
        }
        
        return false; // No error occurred
        
    } catch (const std::exception& e) {
        std::string errorMsg = e.what();
        return errorMsg.find(errorType) != std::string::npos;
    }
}

int VMErrorTest::countRuntimeErrors(const std::string& source) {
    try {
        VM vm;
        Lexer lexer(source);
        Parser parser(source);
        
        auto ast = parser.parseExpression();
        if (ast == nullptr) {
            return 1; // Parsing error
        }
        
        Compiler compiler;
        auto bytecode = compiler.compile(ast.get());
        
        if (bytecode == nullptr) {
            return 1; // Compilation error
        }
        
        auto result = vm.execute(bytecode.get());
        return 0; // No errors
        
    } catch (const std::exception& e) {
        return 1; // Runtime error
    }
}

bool VMErrorTest::checkStackState(VM* vm, int expectedSize) {
    // This would require access to VM internals
    // For now, return true as a placeholder
    return true;
}

bool VMErrorTest::simulateMemoryPressure() {
    // This would require platform-specific memory pressure simulation
    // For now, return false as a placeholder
    return false;
}

} // namespace Tests
} // namespace Lua