#include "compiler_error_test.hpp"
#include "../../common/memory_leak_detector.hpp"
#include <iostream>

namespace Lua {
namespace Tests {
    
    // 示例1: 使用自动内存泄漏检测的测试函数
    void CompilerErrorTest::testVariableOutOfScopeWithMemoryDetection() {
        // 在函数入口处添加这个宏，自动检测内存泄漏
        AUTO_MEMORY_LEAK_TEST_GUARD();
        
        std::string source = R"(
            do
                local x = 1
            end
            return x  -- x is out of scope
        )";
        
        // 在关键点添加内存检查点
        MEMORY_CHECKPOINT("Before compilation");
        
        bool hasError = compileAndCheckError(source);
        
        MEMORY_CHECKPOINT("After compilation");
        
        printTestResult("Variable out of scope detection", hasError);
        
        // 函数结束时，析构函数会自动检测并报告内存泄漏
    }
    
    // 示例2: 使用自定义测试名称的内存检测
    void CompilerErrorTest::testInvalidAssignmentsWithCustomName() {
        MEMORY_LEAK_TEST_GUARD("Invalid Assignments Memory Test");
        
        std::string source = R"(
            local x, y = test(), test()
            x = "invalid"
            y = nil
            return x + y
        )";
        
        bool hasError = compileAndCheckError(source);
        printTestResult("Invalid assignments detection", hasError);
    }
    
    // 示例3: 模拟有内存泄漏的测试函数
    void CompilerErrorTest::testWithIntentionalLeak() {
        AUTO_MEMORY_LEAK_TEST_GUARD();
        
        // 故意创建内存泄漏用于演示
        char* leakedMemory = (char*)LEAK_TRACKED_MALLOC(1024);
        strcpy(leakedMemory, "This memory will be leaked for demonstration");
        
        std::string source = R"(
            local x = 1
            return x
        )";
        
        bool hasError = compileAndCheckError(source);
        printTestResult("Test with intentional leak", hasError);
        
        // 注意：我们故意不释放 leakedMemory，这会被检测到
        // LEAK_TRACKED_FREE(leakedMemory);  // 取消注释这行可以修复泄漏
    }
    
    // 示例4: 使用断言确保没有内存泄漏
    void CompilerErrorTest::testWithMemoryAssertion() {
        AUTO_MEMORY_LEAK_TEST_GUARD();
        
        std::string source = R"(
            function test()
                local x = 1
                return x
            end
            return test()
        )";
        
        bool hasError = compileAndCheckError(source);
        printTestResult("Function test", hasError);
        
        // 在测试结束前断言没有内存泄漏
        ASSERT_NO_MEMORY_LEAKS();
    }
    
    // 示例5: 复杂的内存使用模式测试
    void CompilerErrorTest::testComplexMemoryPattern() {
        AUTO_MEMORY_LEAK_TEST_GUARD();
        
        MEMORY_CHECKPOINT("Test start");
        
        // 模拟复杂的内存分配模式
        std::vector<void*> allocations;
        
        for (int i = 0; i < 10; ++i) {
            void* ptr = LEAK_TRACKED_MALLOC(100 * (i + 1));
            allocations.push_back(ptr);
            
            if (i % 3 == 0) {
                MEMORY_CHECKPOINT("Allocation batch " + std::to_string(i/3));
            }
        }
        
        // 编译测试
        std::string source = R"(
            local function factorial(n)
                if n <= 1 then
                    return 1
                else
                    return n * factorial(n - 1)
                end
            end
            return factorial(5)
        )";
        
        bool hasError = compileAndCheckError(source);
        printTestResult("Factorial function test", hasError);
        
        MEMORY_CHECKPOINT("Before cleanup");
        
        // 清理分配的内存
        for (void* ptr : allocations) {
            LEAK_TRACKED_FREE(ptr);
        }
        
        MEMORY_CHECKPOINT("After cleanup");
    }
    
    // 运行所有内存检测测试的函数
    void CompilerErrorTest::runMemoryLeakTests() {
        std::cout << "\n=== Running Memory Leak Detection Tests ===\n" << std::endl;
        
        testVariableOutOfScopeWithMemoryDetection();
        testInvalidAssignmentsWithCustomName();
        testWithIntentionalLeak();
        testWithMemoryAssertion();
        testComplexMemoryPattern();
        
        std::cout << "\n=== Memory Leak Detection Tests Completed ===\n" << std::endl;
    }
}}

// 使用示例的主函数
int main() {
    Lua::Tests::CompilerErrorTest test;
    test.runMemoryLeakTests();
    return 0;
}