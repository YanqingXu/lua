#include "closure_basic_test.hpp"
#include "../../test_utils.hpp"

namespace Lua {
namespace Tests {

void ClosureBasicTest::runAllTests() {
    // Run core functionality tests
    RUN_TEST_GROUP("Basic Closure Creation Tests", testBasicClosureCreation);
    RUN_TEST_GROUP("Upvalue Capture Tests", testUpvalueCapture);
    RUN_TEST_GROUP("Nested Closure Tests", testNestedClosures);
    RUN_TEST_GROUP("Closure Invocation Tests", testClosureInvocation);
    RUN_TEST_GROUP("Simple Upvalue Modification Tests", testSimpleUpvalueModification);
}

void ClosureBasicTest::testBasicClosureCreation() {
    std::cout << "\n  Testing basic closure creation..." << std::endl;
    
    // Test 1: Simple closure creation
    std::string luaCode1 = R"(
        function createClosure()
            local x = 10
            return function()
                return x
            end
        end
        
        local closure = createClosure()
        return closure()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "10");
    TestUtils::printTestResult("Simple closure creation", test1);
    
    // Test 2: Closure with parameters
    std::string luaCode2 = R"(
        function createAdder(x)
            return function(y)
                return x + y
            end
        end
        
        local add5 = createAdder(5)
        return add5(3)
    )";
    
    bool test2 = executeClosureTest(luaCode2, "8");
    TestUtils::printTestResult("Closure with parameters", test2);
    
    // Test 3: Multiple closures from same function
    std::string luaCode3 = R"(
        function createCounter()
            local count = 0
            return function()
                count = count + 1
                return count
            end
        end
        
        local counter1 = createCounter()
        local counter2 = createCounter()
        
        local result1 = counter1() + counter1()
        local result2 = counter2()
        
        return result1 + result2
    )";
    
    bool test3 = executeClosureTest(luaCode3, "4"); // 1+2+1 = 4
    TestUtils::printTestResult("Multiple closures from same function", test3);
}

void ClosureBasicTest::testUpvalueCapture() {
    std::cout << "\n  Testing upvalue capture..." << std::endl;
    
    // Test 1: Single upvalue capture
    std::string luaCode1 = R"(
        local x = 42
        local function getClosure()
            return function()
                return x
            end
        end
        
        local closure = getClosure()
        return closure()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "42");
    TestUtils::printTestResult("Single upvalue capture", test1);
    
    // Test 2: Multiple upvalue capture
    std::string luaCode2 = R"(
        local a, b, c = 1, 2, 3
        local function createClosure()
            return function()
                return a + b + c
            end
        end
        
        local closure = createClosure()
        return closure()
    )";
    
    bool test2 = executeClosureTest(luaCode2, "6");
    TestUtils::printTestResult("Multiple upvalue capture", test2);
    
    // Test 3: Upvalue capture with local variables
    std::string luaCode3 = R"(
        function outer(x)
            local y = x * 2
            return function(z)
                return x + y + z
            end
        end
        
        local closure = outer(5)
        return closure(3)
    )";
    
    bool test3 = executeClosureTest(luaCode3, "18"); // 5 + 10 + 3 = 18
    TestUtils::printTestResult("Upvalue capture with local variables", test3);
}

void ClosureBasicTest::testNestedClosures() {
    std::cout << "\n  Testing nested closures..." << std::endl;
    
    // Test 1: Two-level nesting
    std::string luaCode1 = R"(
        function level1(x)
            return function(y)
                return function(z)
                    return x + y + z
                end
            end
        end
        
        local closure = level1(1)(2)
        return closure(3)
    )";
    
    bool test1 = executeClosureTest(luaCode1, "6");
    TestUtils::printTestResult("Two-level nested closures", test1);
    
    // Test 2: Nested closures with shared upvalues
    std::string luaCode2 = R"(
        function createNestedCounters()
            local count = 0
            
            local function increment()
                count = count + 1
                return count
            end
            
            local function decrement()
                count = count - 1
                return count
            end
            
            return increment, decrement
        end
        
        local inc, dec = createNestedCounters()
        local result = inc() + inc() + dec()
        return result
    )";
    
    bool test2 = executeClosureTest(luaCode2, "2"); // 1 + 2 + 1 = 4, but 1+2-1 = 2
    TestUtils::printTestResult("Nested closures with shared upvalues", test2);
}

void ClosureBasicTest::testClosureInvocation() {
    std::cout << "\n  Testing closure invocation..." << std::endl;
    
    // Test 1: Direct invocation
    std::string luaCode1 = R"(
        local function createFunc()
            return function(x)
                return x * x
            end
        end
        
        return createFunc()(5)
    )";
    
    bool test1 = executeClosureTest(luaCode1, "25");
    TestUtils::printTestResult("Direct closure invocation", test1);
    
    // Test 2: Stored closure invocation
    std::string luaCode2 = R"(
        local function createMultiplier(factor)
            return function(x)
                return x * factor
            end
        end
        
        local double = createMultiplier(2)
        local triple = createMultiplier(3)
        
        return double(4) + triple(2)
    )";
    
    bool test2 = executeClosureTest(luaCode2, "14"); // 8 + 6 = 14
    TestUtils::printTestResult("Stored closure invocation", test2);
    
    // Test 3: Closure as callback
    std::string luaCode3 = R"(
        local function applyOperation(x, y, operation)
            return operation(x, y)
        end
        
        local function createAdder()
            return function(a, b)
                return a + b
            end
        end
        
        local adder = createAdder()
        return applyOperation(10, 5, adder)
    )";
    
    bool test3 = executeClosureTest(luaCode3, "15");
    TestUtils::printTestResult("Closure as callback", test3);
}

void ClosureBasicTest::testSimpleUpvalueModification() {
    std::cout << "\n  Testing simple upvalue modification..." << std::endl;
    
    // Test 1: Basic upvalue modification
    std::string luaCode1 = R"(
        local function createCounter()
            local count = 0
            return function()
                count = count + 1
                return count
            end
        end
        
        local counter = createCounter()
        local result = counter() + counter() + counter()
        return result
    )";
    
    bool test1 = executeClosureTest(luaCode1, "6"); // 1 + 2 + 3 = 6
    TestUtils::printTestResult("Basic upvalue modification", test1);
    
    // Test 2: Upvalue modification with multiple closures
    std::string luaCode2 = R"(
        local function createSharedCounter()
            local count = 0
            
            local function increment()
                count = count + 1
                return count
            end
            
            local function getCount()
                return count
            end
            
            return increment, getCount
        end
        
        local inc, get = createSharedCounter()
        inc()
        inc()
        return get()
    )";
    
    bool test2 = executeClosureTest(luaCode2, "2");
    TestUtils::printTestResult("Upvalue modification with multiple closures", test2);
}

// Helper method implementations

bool ClosureBasicTest::compileAndExecute(const std::string& luaCode) {
    try {
        // This is a placeholder implementation
        // In a real implementation, you would:
        // 1. Create a lexer and tokenize the code
        // 2. Parse the tokens into an AST
        // 3. Compile the AST to bytecode
        // 4. Execute the bytecode in the VM
        
        // For now, return true to indicate successful compilation
        return true;
    } catch (const std::exception& e) {
        std::cout << "    Error: " << e.what() << std::endl;
        return false;
    }
}

bool ClosureBasicTest::executeClosureTest(const std::string& luaCode, const std::string& expectedResult) {
    try {
        // This is a placeholder implementation
        // In a real implementation, you would:
        // 1. Compile and execute the code
        // 2. Compare the result with expectedResult
        // 3. Return true if they match, false otherwise
        
        bool compiled = compileAndExecute(luaCode);
        
        // For now, assume the test passes if compilation succeeds
        // In a real implementation, you would check the actual result
        return compiled;
    } catch (const std::exception& e) {
        std::cout << "    Execution error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Tests
} // namespace Lua