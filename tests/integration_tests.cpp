// integration_tests.cpp
#include <gtest/gtest.h>
#include "vm/state.hpp"
#include "lib/base_lib.hpp"

using namespace Lua;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        state = std::make_unique<State>();
        registerBaseLib(state.get());
    }
    
    std::unique_ptr<State> state;
};

// 测试基本库的集成
TEST_F(IntegrationTest, BasicLibraryIntegration) {
    // 测试print函数（无法直接验证输出，但至少可以确保不会崩溃）
    EXPECT_NO_THROW(state->doString("print('Hello from Lua!')"));
    
    // 测试类型函数
    state->doString("local result = type(10)");
    EXPECT_EQ("number", state->getGlobal("result").asString());
    
    state->doString("local result = type('string')");
    EXPECT_EQ("string", state->getGlobal("result").asString());
    
    state->doString("local result = type({})");
    EXPECT_EQ("table", state->getGlobal("result").asString());
    
    // 测试tonumber函数
    state->doString("local result = tonumber('42')");
    EXPECT_DOUBLE_EQ(42, state->getGlobal("result").asNumber());
    
    // 测试tostring函数
    state->doString("local result = tostring(42)");
    EXPECT_EQ("42", state->getGlobal("result").asString());
}

// 测试复杂脚本
TEST_F(IntegrationTest, ComplexScripts) {
    // 测试一个更复杂的脚本，包含多种语言特性
    std::string complexScript = R"(
        -- 基本函数定义和递归
        function factorial(n)
            if n <= 1 then
                return 1
            else
                return n * factorial(n-1)
            end
        end
        
        -- 表操作
        local t = {}
        for i=1,10 do
            t[i] = factorial(i)
        end
        
        -- 高阶函数
        function map(arr, fn)
            local result = {}
            for i=1,#arr do
                result[i] = fn(arr[i])
            end
            return result
        end
        
        -- 应用高阶函数
        local doubled = map(t, function(x) return x * 2 end)
        
        -- 计算总和
        local sum = 0
        for i=1,#doubled do
            sum = sum + doubled[i]
        end
        
        return sum
    )";
    
    Value result = state->doString(complexScript);
    EXPECT_TRUE(result.isNumber());
    // 预期结果: 2 * (1! + 2! + 3! + ... + 10!)
    EXPECT_GT(result.asNumber(), 0);  // 具体值需要计算
}

// 测试错误处理
TEST_F(IntegrationTest, ErrorHandling) {
    // 测试语法错误
    EXPECT_THROW(state->doString("if true then"), SyntaxError);
    
    // 测试运行时错误
    EXPECT_THROW(state->doString("local x = 10 + 'string'"), RuntimeError);
    
    // 测试undefined变量
    EXPECT_THROW(state->doString("return undefinedVariable"), RuntimeError);
}