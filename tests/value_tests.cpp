#include <gtest/gtest.h>
#include "vm/value.hpp"

using namespace Lua;

// 值类型测试
class ValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
};

// 测试基本值类型
TEST_F(ValueTest, BasicTypes) {
    // 测试空值
    Value nil;
    EXPECT_TRUE(nil.isNil());
    EXPECT_FALSE(nil.isBoolean());
    EXPECT_FALSE(nil.isNumber());
    EXPECT_FALSE(nil.isString());
    EXPECT_FALSE(nil.isTable());
    EXPECT_FALSE(nil.isFunction());
    EXPECT_EQ("nil", nil.toString());
    
    // 测试布尔值
    Value boolTrue(true);
    Value boolFalse(false);
    EXPECT_TRUE(boolTrue.isBoolean());
    EXPECT_TRUE(boolFalse.isBoolean());
    EXPECT_TRUE(boolTrue.asBoolean());
    EXPECT_FALSE(boolFalse.asBoolean());
    EXPECT_EQ("true", boolTrue.toString());
    EXPECT_EQ("false", boolFalse.toString());
    
    // 测试数字
    Value number(42.5);
    EXPECT_TRUE(number.isNumber());
    EXPECT_DOUBLE_EQ(42.5, number.asNumber());
    EXPECT_EQ("42.5", number.toString());
    
    // 测试字符串
    Value string("Hello, Lua!");
    EXPECT_TRUE(string.isString());
    EXPECT_EQ("Hello, Lua!", string.asString());
    EXPECT_EQ("\"Hello, Lua!\"", string.toString());
}

// 测试值的转换
TEST_F(ValueTest, Conversions) {
    // 数字到字符串的转换
    Value number(123);
    std::string numAsStr = number.asString();
    EXPECT_EQ("123", numAsStr);
    
    // 字符串到数字的转换 (如果实现了)
    Value numStr("456");
    EXPECT_TRUE(numStr.canConvertToNumber());
    EXPECT_DOUBLE_EQ(456.0, numStr.toNumber());
    
    Value invalidNumStr("abc");
    EXPECT_FALSE(invalidNumStr.canConvertToNumber());
}

// 测试值比较
TEST_F(ValueTest, Comparisons) {
    // 相等比较
    EXPECT_TRUE(Value().equals(Value()));          // nil == nil
    EXPECT_TRUE(Value(true).equals(Value(true)));  // true == true
    EXPECT_TRUE(Value(42).equals(Value(42)));      // 42 == 42
    EXPECT_TRUE(Value("lua").equals(Value("lua"))); // "lua" == "lua"
    
    // 不等比较
    EXPECT_FALSE(Value().equals(Value(false)));    // nil != false
    EXPECT_FALSE(Value(true).equals(Value(false))); // true != false
    EXPECT_FALSE(Value(42).equals(Value(43)));     // 42 != 43
    EXPECT_FALSE(Value("lua").equals(Value("Lua"))); // "lua" != "Lua"
    
    // 值类型比较
    EXPECT_NE(Value().type(), Value(true).type());
    EXPECT_NE(Value(true).type(), Value(42).type());
    EXPECT_NE(Value(42).type(), Value("42").type());
}

// 测试表值
TEST_F(ValueTest, TableValue) {
    auto table = make_ptr<Table>();
    table->set(Value(1), Value("one"));
    table->set(Value("name"), Value("lua"));
    
    Value tableVal(table);
    EXPECT_TRUE(tableVal.isTable());
    
    auto retrievedTable = tableVal.asTable();
    EXPECT_EQ(table, retrievedTable);
    EXPECT_EQ("one", retrievedTable->get(Value(1)).asString());
    EXPECT_EQ("lua", retrievedTable->get(Value("name")).asString());
}

// 测试函数值
TEST_F(ValueTest, FunctionValue) {
    // 创建一个本地函数 (C++ 函数)
    auto nativeFunc = [](State* state, const Vec<Value>& args) -> Value {
        return Value(args.size());  // 返回参数数量
    };
    
    auto func = make_ptr<Function>(nativeFunc);
    Value funcVal(func);
    
    EXPECT_TRUE(funcVal.isFunction());
    
    auto retrievedFunc = funcVal.asFunction();
    EXPECT_EQ(func, retrievedFunc);
    
    // 测试调用函数
    State state;
    Vec<Value> args = { Value(1), Value(2), Value(3) };
    Value result = state.call(funcVal, args);
    
    EXPECT_TRUE(result.isNumber());
    EXPECT_DOUBLE_EQ(3.0, result.asNumber());  // 应该返回参数数量
}
