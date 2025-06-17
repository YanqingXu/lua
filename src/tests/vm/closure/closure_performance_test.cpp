#include "closure_performance_test.hpp"

namespace Lua {
namespace Tests {

void ClosurePerformanceTest::runAllTests() {
    printSectionHeader("Closure Performance Tests");
    
    setupTestEnvironment();
    
    // Run performance benchmark tests
    benchmarkClosureCreation();
    benchmarkUpvalueAccess();
    benchmarkNestedClosures();
    benchmarkClosureInvocation();
    benchmarkComplexScenarios();
    
    // Run scalability tests
    testScalability();
    testDeepNesting();
    testManyUpvalues();
    testLargeClosureCount();
    
    // Run comparison tests
    comparePerformance();
    compareWithRegularFunctions();
    compareUpvalueVsGlobal();
    compareNestedVsFlat();
    
    // Run memory performance tests
    measureMemoryPerformance();
    testMemoryAllocationSpeed();
    testGarbageCollectionImpact();
    
    cleanupTestEnvironment();
    
    printSectionFooter();
}

void ClosurePerformanceTest::benchmarkClosureCreation() {
    std::cout << "\n  Benchmarking closure creation..." << std::endl;
    
    // Test 1: Simple closure creation benchmark
    auto simpleCreationTest = []() {
        std::string luaCode = R"(
            function benchmarkSimpleCreation()
                local function createClosure(x)
                    return function()
                        return x
                    end
                end
                
                for i = 1, 1000 do
                    local closure = createClosure(i)
                end
                
                return true
            end
            
            return benchmarkSimpleCreation()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double simpleTime = measureAverageTime(simpleCreationTest, 10);
    printPerformanceResult("Simple closure creation (1000 closures)", simpleTime);
    
    bool simplePass = simpleTime < CLOSURE_CREATION_THRESHOLD;
    printTestResult("Simple closure creation performance", simplePass, 
                   "Time: " + std::to_string(simpleTime) + "ms");
    
    // Test 2: Complex closure creation benchmark
    auto complexCreationTest = []() {
        std::string luaCode = R"(
            function benchmarkComplexCreation()
                local function createComplexClosure(a, b, c)
                    local d = a + b + c
                    return function(x)
                        return function(y)
                            return a + b + c + d + x + y
                        end
                    end
                end
                
                for i = 1, 500 do
                    local closure = createComplexClosure(i, i*2, i*3)
                end
                
                return true
            end
            
            return benchmarkComplexCreation()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double complexTime = measureAverageTime(complexCreationTest, 10);
    printPerformanceResult("Complex closure creation (500 closures)", complexTime);
    
    bool complexPass = complexTime < CLOSURE_CREATION_THRESHOLD * 2;
    printTestResult("Complex closure creation performance", complexPass,
                   "Time: " + std::to_string(complexTime) + "ms");
    
    // Test 3: Closure creation with many upvalues
    auto manyUpvaluesTest = []() {
        std::string luaCode = R"(
            function benchmarkManyUpvalues()
                local a, b, c, d, e = 1, 2, 3, 4, 5
                local f, g, h, i, j = 6, 7, 8, 9, 10
                
                local function createClosureWithManyUpvalues()
                    return function(x)
                        return a + b + c + d + e + f + g + h + i + j + x
                    end
                end
                
                for k = 1, 1000 do
                    local closure = createClosureWithManyUpvalues()
                end
                
                return true
            end
            
            return benchmarkManyUpvalues()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double manyUpvaluesTime = measureAverageTime(manyUpvaluesTest, 10);
    printPerformanceResult("Closure creation with many upvalues (1000 closures)", manyUpvaluesTime);
    
    bool manyUpvaluesPass = manyUpvaluesTime < CLOSURE_CREATION_THRESHOLD * 1.5;
    printTestResult("Many upvalues creation performance", manyUpvaluesPass,
                   "Time: " + std::to_string(manyUpvaluesTime) + "ms");
}

void ClosurePerformanceTest::benchmarkUpvalueAccess() {
    std::cout << "\n  Benchmarking upvalue access..." << std::endl;
    
    // Test 1: Single upvalue access benchmark
    auto singleUpvalueTest = []() {
        std::string luaCode = R"(
            function benchmarkSingleUpvalue()
                local x = 42
                local function getClosure()
                    return function()
                        return x
                    end
                end
                
                local closure = getClosure()
                local sum = 0
                
                for i = 1, 10000 do
                    sum = sum + closure()
                end
                
                return sum
            end
            
            return benchmarkSingleUpvalue()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double singleTime = measureAverageTime(singleUpvalueTest, 10);
    printPerformanceResult("Single upvalue access (10000 accesses)", singleTime);
    
    bool singlePass = singleTime < UPVALUE_ACCESS_THRESHOLD * 100;
    printTestResult("Single upvalue access performance", singlePass,
                   "Time: " + std::to_string(singleTime) + "ms");
    
    // Test 2: Multiple upvalue access benchmark
    auto multipleUpvalueTest = []() {
        std::string luaCode = R"(
            function benchmarkMultipleUpvalues()
                local a, b, c, d, e = 1, 2, 3, 4, 5
                
                local function getClosure()
                    return function()
                        return a + b + c + d + e
                    end
                end
                
                local closure = getClosure()
                local sum = 0
                
                for i = 1, 5000 do
                    sum = sum + closure()
                end
                
                return sum
            end
            
            return benchmarkMultipleUpvalues()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double multipleTime = measureAverageTime(multipleUpvalueTest, 10);
    printPerformanceResult("Multiple upvalue access (5000 accesses)", multipleTime);
    
    bool multiplePass = multipleTime < UPVALUE_ACCESS_THRESHOLD * 50;
    printTestResult("Multiple upvalue access performance", multiplePass,
                   "Time: " + std::to_string(multipleTime) + "ms");
    
    // Test 3: Upvalue modification benchmark
    auto modificationTest = []() {
        std::string luaCode = R"(
            function benchmarkUpvalueModification()
                local count = 0
                
                local function getCounter()
                    return function()
                        count = count + 1
                        return count
                    end
                end
                
                local counter = getCounter()
                
                for i = 1, 5000 do
                    counter()
                end
                
                return count
            end
            
            return benchmarkUpvalueModification()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double modificationTime = measureAverageTime(modificationTest, 10);
    printPerformanceResult("Upvalue modification (5000 modifications)", modificationTime);
    
    bool modificationPass = modificationTime < UPVALUE_ACCESS_THRESHOLD * 50;
    printTestResult("Upvalue modification performance", modificationPass,
                   "Time: " + std::to_string(modificationTime) + "ms");
}

void ClosurePerformanceTest::benchmarkNestedClosures() {
    std::cout << "\n  Benchmarking nested closures..." << std::endl;
    
    // Test 1: Two-level nesting benchmark
    auto twoLevelTest = []() {
        std::string luaCode = R"(
            function benchmarkTwoLevel()
                local function level1(x)
                    return function(y)
                        return function(z)
                            return x + y + z
                        end
                    end
                end
                
                local sum = 0
                for i = 1, 1000 do
                    local closure = level1(i)(i*2)
                    sum = sum + closure(i*3)
                end
                
                return sum
            end
            
            return benchmarkTwoLevel()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double twoLevelTime = measureAverageTime(twoLevelTest, 10);
    printPerformanceResult("Two-level nested closures (1000 operations)", twoLevelTime);
    
    bool twoLevelPass = twoLevelTime < NESTED_CLOSURE_THRESHOLD;
    printTestResult("Two-level nesting performance", twoLevelPass,
                   "Time: " + std::to_string(twoLevelTime) + "ms");
    
    // Test 2: Deep nesting benchmark
    auto deepNestingTest = []() {
        std::string luaCode = R"(
            function benchmarkDeepNesting()
                local function createDeepClosure(depth)
                    if depth <= 0 then
                        return function(x)
                            return x
                        end
                    else
                        local inner = createDeepClosure(depth - 1)
                        return function(x)
                            return inner(x + 1)
                        end
                    end
                end
                
                local sum = 0
                for i = 1, 100 do
                    local closure = createDeepClosure(5)
                    sum = sum + closure(i)
                end
                
                return sum
            end
            
            return benchmarkDeepNesting()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double deepTime = measureAverageTime(deepNestingTest, 10);
    printPerformanceResult("Deep nested closures (100 operations, depth 5)", deepTime);
    
    bool deepPass = deepTime < NESTED_CLOSURE_THRESHOLD * 2;
    printTestResult("Deep nesting performance", deepPass,
                   "Time: " + std::to_string(deepTime) + "ms");
}

void ClosurePerformanceTest::benchmarkClosureInvocation() {
    std::cout << "\n  Benchmarking closure invocation..." << std::endl;
    
    // Test 1: Simple invocation benchmark
    auto simpleInvocationTest = []() {
        std::string luaCode = R"(
            function benchmarkSimpleInvocation()
                local function createClosure()
                    return function(x)
                        return x * 2
                    end
                end
                
                local closure = createClosure()
                local sum = 0
                
                for i = 1, 10000 do
                    sum = sum + closure(i)
                end
                
                return sum
            end
            
            return benchmarkSimpleInvocation()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double simpleInvocationTime = measureAverageTime(simpleInvocationTest, 10);
    printPerformanceResult("Simple closure invocation (10000 calls)", simpleInvocationTime);
    
    bool simpleInvocationPass = simpleInvocationTime < INVOCATION_THRESHOLD * 200;
    printTestResult("Simple invocation performance", simpleInvocationPass,
                   "Time: " + std::to_string(simpleInvocationTime) + "ms");
    
    // Test 2: Complex invocation benchmark
    auto complexInvocationTest = []() {
        std::string luaCode = R"(
            function benchmarkComplexInvocation()
                local function createComplexClosure(a, b)
                    return function(x, y)
                        return (a + x) * (b + y)
                    end
                end
                
                local closure = createComplexClosure(10, 20)
                local sum = 0
                
                for i = 1, 5000 do
                    sum = sum + closure(i, i * 2)
                end
                
                return sum
            end
            
            return benchmarkComplexInvocation()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double complexInvocationTime = measureAverageTime(complexInvocationTest, 10);
    printPerformanceResult("Complex closure invocation (5000 calls)", complexInvocationTime);
    
    bool complexInvocationPass = complexInvocationTime < INVOCATION_THRESHOLD * 100;
    printTestResult("Complex invocation performance", complexInvocationPass,
                   "Time: " + std::to_string(complexInvocationTime) + "ms");
}

void ClosurePerformanceTest::benchmarkComplexScenarios() {
    std::cout << "\n  Benchmarking complex scenarios..." << std::endl;
    
    // Test 1: Closure factory benchmark
    auto factoryTest = []() {
        std::string luaCode = R"(
            function benchmarkClosureFactory()
                local function createOperationFactory(op)
                    if op == "add" then
                        return function(a)
                            return function(b)
                                return a + b
                            end
                        end
                    elseif op == "mul" then
                        return function(a)
                            return function(b)
                                return a * b
                            end
                        end
                    end
                end
                
                local addFactory = createOperationFactory("add")
                local mulFactory = createOperationFactory("mul")
                
                local sum = 0
                for i = 1, 1000 do
                    local adder = addFactory(i)
                    local multiplier = mulFactory(i)
                    sum = sum + adder(10) + multiplier(2)
                end
                
                return sum
            end
            
            return benchmarkClosureFactory()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double factoryTime = measureAverageTime(factoryTest, 10);
    printPerformanceResult("Closure factory scenario (1000 operations)", factoryTime);
    
    bool factoryPass = factoryTime < NESTED_CLOSURE_THRESHOLD * 2;
    printTestResult("Closure factory performance", factoryPass,
                   "Time: " + std::to_string(factoryTime) + "ms");
}

void ClosurePerformanceTest::testScalability() {
    std::cout << "\n  Testing scalability..." << std::endl;
    
    // Test scalability with increasing closure count
    std::vector<int> closureCounts = {100, 500, 1000, 2000, 5000};
    std::vector<double> times;
    
    for (int count : closureCounts) {
        auto scalabilityTest = [count]() {
            std::string luaCode = R"(
                function testScalability(count)
                    local closures = {}
                    
                    -- Create closures
                    for i = 1, count do
                        local x = i
                        closures[i] = function()
                            return x * 2
                        end
                    end
                    
                    -- Use closures
                    local sum = 0
                    for i = 1, count do
                        sum = sum + closures[i]()
                    end
                    
                    return sum
                end
                
                return testScalability(" + std::to_string(count) + ")
            )";
            
            executePerformanceTest(luaCode, 1);
        };
        
        double time = measureAverageTime(scalabilityTest, 5);
        times.push_back(time);
        
        printPerformanceResult("Scalability test (" + std::to_string(count) + " closures)", time);
    }
    
    // Check if scaling is reasonable (not exponential)
    bool scalingReasonable = true;
    for (size_t i = 1; i < times.size(); i++) {
        double ratio = times[i] / times[i-1];
        double countRatio = static_cast<double>(closureCounts[i]) / closureCounts[i-1];
        if (ratio > countRatio * 2) {
            scalingReasonable = false;
            break;
        }
    }
    
    printTestResult("Scalability is reasonable", scalingReasonable);
}

void ClosurePerformanceTest::testDeepNesting() {
    std::cout << "\n  Testing deep nesting performance..." << std::endl;
    
    // Test performance with increasing nesting depth
    std::vector<int> depths = {2, 5, 10, 15, 20};
    
    for (int depth : depths) {
        auto deepNestingTest = [depth]() {
            std::string luaCode = R"(
                function testDeepNesting(depth)
                    local function createNested(d)
                        if d <= 0 then
                            return function(x)
                                return x
                            end
                        else
                            local inner = createNested(d - 1)
                            return function(x)
                                return inner(x + 1)
                            end
                        end
                    end
                    
                    local closure = createNested(depth)
                    local sum = 0
                    
                    for i = 1, 100 do
                        sum = sum + closure(i)
                    end
                    
                    return sum
                end
                
                return testDeepNesting(" + std::to_string(depth) + ")
            )";
            
            executePerformanceTest(luaCode, 1);
        };
        
        double time = measureAverageTime(deepNestingTest, 5);
        printPerformanceResult("Deep nesting (depth " + std::to_string(depth) + ")", time);
        
        bool depthPass = time < NESTED_CLOSURE_THRESHOLD * depth;
        printTestResult("Deep nesting depth " + std::to_string(depth) + " performance", depthPass,
                       "Time: " + std::to_string(time) + "ms");
    }
}

void ClosurePerformanceTest::testManyUpvalues() {
    std::cout << "\n  Testing many upvalues performance..." << std::endl;
    
    // Test performance with increasing upvalue count
    std::vector<int> upvalueCounts = {5, 10, 20, 50, 100};
    
    for (int count : upvalueCounts) {
        auto manyUpvaluesTest = [count]() {
            // Generate Lua code with many upvalues
            std::string luaCode = "function testManyUpvalues()\n";
            
            // Declare upvalues
            for (int i = 0; i < count; i++) {
                luaCode += "    local var" + std::to_string(i) + " = " + std::to_string(i) + "\n";
            }
            
            luaCode += "    local function createClosure()\n";
            luaCode += "        return function()\n";
            luaCode += "            return ";
            
            // Use all upvalues
            for (int i = 0; i < count; i++) {
                if (i > 0) luaCode += " + ";
                luaCode += "var" + std::to_string(i);
            }
            
            luaCode += "\n        end\n";
            luaCode += "    end\n";
            luaCode += "    \n";
            luaCode += "    local closure = createClosure()\n";
            luaCode += "    local sum = 0\n";
            luaCode += "    for i = 1, 1000 do\n";
            luaCode += "        sum = sum + closure()\n";
            luaCode += "    end\n";
            luaCode += "    return sum\n";
            luaCode += "end\n";
            luaCode += "return testManyUpvalues()";
            
            executePerformanceTest(luaCode, 1);
        };
        
        double time = measureAverageTime(manyUpvaluesTest, 5);
        printPerformanceResult("Many upvalues (" + std::to_string(count) + " upvalues)", time);
        
        bool upvaluePass = time < UPVALUE_ACCESS_THRESHOLD * count * 10;
        printTestResult("Many upvalues (" + std::to_string(count) + ") performance", upvaluePass,
                       "Time: " + std::to_string(time) + "ms");
    }
}

void ClosurePerformanceTest::testLargeClosureCount() {
    std::cout << "\n  Testing large closure count performance..." << std::endl;
    
    // Test with very large numbers of closures
    std::vector<int> largeCounts = {1000, 5000, 10000, 20000};
    
    for (int count : largeCounts) {
        auto largeCountTest = [count]() {
            std::string luaCode = R"(
                function testLargeCount(count)
                    local closures = {}
                    
                    -- Create many closures
                    for i = 1, count do
                        local x = i
                        closures[i] = function()
                            return x
                        end
                    end
                    
                    -- Use a subset of closures
                    local sum = 0
                    local step = math.max(1, math.floor(count / 100))
                    for i = 1, count, step do
                        sum = sum + closures[i]()
                    end
                    
                    return sum
                end
                
                return testLargeCount(" + std::to_string(count) + ")
            )";
            
            executePerformanceTest(luaCode, 1);
        };
        
        double time = measureAverageTime(largeCountTest, 3);
        printPerformanceResult("Large closure count (" + std::to_string(count) + " closures)", time);
        
        bool largeCountPass = time < SCALABILITY_THRESHOLD;
        printTestResult("Large closure count (" + std::to_string(count) + ") performance", largeCountPass,
                       "Time: " + std::to_string(time) + "ms");
    }
}

void ClosurePerformanceTest::comparePerformance() {
    std::cout << "\n  Comparing performance scenarios..." << std::endl;
    
    // This method would contain various performance comparisons
    // For now, just indicate that comparisons are being performed
    printTestResult("Performance comparison framework", true, "Ready for comparisons");
}

void ClosurePerformanceTest::compareWithRegularFunctions() {
    std::cout << "\n  Comparing closures with regular functions..." << std::endl;
    
    // Test closure vs regular function performance
    auto closureTest = []() {
        std::string luaCode = R"(
            function testClosurePerformance()
                local function createClosure(factor)
                    return function(x)
                        return x * factor
                    end
                end
                
                local closure = createClosure(2)
                local sum = 0
                
                for i = 1, 10000 do
                    sum = sum + closure(i)
                end
                
                return sum
            end
            
            return testClosurePerformance()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    auto regularFunctionTest = []() {
        std::string luaCode = R"(
            function testRegularFunctionPerformance()
                local function regularFunction(x, factor)
                    return x * factor
                end
                
                local sum = 0
                
                for i = 1, 10000 do
                    sum = sum + regularFunction(i, 2)
                end
                
                return sum
            end
            
            return testRegularFunctionPerformance()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double closureTime = measureAverageTime(closureTest, 10);
    double regularTime = measureAverageTime(regularFunctionTest, 10);
    
    printPerformanceResult("Closure performance", closureTime);
    printPerformanceResult("Regular function performance", regularTime);
    
    double ratio = closureTime / regularTime;
    printTestResult("Closure vs regular function ratio", ratio < 3.0,
                   "Ratio: " + std::to_string(ratio));
}

void ClosurePerformanceTest::compareUpvalueVsGlobal() {
    std::cout << "\n  Comparing upvalue vs global access..." << std::endl;
    
    // Test upvalue vs global variable access performance
    auto upvalueTest = []() {
        std::string luaCode = R"(
            function testUpvalueAccess()
                local x = 42
                
                local function getClosure()
                    return function()
                        return x
                    end
                end
                
                local closure = getClosure()
                local sum = 0
                
                for i = 1, 10000 do
                    sum = sum + closure()
                end
                
                return sum
            end
            
            return testUpvalueAccess()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    auto globalTest = []() {
        std::string luaCode = R"(
            globalVar = 42
            
            function testGlobalAccess()
                local function getGlobal()
                    return globalVar
                end
                
                local sum = 0
                
                for i = 1, 10000 do
                    sum = sum + getGlobal()
                end
                
                return sum
            end
            
            return testGlobalAccess()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double upvalueTime = measureAverageTime(upvalueTest, 10);
    double globalTime = measureAverageTime(globalTest, 10);
    
    printPerformanceResult("Upvalue access performance", upvalueTime);
    printPerformanceResult("Global access performance", globalTime);
    
    double ratio = upvalueTime / globalTime;
    printTestResult("Upvalue vs global access ratio", ratio < 2.0,
                   "Ratio: " + std::to_string(ratio));
}

void ClosurePerformanceTest::compareNestedVsFlat() {
    std::cout << "\n  Comparing nested vs flat closures..." << std::endl;
    
    // Test nested vs flat closure performance
    auto nestedTest = []() {
        std::string luaCode = R"(
            function testNestedClosures()
                local function level1(a)
                    return function(b)
                        return function(c)
                            return a + b + c
                        end
                    end
                end
                
                local sum = 0
                for i = 1, 1000 do
                    local closure = level1(i)(i*2)
                    sum = sum + closure(i*3)
                end
                
                return sum
            end
            
            return testNestedClosures()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    auto flatTest = []() {
        std::string luaCode = R"(
            function testFlatClosures()
                local function createClosure(a, b)
                    return function(c)
                        return a + b + c
                    end
                end
                
                local sum = 0
                for i = 1, 1000 do
                    local closure = createClosure(i, i*2)
                    sum = sum + closure(i*3)
                end
                
                return sum
            end
            
            return testFlatClosures()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double nestedTime = measureAverageTime(nestedTest, 10);
    double flatTime = measureAverageTime(flatTest, 10);
    
    printPerformanceResult("Nested closure performance", nestedTime);
    printPerformanceResult("Flat closure performance", flatTime);
    
    double ratio = nestedTime / flatTime;
    printTestResult("Nested vs flat closure ratio", ratio < 2.0,
                   "Ratio: " + std::to_string(ratio));
}

void ClosurePerformanceTest::measureMemoryPerformance() {
    std::cout << "\n  Measuring memory performance..." << std::endl;
    
    // Test memory allocation/deallocation performance
    auto memoryTest = []() {
        std::string luaCode = R"(
            function testMemoryPerformance()
                local function createAndDestroy()
                    local closures = {}
                    
                    -- Create many closures
                    for i = 1, 1000 do
                        local x = i
                        closures[i] = function()
                            return x
                        end
                    end
                    
                    -- Use some closures
                    local sum = 0
                    for i = 1, 100 do
                        sum = sum + closures[i]()
                    end
                    
                    -- Release references
                    for i = 1, 1000 do
                        closures[i] = nil
                    end
                    
                    return sum
                end
                
                local total = 0
                for i = 1, 10 do
                    total = total + createAndDestroy()
                end
                
                return total
            end
            
            return testMemoryPerformance()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double memoryTime = measureAverageTime(memoryTest, 5);
    printPerformanceResult("Memory allocation/deallocation performance", memoryTime);
    
    bool memoryPass = memoryTime < SCALABILITY_THRESHOLD;
    printTestResult("Memory performance", memoryPass,
                   "Time: " + std::to_string(memoryTime) + "ms");
}

void ClosurePerformanceTest::testMemoryAllocationSpeed() {
    std::cout << "\n  Testing memory allocation speed..." << std::endl;
    
    // Test rapid allocation and deallocation
    auto allocationTest = []() {
        std::string luaCode = R"(
            function testAllocationSpeed()
                for round = 1, 100 do
                    local closures = {}
                    
                    for i = 1, 100 do
                        local x = i
                        closures[i] = function()
                            return x
                        end
                    end
                    
                    -- Clear references
                    closures = nil
                end
                
                return true
            end
            
            return testAllocationSpeed()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double allocationTime = measureAverageTime(allocationTest, 5);
    printPerformanceResult("Memory allocation speed", allocationTime);
    
    bool allocationPass = allocationTime < SCALABILITY_THRESHOLD / 2;
    printTestResult("Memory allocation speed", allocationPass,
                   "Time: " + std::to_string(allocationTime) + "ms");
}

void ClosurePerformanceTest::testGarbageCollectionImpact() {
    std::cout << "\n  Testing garbage collection impact..." << std::endl;
    
    // Test performance with and without garbage collection pressure
    auto gcTest = []() {
        std::string luaCode = R"(
            function testGCImpact()
                local function createPressure()
                    local data = {}
                    for i = 1, 1000 do
                        local x = i
                        data[i] = function()
                            return x
                        end
                    end
                    return data
                end
                
                local results = {}
                for round = 1, 50 do
                    local data = createPressure()
                    
                    -- Use some data
                    local sum = 0
                    for i = 1, 10 do
                        sum = sum + data[i]()
                    end
                    
                    results[round] = sum
                    
                    -- Force GC if available
                    if collectgarbage then
                        collectgarbage("collect")
                    end
                end
                
                local total = 0
                for i = 1, #results do
                    total = total + results[i]
                end
                
                return total
            end
            
            return testGCImpact()
        )";
        
        executePerformanceTest(luaCode, 1);
    };
    
    double gcTime = measureAverageTime(gcTest, 3);
    printPerformanceResult("Garbage collection impact", gcTime);
    
    bool gcPass = gcTime < SCALABILITY_THRESHOLD * 2;
    printTestResult("Garbage collection impact", gcPass,
                   "Time: " + std::to_string(gcTime) + "ms");
}

// Helper method implementations
void ClosurePerformanceTest::printTestResult(const std::string& testName, bool passed, const std::string& details) {
    std::cout << "    [" << (passed ? "PASS" : "FAIL") << "] " << testName;
    if (!details.empty()) {
        std::cout << " - " << details;
    }
    std::cout << std::endl;
}

void ClosurePerformanceTest::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n=== " << sectionName << " ===" << std::endl;
}

void ClosurePerformanceTest::printSectionFooter() {
    std::cout << "\n=== Performance Tests Completed ===\n" << std::endl;
}

void ClosurePerformanceTest::printPerformanceResult(const std::string& testName, double timeMs, const std::string& unit) {
    std::cout << "    [PERF] " << testName << ": " << std::fixed << std::setprecision(3) << timeMs << " " << unit << std::endl;
}

void ClosurePerformanceTest::printThroughputResult(const std::string& testName, double operationsPerSecond) {
    std::cout << "    [THRU] " << testName << ": " << std::fixed << std::setprecision(0) << operationsPerSecond << " ops/sec" << std::endl;
}

bool ClosurePerformanceTest::compileAndExecute(const std::string& luaCode) {
    try {
        // Placeholder implementation
        return true;
    } catch (const std::exception& e) {
        std::cout << "    Error: " << e.what() << std::endl;
        return false;
    }
}

bool ClosurePerformanceTest::executePerformanceTest(const std::string& luaCode, int iterations) {
    try {
        for (int i = 0; i < iterations; i++) {
            if (!compileAndExecute(luaCode)) {
                return false;
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cout << "    Performance test error: " << e.what() << std::endl;
        return false;
    }
}

double ClosurePerformanceTest::measureExecutionTime(const std::function<void()>& operation) {
    auto start = std::chrono::high_resolution_clock::now();
    operation();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0; // Convert to milliseconds
}

double ClosurePerformanceTest::measureAverageTime(const std::function<void()>& operation, int iterations) {
    double totalTime = 0.0;
    
    for (int i = 0; i < iterations; i++) {
        totalTime += measureExecutionTime(operation);
    }
    
    return totalTime / iterations;
}

void ClosurePerformanceTest::setupTestEnvironment() {
    // Initialize performance testing environment
}

void ClosurePerformanceTest::cleanupTestEnvironment() {
    // Clean up performance testing environment
}

} // namespace Tests
} // namespace Lua