#include "../../common/timeout_memory_detector.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

namespace Lua {
    class ComprehensiveTestExamples {
    public:
        // 示例1：正常测试 - 应该通过所有检测
        static void testNormalFunction() {
            AUTO_COMPREHENSIVE_TEST_GUARD_DEFAULT();
            
            std::cout << "Running normal test..." << std::endl;
            
            // 模拟一些正常的操作
            for (int i = 0; i < 1000; ++i) {
                LOOP_OPERATION_RECORD(i);  // 记录操作，防止被误判为死循环
                
                // 模拟一些计算
                volatile int result = i * i;
                (void)result;  // 避免未使用变量警告
                
                if (i % 100 == 0) {
                    MEMORY_CHECKPOINT("Processing step " + std::to_string(i));
                }
            }
            
            std::cout << "Normal test completed successfully" << std::endl;
        }
        
        // 示例2：无限递归测试 - 应该被递归检测器捕获
        static void testInfiniteRecursion() {
            AUTO_COMPREHENSIVE_TEST_GUARD(5000);  // 5秒超时
            
            std::cout << "Testing infinite recursion detection..." << std::endl;
            
            try {
                infiniteRecursiveFunction(0);
            } catch (const std::exception& e) {
                std::cout << "[SUCCESS] Recursion detected: " << e.what() << std::endl;
                return;
            }
            
            std::cout << "[ERROR] Recursion was not detected!" << std::endl;
        }
        
        // 示例3：死循环测试 - 应该被超时检测器捕获
        static void testInfiniteLoop() {
            AUTO_COMPREHENSIVE_TEST_GUARD(10000);  // 10秒超时
            
            std::cout << "Testing infinite loop detection..." << std::endl;
            
            // 这个循环永远不会结束，但没有记录操作
            // 应该被死循环检测器发现
            volatile bool condition = true;
            while (condition) {
                // 故意不调用 RECORD_OPERATION()
                // 模拟真正的死循环
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        // 示例4：内存泄漏 + 超时测试
        static void testMemoryLeakWithTimeout() {
            AUTO_COMPREHENSIVE_TEST_GUARD(15000);  // 15秒超时
            
            std::cout << "Testing memory leak with timeout..." << std::endl;
            
            // 故意创建内存泄漏
            for (int i = 0; i < 100; ++i) {
                LOOP_OPERATION_RECORD(i);
                
                void* leak = LEAK_TRACKED_MALLOC(1024);
                // 故意不释放内存
                (void)leak;
                
                MEMORY_CHECKPOINT("Leak iteration " + std::to_string(i));
            }
            
            std::cout << "Memory leak test completed" << std::endl;
        }
        
        // 示例5：复杂的嵌套递归测试
        static void testComplexRecursion() {
            AUTO_COMPREHENSIVE_TEST_GUARD(8000);  // 8秒超时
            
            std::cout << "Testing complex recursion patterns..." << std::endl;
            
            try {
                complexRecursiveFunction(0, 0);
                std::cout << "Complex recursion completed successfully" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[EXPECTED] Complex recursion caught: " << e.what() << std::endl;
            }
        }
        
        // 示例6：模拟编译器解析器的无限循环
        static void testParserInfiniteLoop() {
            AUTO_COMPREHENSIVE_TEST_GUARD(12000);  // 12秒超时
            
            std::cout << "Testing parser-like infinite loop..." << std::endl;
            
            // 模拟解析器卡在某个token上的情况
            int tokenPosition = 0;
            int lastPosition = -1;
            int stuckCount = 0;
            
            while (true) {
                RECORD_OPERATION();  // 记录操作
                
                // 模拟解析逻辑
                if (tokenPosition == lastPosition) {
                    stuckCount++;
                    if (stuckCount > 3) {
                        std::cout << "[SUCCESS] Parser stuck detection would trigger here" << std::endl;
                        break;  // 正常退出，避免真正的无限循环
                    }
                } else {
                    stuckCount = 0;
                }
                
                lastPosition = tokenPosition;
                
                // 模拟某种条件下token位置不前进的bug
                if (tokenPosition < 100) {
                    tokenPosition++;
                } else {
                    // 故意让token位置不变，模拟解析器bug
                    // tokenPosition保持不变
                }
                
                MEMORY_CHECKPOINT("Parser position " + std::to_string(tokenPosition));
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            std::cout << "Parser infinite loop test completed" << std::endl;
        }
        
        // 示例7：压力测试 - 大量操作但正常完成
        static void testStressTest() {
            AUTO_COMPREHENSIVE_TEST_GUARD(20000);  // 20秒超时
            
            std::cout << "Running stress test..." << std::endl;
            
            const int iterations = 10000;
            std::vector<void*> allocations;
            
            for (int i = 0; i < iterations; ++i) {
                LOOP_OPERATION_RECORD(i);
                
                // 分配内存
                void* ptr = LEAK_TRACKED_MALLOC(64);
                allocations.push_back(ptr);
                
                // 每1000次迭代释放一些内存
                if (i % 1000 == 0 && !allocations.empty()) {
                    for (int j = 0; j < 100 && !allocations.empty(); ++j) {
                        LEAK_TRACKED_FREE(allocations.back());
                        allocations.pop_back();
                    }
                    MEMORY_CHECKPOINT("Stress test iteration " + std::to_string(i));
                }
                
                // 模拟一些递归调用
                if (i % 500 == 0) {
                    try {
                        controlledRecursion(10);  // 受控的递归
                    } catch (const std::exception& e) {
                        std::cout << "Controlled recursion completed: " << e.what() << std::endl;
                    }
                }
            }
            
            // 清理剩余内存
            for (void* ptr : allocations) {
                LEAK_TRACKED_FREE(ptr);
            }
            
            std::cout << "Stress test completed successfully" << std::endl;
        }
        
        // 运行所有测试的主函数
        static void runAllTests() {
            std::cout << "=== COMPREHENSIVE TEST SUITE ===\n" << std::endl;
            
            try {
                std::cout << "\n1. Testing normal function..." << std::endl;
                testNormalFunction();
            } catch (const std::exception& e) {
                std::cout << "Normal function test failed: " << e.what() << std::endl;
            }
            
            try {
                std::cout << "\n2. Testing infinite recursion detection..." << std::endl;
                testInfiniteRecursion();
            } catch (const std::exception& e) {
                std::cout << "Infinite recursion test result: " << e.what() << std::endl;
            }
            
            // 注意：以下测试可能会导致程序终止，在实际使用中要小心
            /*
            try {
                std::cout << "\n3. Testing infinite loop detection..." << std::endl;
                testInfiniteLoop();
            } catch (const std::exception& e) {
                std::cout << "Infinite loop test result: " << e.what() << std::endl;
            }
            */
            
            try {
                std::cout << "\n4. Testing memory leak with timeout..." << std::endl;
                testMemoryLeakWithTimeout();
            } catch (const std::exception& e) {
                std::cout << "Memory leak test result: " << e.what() << std::endl;
            }
            
            try {
                std::cout << "\n5. Testing complex recursion..." << std::endl;
                testComplexRecursion();
            } catch (const std::exception& e) {
                std::cout << "Complex recursion test result: " << e.what() << std::endl;
            }
            
            try {
                std::cout << "\n6. Testing parser-like infinite loop..." << std::endl;
                testParserInfiniteLoop();
            } catch (const std::exception& e) {
                std::cout << "Parser infinite loop test result: " << e.what() << std::endl;
            }
            
            try {
                std::cout << "\n7. Running stress test..." << std::endl;
                testStressTest();
            } catch (const std::exception& e) {
                std::cout << "Stress test result: " << e.what() << std::endl;
            }
            
            std::cout << "\n=== ALL TESTS COMPLETED ===" << std::endl;
        }
        
    private:
        // 辅助函数：无限递归
        static void infiniteRecursiveFunction(int depth) {
            RECURSION_GUARD();  // 每次递归调用都检查深度
            RECORD_OPERATION(); // 记录操作
            
            std::cout << "Recursion depth: " << depth << std::endl;
            
            // 无限递归
            infiniteRecursiveFunction(depth + 1);
        }
        
        // 辅助函数：复杂递归模式
        static void complexRecursiveFunction(int depth, int branch) {
            RECURSION_GUARD();
            RECORD_OPERATION();
            
            if (depth > 50) {  // 正常的递归终止条件
                return;
            }
            
            // 多分支递归
            if (branch % 2 == 0) {
                complexRecursiveFunction(depth + 1, branch + 1);
                complexRecursiveFunction(depth + 1, branch + 2);
            } else {
                complexRecursiveFunction(depth + 1, branch * 2);
            }
        }
        
        // 辅助函数：受控递归
        static void controlledRecursion(int remaining) {
            RECURSION_GUARD();
            RECORD_OPERATION();
            
            if (remaining <= 0) {
                return;
            }
            
            controlledRecursion(remaining - 1);
        }
    };
}

// 主测试入口
int main() {
    try {
        Lua::ComprehensiveTestExamples::runAllTests();
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}