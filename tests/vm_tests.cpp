// vm_tests.cpp
#include <gtest/gtest.h>
#include "vm/vm.hpp"
#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

using namespace Lua;

class VmTest : public ::testing::Test {
protected:
    void SetUp() override {
        vm = std::make_unique<VM>();
        state = std::make_unique<State>();
    }
    
    // 执行Lua代码的辅助函数
    Value execute(const std::string& source) {
        Lexer lexer(source);
        Parser parser(&lexer);
        auto ast = parser.parse();
        
        Compiler compiler;
        auto chunk = compiler.compile(ast.get());
        
        return vm->execute(chunk.get(), *state);
    }
    
    std::unique_ptr<VM> vm;
    std::unique_ptr<State> state;
};

// 测试基本算术运算
TEST_F(VmTest, ArithmeticOperations) {
    Value result = execute("return 10 + 20");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(30, result.asNumber());
    
    result = execute("return 50 - 30");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(20, result.asNumber());
    
    result = execute("return 6 * 7");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(42, result.asNumber());
    
    result = execute("return 100 / 4");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(25, result.asNumber());
    
    result = execute("return 10 % 3");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(1, result.asNumber());
    
    result = execute("return 2^3");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(8, result.asNumber());
}

// 测试比较运算和逻辑运算
TEST_F(VmTest, ComparisonAndLogic) {
    Value result = execute("return 10 > 5");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
    
    result = execute("return 10 < 5");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_FALSE(result.asBoolean());
    
    result = execute("return 10 == 10");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
    
    result = execute("return 10 ~= 5");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
    
    result = execute("return 10 >= 10");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
    
    result = execute("return 5 <= 10");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
    
    result = execute("return true and false");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_FALSE(result.asBoolean());
    
    result = execute("return true or false");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
    
    result = execute("return not false");
    EXPECT_TRUE(result.isBoolean());
    EXPECT_TRUE(result.asBoolean());
}

// 测试变量和控制结构
TEST_F(VmTest, VariablesAndControl) {
    Value result = execute(R"(
        local x = 10
        local y = 20
        return x + y
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(30, result.asNumber());
    
    result = execute(R"(
        local result = 0
        if 10 > 5 then
            result = 1
        else
            result = 2
        end
        return result
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(1, result.asNumber());
    
    result = execute(R"(
        local sum = 0
        for i=1,10 do
            sum = sum + i
        end
        return sum
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(55, result.asNumber());  // 1+2+...+10 = 55
    
    result = execute(R"(
        local i = 1
        local sum = 0
        while i <= 10 do
            sum = sum + i
            i = i + 1
        end
        return sum
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(55, result.asNumber());
}

// 测试表和函数
TEST_F(VmTest, TablesAndFunctions) {
    Value result = execute(R"(
        local t = {10, 20, 30, name = "lua"}
        return #t
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(3, result.asNumber());  // 只计算数组部分
    
    result = execute(R"(
        local t = {10, 20, 30, name = "lua"}
        return t[2]  -- Lua索引从1开始
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(20, result.asNumber());
    
    result = execute(R"(
        local t = {10, 20, 30, name = "lua"}
        return t.name
    )");
    EXPECT_TRUE(result.isString());
    EXPECT_EQ("lua", result.asString());
    
    result = execute(R"(
        function add(a, b)
            return a + b
        end
        return add(10, 20)
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(30, result.asNumber());
    
    result = execute(R"(
        local function fact(n)
            if n <= 1 then
                return 1
            else
                return n * fact(n-1)
            end
        end
        return fact(5)
    )");
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(120, result.asNumber());  // 5! = 120
}