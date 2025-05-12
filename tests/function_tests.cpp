// function_tests.cpp
#include <gtest/gtest.h>
#include "vm/function.hpp"
#include "vm/state.hpp"

using namespace Lua;

class FunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        state = std::make_unique<State>();
    }
    
    std::unique_ptr<State> state;
};

// 测试本地函数
TEST_F(FunctionTest, NativeFunctions) {
    // 创建一个简单的加法函数
    auto addFunc = [](State* state, const Vec<Value>& args) -> Value {
        if (args.size() >= 2 && args[0].isNumber() && args[1].isNumber()) {
            return Value(args[0].asNumber() + args[1].asNumber());
        }
        return Value();
    };
    
    auto fn = make_ptr<Function>(addFunc);
    
    // 直接调用函数
    Vec<Value> args = { Value(10), Value(20) };
    Value result = state->call(Value(fn), args);
    
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(30, result.asNumber());
    
    // 测试参数验证
    args = { Value("not a number"), Value(20) };
    result = state->call(Value(fn), args);
    EXPECT_TRUE(result.isNil());
}

// 测试Lua函数
TEST_F(FunctionTest, LuaFunctions) {
    // 编译和执行一个Lua函数
    std::string source = "function add(a, b) return a + b end; return add";
    state->doString(source);
    
    // 栈顶应该是add函数
    Value fnValue = state->pop();
    EXPECT_TRUE(fnValue.isFunction());
    
    // 调用函数
    Vec<Value> args = { Value(5), Value(7) };
    Value result = state->call(fnValue, args);
    
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(12, result.asNumber());
}

// 测试闭包
TEST_F(FunctionTest, Closures) {
    // 创建一个闭包
    std::string source = "local counter = 0; return function() counter = counter + 1; return counter end";
    state->doString(source);
    
    // 栈顶应该是计数器函数
    Value counterFn = state->pop();
    EXPECT_TRUE(counterFn.isFunction());
    
    // 多次调用函数，验证闭包变量保持状态
    Value result1 = state->call(counterFn, {});
    EXPECT_TRUE(result1.isNumber());
    EXPECT_DOUBLE_EQ(1, result1.asNumber());
    
    Value result2 = state->call(counterFn, {});
    EXPECT_TRUE(result2.isNumber());
    EXPECT_DOUBLE_EQ(2, result2.asNumber());
    
    Value result3 = state->call(counterFn, {});
    EXPECT_TRUE(result3.isNumber());
    EXPECT_DOUBLE_EQ(3, result3.asNumber());
}

// 测试函数传递
TEST_F(FunctionTest, FunctionPassing) {
    // 创建一个高阶函数，接受函数作为参数
    std::string source = R"(
        function apply(fn, a, b)
            return fn(a, b)
        end
        
        function multiply(a, b)
            return a * b
        end
        
        return apply
    )";
    
    state->doString(source);
    Value applyFn = state->pop();
    
    // 获取multiply函数
    Value multiplyFn = state->getGlobal("multiply");
    
    // 调用apply，传入multiply作为参数
    Vec<Value> args = { multiplyFn, Value(6), Value(7) };
    Value result = state->call(applyFn, args);
    
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(42, result.asNumber());
}