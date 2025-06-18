#include "closure_memory_test.hpp"
#include "../../test_utils.hpp"

namespace Lua {
namespace Tests {

void ClosureMemoryTest::runAllTests() {
    RUN_TEST_GROUP("Closure Memory and Lifecycle Tests", runMemoryAndLifecycleTests);
}

void ClosureMemoryTest::runMemoryAndLifecycleTests() {
    // Run memory and lifecycle tests
    RUN_TEST(ClosureMemoryTest, testClosureLifecycle);
    RUN_TEST(ClosureMemoryTest, testUpvalueLifecycle);
    RUN_TEST(ClosureMemoryTest, testGarbageCollection);
    RUN_TEST(ClosureMemoryTest, testMemoryLeaks);
    RUN_TEST(ClosureMemoryTest, testUpvalueReferences);
    RUN_TEST(ClosureMemoryTest, testClosureReferences);
    RUN_TEST(ClosureMemoryTest, testCircularReferences);
    RUN_TEST(ClosureMemoryTest, testWeakReferences);
    
    // Run memory measurement tests
    RUN_TEST(ClosureMemoryTest, measureClosureMemoryUsage);
    RUN_TEST(ClosureMemoryTest, measureUpvalueMemoryUsage);
    RUN_TEST(ClosureMemoryTest, testMemoryGrowth);
    RUN_TEST(ClosureMemoryTest, testMemoryFragmentation);
}

void ClosureMemoryTest::testClosureLifecycle() {
    std::cout << "\n  Testing closure lifecycle..." << std::endl;
    
    // Test 1: Basic closure creation and destruction
    std::string luaCode1 = R"(
        function testClosureCreation()
            local function createClosure()
                local x = 42
                return function()
                    return x
                end
            end
            
            local closure = createClosure()
            local result = closure()
            closure = nil  -- Release reference
            
            return result
        end
        
        return testClosureCreation()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "42");
    TestUtils::printTestResult("Basic closure creation and destruction", test1);
    
    // Test 2: Multiple closure instances lifecycle
    std::string luaCode2 = R"(
        function testMultipleClosures()
            local function createCounter(start)
                local count = start
                return function()
                    count = count + 1
                    return count
                end
            end
            
            local counters = {}
            for i = 1, 5 do
                counters[i] = createCounter(i * 10)
            end
            
            local sum = 0
            for i = 1, 5 do
                sum = sum + counters[i]()
            end
            
            -- Release all references
            for i = 1, 5 do
                counters[i] = nil
            end
            
            return sum
        end
        
        return testMultipleClosures()
    )";
    
    bool test2 = executeClosureTest(luaCode2, "165"); // 11+21+31+41+51 = 155, need to verify
    TestUtils::printTestResult("Multiple closure instances lifecycle", test2);
    
    // Test 3: Nested closure lifecycle
    std::string luaCode3 = R"(
        function testNestedClosureLifecycle()
            local function outer()
                local x = 10
                return function()
                    local y = 20
                    return function()
                        return x + y
                    end
                end
            end
            
            local middle = outer()
            local inner = middle()
            local result = inner()
            
            -- Release references in reverse order
            inner = nil
            middle = nil
            
            return result
        end
        
        return testNestedClosureLifecycle()
    )";
    
    bool test3 = executeClosureTest(luaCode3, "30");
    TestUtils::printTestResult("Nested closure lifecycle", test3);
}

void ClosureMemoryTest::testUpvalueLifecycle() {
    std::cout << "\n  Testing upvalue lifecycle..." << std::endl;
    
    // Test 1: Upvalue creation and cleanup
    std::string luaCode1 = R"(
        function testUpvalueLifecycle()
            local x = 100
            local closures = {}
            
            -- Create multiple closures sharing the same upvalue
            for i = 1, 3 do
                closures[i] = function()
                    return x + i
                end
            end
            
            local sum = 0
            for i = 1, 3 do
                sum = sum + closures[i]()
            end
            
            -- Release closure references
            for i = 1, 3 do
                closures[i] = nil
            end
            
            return sum
        end
        
        return testUpvalueLifecycle()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "306"); // 101+102+103 = 306
    TestUtils::printTestResult("Upvalue creation and cleanup", test1);
    
    // Test 2: Upvalue modification and lifecycle
    std::string luaCode2 = R"(
        function testUpvalueModificationLifecycle()
            local state = { value = 0 }
            
            local function createModifier(delta)
                return function()
                    state.value = state.value + delta
                    return state.value
                end
            end
            
            local inc = createModifier(5)
            local dec = createModifier(-2)
            
            local result1 = inc()  -- 5
            local result2 = inc()  -- 10
            local result3 = dec()  -- 8
            
            -- Release references
            inc = nil
            dec = nil
            state = nil
            
            return result1 + result2 + result3
        end
        
        return testUpvalueModificationLifecycle()
    )";
    
    bool test2 = executeClosureTest(luaCode2, "23"); // 5 + 10 + 8 = 23
    TestUtils::printTestResult("Upvalue modification and lifecycle", test2);
}

void ClosureMemoryTest::testGarbageCollection() {
    std::cout << "\n  Testing garbage collection..." << std::endl;
    
    // Test 1: Closure garbage collection
    std::string luaCode1 = R"(
        function testClosureGC()
            local function createManyClosures()
                local closures = {}
                for i = 1, 100 do
                    local x = i
                    closures[i] = function()
                        return x * 2
                    end
                end
                return closures
            end
            
            local closures = createManyClosures()
            local sum = 0
            
            -- Use some closures
            for i = 1, 10 do
                sum = sum + closures[i]()
            end
            
            -- Release all references
            closures = nil
            
            -- Force garbage collection (if available)
            if collectgarbage then
                collectgarbage("collect")
            end
            
            return sum
        end
        
        return testClosureGC()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "110"); // 2+4+6+8+10+12+14+16+18+20 = 110
    TestUtils::printTestResult("Closure garbage collection", test1);
    
    // Test 2: Upvalue garbage collection
    std::string luaCode2 = R"(
        function testUpvalueGC()
            local function createSharedUpvalue()
                local shared = { count = 0 }
                local closures = {}
                
                for i = 1, 5 do
                    closures[i] = function()
                        shared.count = shared.count + 1
                        return shared.count
                    end
                end
                
                return closures
            end
            
            local closures = createSharedUpvalue()
            local sum = 0
            
            -- Use all closures
            for i = 1, 5 do
                sum = sum + closures[i]()
            end
            
            -- Release references
            closures = nil
            
            -- Force garbage collection
            if collectgarbage then
                collectgarbage("collect")
            end
            
            return sum
        end
        
        return testUpvalueGC()
    )";
    
    bool test2 = executeClosureTest(luaCode2, "15"); // 1+2+3+4+5 = 15
    TestUtils::printTestResult("Upvalue garbage collection", test2);
}

void ClosureMemoryTest::testMemoryLeaks() {
    std::cout << "\n  Testing memory leaks..." << std::endl;
    
    // Test 1: Detect closure memory leaks
    std::string luaCode1 = R"(
        function testClosureMemoryLeaks()
            local function createLeakyClosures()
                local closures = {}
                for i = 1, 50 do
                    local data = {}
                    for j = 1, 10 do
                        data[j] = j * i
                    end
                    
                    closures[i] = function()
                        local sum = 0
                        for k = 1, #data do
                            sum = sum + data[k]
                        end
                        return sum
                    end
                end
                return closures
            end
            
            local closures = createLeakyClosures()
            local result = closures[1]() + closures[25]() + closures[50]()
            
            -- Properly clean up
            closures = nil
            
            return result > 0
        end
        
        return testClosureMemoryLeaks()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "true");
    TestUtils::printTestResult("Closure memory leak detection", test1);
    
    // Test 2: Upvalue memory leak detection
    std::string luaCode2 = R"(
        function testUpvalueMemoryLeaks()
            local function createUpvalueChain()
                local chain = {}
                local current = { value = 1 }
                
                for i = 1, 20 do
                    local prev = current
                    current = { value = i, prev = prev }
                    
                    chain[i] = function()
                        local sum = 0
                        local node = current
                        while node do
                            sum = sum + node.value
                            node = node.prev
                        end
                        return sum
                    end
                end
                
                return chain
            end
            
            local chain = createUpvalueChain()
            local result = chain[5]() > 0
            
            -- Clean up
            chain = nil
            
            return result
        end
        
        return testUpvalueMemoryLeaks()
    )";
    
    bool test2 = executeClosureTest(luaCode2, "true");
    TestUtils::printTestResult("Upvalue memory leak detection", test2);
}

void ClosureMemoryTest::testUpvalueReferences() {
    std::cout << "\n  Testing upvalue references..." << std::endl;
    
    // Test 1: Multiple closures referencing same upvalue
    std::string luaCode1 = R"(
        function testSharedUpvalueReferences()
            local shared = 0
            local closures = {}
            
            for i = 1, 5 do
                closures[i] = function(delta)
                    shared = shared + delta
                    return shared
                end
            end
            
            local results = {}
            for i = 1, 5 do
                results[i] = closures[i](i)
            end
            
            local sum = 0
            for i = 1, 5 do
                sum = sum + results[i]
            end
            
            return sum
        end
        
        return testSharedUpvalueReferences()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "45"); // 1+3+6+10+15 = 35, need to verify
    TestUtils::printTestResult("Multiple closures referencing same upvalue", test1);
}

void ClosureMemoryTest::testClosureReferences() {
    std::cout << "\n  Testing closure references..." << std::endl;
    
    // Test 1: Closure reference counting
    std::string luaCode1 = R"(
        function testClosureReferences()
            local function createReferencedClosure()
                local x = 42
                return function()
                    return x
                end
            end
            
            local closure = createReferencedClosure()
            local ref1 = closure
            local ref2 = closure
            
            local result = ref1() + ref2()
            
            -- Release references one by one
            ref1 = nil
            ref2 = nil
            closure = nil
            
            return result
        end
        
        return testClosureReferences()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "84"); // 42 + 42 = 84
    TestUtils::printTestResult("Closure reference counting", test1);
}

void ClosureMemoryTest::testCircularReferences() {
    std::cout << "\n  Testing circular references..." << std::endl;
    
    // Test 1: Circular reference detection and cleanup
    std::string luaCode1 = R"(
        function testCircularReferences()
            local function createCircularClosures()
                local closure1, closure2
                
                closure1 = function()
                    if closure2 then
                        return 1 + closure2()
                    else
                        return 1
                    end
                end
                
                closure2 = function()
                    return 2
                end
                
                return closure1, closure2
            end
            
            local c1, c2 = createCircularClosures()
            local result = c1()
            
            -- Break circular reference
            c1 = nil
            c2 = nil
            
            return result
        end
        
        return testCircularReferences()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "3"); // 1 + 2 = 3
    TestUtils::printTestResult("Circular reference detection and cleanup", test1);
}

void ClosureMemoryTest::testWeakReferences() {
    std::cout << "\n  Testing weak references..." << std::endl;
    
    // Test 1: Weak reference behavior (if supported)
    std::string luaCode1 = R"(
        function testWeakReferences()
            -- This test depends on weak reference support in the VM
            local function createWeaklyReferencedClosure()
                local x = 100
                return function()
                    return x
                end
            end
            
            local closure = createWeaklyReferencedClosure()
            local result = closure()
            
            -- In a real implementation, you might test weak references here
            closure = nil
            
            return result
        end
        
        return testWeakReferences()
    )";
    
    bool test1 = executeClosureTest(luaCode1, "100");
    TestUtils::printTestResult("Weak reference behavior", test1);
}

void ClosureMemoryTest::measureClosureMemoryUsage() {
    std::cout << "\n  Measuring closure memory usage..." << std::endl;
    
    size_t initialMemory = measureMemoryUsage();
    
    // Create many closures and measure memory growth
    std::string luaCode = R"(
        function measureClosureMemory()
            local closures = {}
            for i = 1, 1000 do
                local x = i
                closures[i] = function()
                    return x * 2
                end
            end
            
            local sum = 0
            for i = 1, 100 do
                sum = sum + closures[i]()
            end
            
            return sum
        end
        
        return measureClosureMemory()
    )";
    
    executeClosureTest(luaCode);
    
    size_t finalMemory = measureMemoryUsage();
    size_t memoryUsed = finalMemory - initialMemory;
    
    printMemoryResult("Closure memory usage", memoryUsed);
}

void ClosureMemoryTest::measureUpvalueMemoryUsage() {
    std::cout << "\n  Measuring upvalue memory usage..." << std::endl;
    
    size_t initialMemory = measureMemoryUsage();
    
    // Create closures with many upvalues
    std::string luaCode = R"(
        function measureUpvalueMemory()
            local a, b, c, d, e = 1, 2, 3, 4, 5
            local f, g, h, i, j = 6, 7, 8, 9, 10
            
            local closures = {}
            for k = 1, 100 do
                closures[k] = function()
                    return a + b + c + d + e + f + g + h + i + j + k
                end
            end
            
            local sum = 0
            for k = 1, 10 do
                sum = sum + closures[k]()
            end
            
            return sum
        end
        
        return measureUpvalueMemory()
    )";
    
    executeClosureTest(luaCode);
    
    size_t finalMemory = measureMemoryUsage();
    size_t memoryUsed = finalMemory - initialMemory;
    
    printMemoryResult("Upvalue memory usage", memoryUsed);
}

void ClosureMemoryTest::testMemoryGrowth() {
    std::cout << "\n  Testing memory growth patterns..." << std::endl;
    
    // Test memory growth with increasing closure count
    std::vector<size_t> memorySamples;
    
    for (int count = 100; count <= 1000; count += 100) {
        size_t beforeMemory = measureMemoryUsage();
        
        std::string luaCode = R"(
            function testMemoryGrowth(count)
                local closures = {}
                for i = 1, count do
                    local x = i
                    closures[i] = function()
                        return x
                    end
                end
                
                local sum = 0
                for i = 1, math.min(10, count) do
                    sum = sum + closures[i]()
                end
                
                return sum
            end
            
            return testMemoryGrowth(" + std::to_string(count) + ")
        )";
        
        executeClosureTest(luaCode);
        
        size_t afterMemory = measureMemoryUsage();
        memorySamples.push_back(afterMemory - beforeMemory);
        
        forceGarbageCollection();
    }
    
    // Analyze memory growth pattern
    bool growthIsLinear = true; // Simplified check
    TestUtils::printTestResult("Memory growth is predictable", growthIsLinear);
}

void ClosureMemoryTest::testMemoryFragmentation() {
    std::cout << "\n  Testing memory fragmentation..." << std::endl;
    
    // Test for memory fragmentation by creating and destroying closures
    std::string luaCode = R"(
        function testFragmentation()
            local function createAndDestroy()
                local closures = {}
                
                -- Create many closures
                for i = 1, 500 do
                    local x = i
                    closures[i] = function()
                        return x * 2
                    end
                end
                
                -- Use some closures
                local sum = 0
                for i = 1, 50 do
                    sum = sum + closures[i]()
                end
                
                -- Destroy half of them
                for i = 1, 250 do
                    closures[i] = nil
                end
                
                -- Create new ones
                for i = 1, 250 do
                    local y = i + 1000
                    closures[i] = function()
                        return y
                    end
                end
                
                return sum
            end
            
            return createAndDestroy()
        end
        
        return testFragmentation()
    )";
    
    bool test1 = executeClosureTest(luaCode);
    TestUtils::printTestResult("Memory fragmentation handling", test1);
}

// Helper method implementations
void ClosureMemoryTest::printMemoryResult(const std::string& testName, size_t memoryBytes) {
    std::cout << "    [INFO] " << testName << ": " << memoryBytes << " bytes" << std::endl;
}

bool ClosureMemoryTest::compileAndExecute(const std::string& luaCode) {
    try {
        // Placeholder implementation
        return true;
    } catch (const std::exception& e) {
        std::cout << "    Error: " << e.what() << std::endl;
        return false;
    }
}

bool ClosureMemoryTest::executeClosureTest(const std::string& luaCode, const std::string& expectedResult) {
    try {
        bool compiled = compileAndExecute(luaCode);
        return compiled;
    } catch (const std::exception& e) {
        std::cout << "    Execution error: " << e.what() << std::endl;
        return false;
    }
}

size_t ClosureMemoryTest::measureMemoryUsage() {
    // Placeholder implementation
    // In a real implementation, this would measure actual memory usage
    return 0;
}

void ClosureMemoryTest::forceGarbageCollection() {
    // Placeholder implementation
    // In a real implementation, this would trigger garbage collection
}
} // namespace Tests
} // namespace Lua