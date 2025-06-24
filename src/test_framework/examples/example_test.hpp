#ifndef LUA_TEST_FRAMEWORK_EXAMPLE_TEST_HPP
#define LUA_TEST_FRAMEWORK_EXAMPLE_TEST_HPP

#include "../core/test_macros.hpp"
#include "../core/test_utils.hpp"
#include "../core/test_memory.hpp"
#include "common/types.hpp"
#include <stdexcept>
#include <iostream>

namespace Lua {
namespace TestFramework {
namespace Examples {

/**
 * @brief 示例测试类 - 演示如何使用新的测试框架
 * 
 * 这个示例展示了测试框架的各种功能和最佳实践。
 */
class ExampleTestSuite {
public:
    /**
     * @brief 运行所有示例测试
     */
    static void runAllTests() {
        // 运行不同类型的测试组
        RUN_TEST_GROUP("Basic Tests", runBasicTests);
        RUN_TEST_GROUP("Memory Tests", runMemoryTests);
        RUN_TEST_GROUP("Error Handling Tests", runErrorHandlingTests);
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Memory-Safe Tests", runMemorySafeTests);
    }
    
private:
    /**
     * @brief 基础测试组
     */
    static void runBasicTests() {
        RUN_TEST(ExampleTestClass, testBasicFunctionality);
        RUN_TEST(ExampleTestClass, testStringOperations);
        RUN_TEST(ExampleTestClass, testMathOperations);
    }
    
    /**
     * @brief 内存测试组
     */
    static void runMemoryTests() {
        RUN_TEST(ExampleTestClass, testMemoryAllocation);
        RUN_TEST(ExampleTestClass, testMemoryDeallocation);
    }
    
    /**
     * @brief 错误处理测试组
     */
    static void runErrorHandlingTests() {
        RUN_TEST(ExampleTestClass, testExceptionHandling);
        RUN_TEST(ExampleTestClass, testErrorRecovery);
    }
    
    /**
     * @brief 内存安全测试组
     */
    static void runMemorySafeTests() {
        RUN_TEST(ExampleTestClass, testNoMemoryLeaks);
        RUN_TEST(ExampleTestClass, testProperCleanup);
    }
};

/**
 * @brief 示例测试类 - 包含具体的测试方法
 */
class ExampleTestClass {
public:
    /**
     * @brief 测试基础功能
     */
    static void testBasicFunctionality() {
        TestUtils::printInfo("Testing basic functionality...");
        
        // 模拟一些基础测试
        i32 result = 2 + 2;
        if (result != 4) {
            throw std::runtime_error("Basic math failed: 2 + 2 != 4");
        }
        
        TestUtils::printInfo("Basic functionality test passed");
    }
    
    /**
     * @brief 测试字符串操作
     */
    static void testStringOperations() {
        TestUtils::printInfo("Testing string operations...");
        
        Str str1 = "Hello";
        Str str2 = "World";
        Str result = str1 + " " + str2;
        
        if (result != "Hello World") {
            throw std::runtime_error("String concatenation failed");
        }
        
        TestUtils::printInfo("String operations test passed");
    }
    
    /**
     * @brief 测试数学操作
     */
    static void testMathOperations() {
        TestUtils::printInfo("Testing math operations...");
        
        // 测试各种数学操作
        f64 a = 10.5;
        f64 b = 3.2;
        
        if (a + b < 13.0 || a + b > 14.0) {
            throw std::runtime_error("Addition test failed");
        }
        
        if (a * b < 33.0 || a * b > 34.0) {
            throw std::runtime_error("Multiplication test failed");
        }
        
        TestUtils::printInfo("Math operations test passed");
    }
    
    /**
     * @brief 测试内存分配
     */
    static void testMemoryAllocation() {
        MEMORY_LEAK_TEST_GUARD("Memory Allocation Test");
        
        TestUtils::printInfo("Testing memory allocation...");
        
        // 分配一些内存
        i32* ptr = new i32[100];
        
        // 使用内存
        for (i32 i = 0; i < 100; ++i) {
            ptr[i] = i;
        }
        
        // 验证内存内容
        for (i32 i = 0; i < 100; ++i) {
            if (ptr[i] != i) {
                delete[] ptr;
                throw std::runtime_error("Memory content verification failed");
            }
        }
        
        // 释放内存
        delete[] ptr;
        
        TestUtils::printInfo("Memory allocation test passed");
    }
    
    /**
     * @brief 测试内存释放
     */
    static void testMemoryDeallocation() {
        MEMORY_LEAK_TEST_GUARD("Memory Deallocation Test");
        
        TestUtils::printInfo("Testing memory deallocation...");
        
        // 测试多次分配和释放
        for (i32 i = 0; i < 10; ++i) {
            char* buffer = new char[1024];
            // 使用buffer
            buffer[0] = 'A';
            buffer[1023] = 'Z';
            delete[] buffer;
        }
        
        TestUtils::printInfo("Memory deallocation test passed");
    }
    
    /**
     * @brief 测试异常处理
     */
    static void testExceptionHandling() {
        TestUtils::printInfo("Testing exception handling...");
        
        bool exceptionCaught = false;
        
        try {
            // 故意抛出异常
            throw std::runtime_error("Test exception");
        } catch (const std::runtime_error& e) {
            exceptionCaught = true;
            TestUtils::printInfo("Caught expected exception: " + Str(e.what()));
        }
        
        if (!exceptionCaught) {
            throw std::runtime_error("Exception was not caught properly");
        }
        
        TestUtils::printInfo("Exception handling test passed");
    }
    
    /**
     * @brief 测试错误恢复
     */
    static void testErrorRecovery() {
        TestUtils::printInfo("Testing error recovery...");
        
        // 模拟错误情况和恢复
        bool errorOccurred = true;
        bool recovered = false;
        
        if (errorOccurred) {
            TestUtils::printWarning("Simulated error occurred");
            // 执行恢复操作
            recovered = true;
            TestUtils::printInfo("Recovery operation completed");
        }
        
        if (!recovered) {
            throw std::runtime_error("Error recovery failed");
        }
        
        TestUtils::printInfo("Error recovery test passed");
    }
    
    /**
     * @brief 测试无内存泄漏
     */
    static void testNoMemoryLeaks() {
        MEMORY_LEAK_TEST_GUARD("No Memory Leaks Test");
        
        TestUtils::printInfo("Testing for memory leaks...");
        
        // 执行一些可能导致内存泄漏的操作
        for (i32 i = 0; i < 100; ++i) {
            Str* str = new Str("Test string " + std::to_string(i));
            // 正确释放内存
            delete str;
        }
        
        TestUtils::printInfo("No memory leaks test passed");
    }
    
    /**
     * @brief 测试正确清理
     */
    static void testProperCleanup() {
        MEMORY_LEAK_TEST_GUARD("Proper Cleanup Test");
        
        TestUtils::printInfo("Testing proper cleanup...");
        
        // 创建需要清理的资源
        struct Resource {
            i32* data;
            Resource() : data(new i32[50]) {}
            ~Resource() { delete[] data; }
        };
        
        // 使用RAII确保正确清理
        {
            Resource resource;
            // 使用资源
            resource.data[0] = 42;
            resource.data[49] = 99;
        } // 资源在这里自动清理
        
        TestUtils::printInfo("Proper cleanup test passed");
    }
};

/**
 * @brief 运行示例测试的便捷函数
 */
inline void runExampleTests() {
    RUN_TEST_SUITE(ExampleTestSuite);
}

} // namespace Examples
} // namespace TestFramework
} // namespace Lua

#endif // LUA_TEST_FRAMEWORK_EXAMPLE_TEST_HPP