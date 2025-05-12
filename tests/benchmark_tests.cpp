// benchmark_tests.cpp
#include <gtest/gtest.h>
#include <chrono>
#include "vm/state.hpp"
#include "lib/base_lib.hpp"

using namespace Lua;
using namespace std::chrono;

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        state = std::make_unique<State>();
        registerBaseLib(state.get());
    }
    
    // 测量函数执行时间的辅助函数
    template<typename Func>
    double measureTime(Func func) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        return duration_cast<milliseconds>(end - start).count();
    }
    
    std::unique_ptr<State> state;
};

// 基准测试 - 斐波那契数列
TEST_F(BenchmarkTest, FibonacciBenchmark) {
    std::string fibCode = R"(
        function fib(n)
            if n < 2 then
                return n
            else
                return fib(n-1) + fib(n-2)
            end
        end
        
        -- Warm-up
        fib(10)
        
        -- Actual test
        return fib(20)
    )";
    
    double time = measureTime([&]() {
        Value result = state->doString(fibCode);
        EXPECT_TRUE(result.isNumber());
        EXPECT_DOUBLE_EQ(6765, result.asNumber());  // fib(20) = 6765
    });
    
    std::cout << "Fibonacci benchmark: " << time << " ms" << std::endl;
    // 这里可以加入一些断言来确保性能符合预期
    // 但具体阈值需要根据实际测试结果来确定
}

// 基准测试 - 表操作
TEST_F(BenchmarkTest, TableBenchmark) {
    std::string tableCode = R"(
        local t = {}
        
        -- Insert
        local function insert_test()
            for i=1,10000 do
                t[i] = i
            end
        end
        
        -- Lookup
        local function lookup_test()
            local sum = 0
            for i=1,10000 do
                sum = sum + t[i]
            end
            return sum
        end
        
        -- Run tests
        insert_test()
        return lookup_test()
    )";
    
    double time = measureTime([&]() {
        Value result = state->doString(tableCode);
        EXPECT_TRUE(result.isNumber());
        EXPECT_DOUBLE_EQ(50005000, result.asNumber());  // 1+2+...+10000 = 50005000
    });
    
    std::cout << "Table benchmark: " << time << " ms" << std::endl;
}

// 基准测试 - 字符串操作
TEST_F(BenchmarkTest, StringBenchmark) {
    std::string stringCode = R"(
        local function string_test()
            local result = ""
            for i=1,1000 do
                result = result .. tostring(i)
            end
            return #result
        end
        
        return string_test()
    )";
    
    double time = measureTime([&]() {
        Value result = state->doString(stringCode);
        EXPECT_TRUE(result.isNumber());
        // 预期长度应该是所有数字1到1000的字符串长度之和
    });
    
    std::cout << "String benchmark: " << time << " ms" << std::endl;
}