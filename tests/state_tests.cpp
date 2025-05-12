// state_tests.cpp
#include <gtest/gtest.h>
#include "vm/state.hpp"

using namespace Lua;

class StateTest : public ::testing::Test {
protected:
    void SetUp() override {
        state = std::make_unique<State>();
    }
    
    std::unique_ptr<State> state;
};

// 测试全局变量操作
TEST_F(StateTest, GlobalVariables) {
    // 设置和获取全局变量
    state->setGlobal("x", Value(10));
    state->setGlobal("y", Value("test"));
    
    EXPECT_TRUE(state->getGlobal("x").isNumber());
    EXPECT_DOUBLE_EQ(10, state->getGlobal("x").asNumber());
    
    EXPECT_TRUE(state->getGlobal("y").isString());
    EXPECT_EQ("test", state->getGlobal("y").asString());
    
    // 未定义的全局变量应返回nil
    EXPECT_TRUE(state->getGlobal("z").isNil());
    
    // 更新全局变量
    state->setGlobal("x", Value(20));
    EXPECT_DOUBLE_EQ(20, state->getGlobal("x").asNumber());
    
    // 删除全局变量（设置为nil）
    state->setGlobal("x", Value());
    EXPECT_TRUE(state->getGlobal("x").isNil());
}

// 测试栈操作
TEST_F(StateTest, StackOperations) {
    // 测试入栈
    state->push(Value(1));
    state->push(Value(2));
    state->push(Value("three"));
    
    EXPECT_EQ(3, state->getTop());
    
    // 测试索引访问
    EXPECT_TRUE(state->get(1).isNumber());
    EXPECT_DOUBLE_EQ(1, state->get(1).asNumber());
    
    EXPECT_TRUE(state->get(3).isString());
    EXPECT_EQ("three", state->get(3).asString());
    
    // 测试修改栈元素
    state->set(2, Value("two"));
    EXPECT_TRUE(state->get(2).isString());
    EXPECT_EQ("two", state->get(2).asString());
    
    // 测试出栈
    Value v = state->pop();
    EXPECT_TRUE(v.isString());
    EXPECT_EQ("three", v.asString());
    EXPECT_EQ(2, state->getTop());
    
    // 测试清空栈
    state->clearStack();
    EXPECT_EQ(0, state->getTop());
}

// 测试注册表
TEST_F(StateTest, Registry) {
    // 测试注册表操作
    int ref = state->ref(Value("registered value"));
    EXPECT_GT(ref, 0);
    
    Value retrieved = state->getRef(ref);
    EXPECT_TRUE(retrieved.isString());
    EXPECT_EQ("registered value", retrieved.asString());
    
    // 解除引用
    state->unref(ref);
    retrieved = state->getRef(ref);
    EXPECT_TRUE(retrieved.isNil());
}

// 测试函数调用
TEST_F(StateTest, FunctionCall) {
    // 注册和调用本地函数
    auto addFunc = [](State* state, const Vec<Value>& args) -> Value {
        if (args.size() >= 2 && args[0].isNumber() && args[1].isNumber()) {
            return Value(args[0].asNumber() + args[1].asNumber());
        }
        return Value();
    };
    
    state->setGlobal("add", Value(make_ptr<Function>(addFunc)));
    
    // 通过名称调用函数
    Vec<Value> args = { Value(5), Value(3) };
    Value result = state->callGlobal("add", args);
    
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(8, result.asNumber());
    
    // 直接调用函数值
    Value fn = state->getGlobal("add");
    result = state->call(fn, args);
    
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(8, result.asNumber());
}