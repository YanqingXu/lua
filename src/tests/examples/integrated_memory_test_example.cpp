#include "../test_utils.hpp"
#include "../../common/types.hpp"
#include <iostream>
#include <vector>
#include <memory>

namespace Lua {
namespace Tests {

/**
 * 示例测试类，展示集成内存泄漏检测的使用
 */
class IntegratedMemoryTestExample {
public:
    /**
     * 基础测试方法 - 无内存泄漏
     */
    static void testBasicOperationNoLeak() {
        // 不需要手动添加内存检测代码，RUN_TEST宏会自动处理
        Vec<i32> numbers = {1, 2, 3, 4, 5};
        i32 sum = 0;
        for (i32 num : numbers) {
            sum += num;
        }
        
        if (sum != 15) {
            throw std::runtime_error("Sum calculation failed");
        }
        
        TestUtils::printInfo("Basic operation completed successfully");
    }
    
    /**
     * 复杂测试方法 - 动态内存分配
     */
    static void testDynamicAllocationNoLeak() {
        // 使用智能指针，应该没有内存泄漏
        auto ptr = std::make_unique<Vec<i32>>(1000, 42);
        
        // 验证分配
        if (ptr->size() != 1000 || (*ptr)[0] != 42) {
            throw std::runtime_error("Dynamic allocation test failed");
        }
        
        TestUtils::printInfo("Dynamic allocation test completed");
        // 智能指针会自动释放内存
    }
    
    /**
     * 模拟内存泄漏的测试方法（仅用于演示）
     */
    static void testWithIntentionalLeak() {
        // 注意：这是故意的内存泄漏，仅用于演示检测功能
        // 在实际测试中不应该有这样的代码
        
        i32* leaked_memory = new i32[100];
        for (i32 i = 0; i < 100; ++i) {
            leaked_memory[i] = i;
        }
        
        // 故意不调用 delete[] leaked_memory;
        // 内存检测器会捕获这个泄漏
        
        TestUtils::printWarning("This test intentionally leaks memory for demonstration");
    }
    
    /**
     * 长时间运行的测试方法
     */
    static void testLongRunningOperation() {
        TestUtils::printInfo("Starting long-running operation...");
        
        // 模拟长时间运行的操作
        Vec<Vec<i32>> matrix;
        for (i32 i = 0; i < 1000; ++i) {
            Vec<i32> row;
            for (i32 j = 0; j < 1000; ++j) {
                row.push_back(i * j);
            }
            matrix.push_back(std::move(row));
        }
        
        // 验证结果
        if (matrix.size() != 1000 || matrix[0].size() != 1000) {
            throw std::runtime_error("Matrix creation failed");
        }
        
        TestUtils::printInfo("Long-running operation completed");
    }
    
    /**
     * 递归测试方法
     */
    static void testRecursiveOperation() {
        i32 result = fibonacci(20);
        if (result != 6765) {
            throw std::runtime_error("Fibonacci calculation failed");
        }
        TestUtils::printInfo("Recursive operation completed");
    }
    
private:
    static i32 fibonacci(i32 n) {
        if (n <= 1) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
};

/**
 * 测试组函数 - 基础测试
 */
void runBasicIntegratedTests() {
    // 使用标准测试宏，自动包含内存检测
    RUN_TEST(IntegratedMemoryTestExample, testBasicOperationNoLeak);
    RUN_TEST(IntegratedMemoryTestExample, testDynamicAllocationNoLeak);
}

/**
 * 测试组函数 - 安全测试（不会因异常停止）
 */
void runSafeIntegratedTests() {
    // 使用安全测试宏，即使有异常也会继续执行
    SAFE_RUN_TEST(IntegratedMemoryTestExample, testBasicOperationNoLeak);
    SAFE_RUN_TEST(IntegratedMemoryTestExample, testWithIntentionalLeak); // 这个会检测到内存泄漏
    SAFE_RUN_TEST(IntegratedMemoryTestExample, testDynamicAllocationNoLeak);
}

/**
 * 测试组函数 - 综合测试
 */
void runComprehensiveIntegratedTests() {
    // 使用综合测试宏，包含所有检测功能
    RUN_COMPREHENSIVE_TEST(IntegratedMemoryTestExample, testBasicOperationNoLeak, 5000);
    RUN_COMPREHENSIVE_TEST(IntegratedMemoryTestExample, testLongRunningOperation, 30000);
    RUN_COMPREHENSIVE_TEST(IntegratedMemoryTestExample, testRecursiveOperation, 10000);
}

/**
 * 测试组函数 - 默认综合测试
 */
void runDefaultComprehensiveTests() {
    // 使用默认30秒超时的综合测试
    RUN_COMPREHENSIVE_TEST_DEFAULT(IntegratedMemoryTestExample, testBasicOperationNoLeak);
    RUN_COMPREHENSIVE_TEST_DEFAULT(IntegratedMemoryTestExample, testDynamicAllocationNoLeak);
}

/**
 * 集成内存测试套件
 */
class IntegratedMemoryTestSuite {
public:
    static void runAllTests() {
        TestUtils::printLevelHeader(TestUtils::TestLevel::SUITE, "Integrated Memory Test Suite", 
                                   "Demonstrating integrated memory leak detection");
        
        // 使用增强的测试组宏，为整个组添加内存检测
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Basic Integrated Tests", runBasicIntegratedTests);
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Safe Integrated Tests", runSafeIntegratedTests);
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Comprehensive Tests", runComprehensiveIntegratedTests);
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Default Comprehensive Tests", runDefaultComprehensiveTests);
        
        TestUtils::printLevelFooter(TestUtils::TestLevel::SUITE, "Integrated memory tests completed");
    }
};

/**
 * 主测试函数
 */
void runIntegratedMemoryExamples() {
    try {
        TestUtils::printLevelHeader(TestUtils::TestLevel::MAIN, "Integrated Memory Detection Examples",
                                   "Showcasing automatic memory leak detection in test framework");
        
        // 使用增强的测试套件宏
        RUN_TEST_SUITE_WITH_MEMORY_CHECK(IntegratedMemoryTestSuite);
        
        TestUtils::printLevelFooter(TestUtils::TestLevel::MAIN, "All integrated memory examples completed successfully");
    } catch (const std::exception& e) {
        TestUtils::printException(e, "Integrated Memory Examples");
        throw;
    } catch (...) {
        TestUtils::printUnknownException("Integrated Memory Examples");
        throw;
    }
}

} // namespace Tests
} // namespace Lua

/**
 * 主函数 - 运行所有集成内存检测示例
 */
int main() {
    try {
        // 设置测试配置
        Lua::Tests::TestUtils::setColorEnabled(true);
        
        // 运行所有示例
        RUN_MAIN_TEST("Integrated Memory Detection Examples", Lua::Tests::runIntegratedMemoryExamples);
        
        std::cout << "\n=== All tests completed successfully! ===\n" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test execution failed: " << e.what() << " ===\n" << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n=== Test execution failed with unknown exception ===\n" << std::endl;
        return 1;
    }
}