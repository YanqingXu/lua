#include <gtest/gtest.h>
#include "vm/table.hpp"
#include "vm/value.hpp"

using namespace Lua;

// Lua表测试
class TableTest : public ::testing::Test {
protected:
    void SetUp() override {
        table = make_ptr<Table>();
    }
    
    Ptr<Table> table;
};

// 测试表的基本操作
TEST_F(TableTest, BasicOperations) {
    // 测试设置和获取值
    table->set(Value(1), Value("one"));
    table->set(Value(2), Value("two"));
    table->set(Value("name"), Value("lua"));
    
    EXPECT_EQ("one", table->get(Value(1)).asString());
    EXPECT_EQ("two", table->get(Value(2)).asString());
    EXPECT_EQ("lua", table->get(Value("name")).asString());
    
    // 测试获取不存在的键
    EXPECT_TRUE(table->get(Value(3)).isNil());
    EXPECT_TRUE(table->get(Value("unknown")).isNil());
    
    // 测试更新值
    table->set(Value(1), Value("ONE"));
    EXPECT_EQ("ONE", table->get(Value(1)).asString());
    
    // 测试删除值（设置为nil）
    table->set(Value(1), Value());
    EXPECT_TRUE(table->get(Value(1)).isNil());
}

// 测试表的长度操作符
TEST_F(TableTest, LengthOperator) {
    // 添加连续的整数键
    table->set(Value(1), Value("a"));
    table->set(Value(2), Value("b"));
    table->set(Value(3), Value("c"));
    
    EXPECT_EQ(3, table->length());
    
    // 添加一个中间的空洞
    table->set(Value(2), Value());
    EXPECT_EQ(1, table->length()); // 在Lua 5.1中，空洞会终止序列
    
    // 重新填充空洞
    table->set(Value(2), Value("b"));
    EXPECT_EQ(3, table->length());
    
    // 添加非连续键
    table->set(Value(5), Value("e"));
    EXPECT_EQ(3, table->length()); // 非连续键不影响长度
}

// 测试表的迭代
TEST_F(TableTest, Iteration) {
    // 填充表
    table->set(Value("a"), Value(1));
    table->set(Value("b"), Value(2));
    table->set(Value("c"), Value(3));
    
    // 使用迭代器
    std::unordered_map<std::string, double> expected = {
        {"a", 1}, {"b", 2}, {"c", 3}
    };
    std::unordered_map<std::string, double> actual;
    
    auto iter = table->iterator();
    Value key, value;
    while (iter->next(key, value)) {
        EXPECT_TRUE(key.isString());
        EXPECT_TRUE(value.isNumber());
        actual[key.asString()] = value.asNumber();
    }
    
    EXPECT_EQ(expected.size(), actual.size());
    for (const auto& [k, v] : expected) {
        EXPECT_TRUE(actual.find(k) != actual.end());
        EXPECT_DOUBLE_EQ(v, actual[k]);
    }
}

// 测试表的元表行为
TEST_F(TableTest, Metatable) {
    // 创建主表和元表
    Ptr<Table> metatable = make_ptr<Table>();
    
    // 设置元表
    table->setMetatable(metatable);
    EXPECT_EQ(metatable, table->metatable());
    
    // 设置__index元方法
    Ptr<Table> fallbackTable = make_ptr<Table>();
    fallbackTable->set(Value("key"), Value("value"));
    metatable->set(Value("__index"), Value(fallbackTable));
    
    // 测试__index元方法
    EXPECT_TRUE(table->get(Value("key")).isString());
    EXPECT_EQ("value", table->get(Value("key")).asString());
    
    // 主表中设置同名键会覆盖元表
    table->set(Value("key"), Value("direct"));
    EXPECT_EQ("direct", table->get(Value("key")).asString());
}

// 测试具有复杂类型键的表
TEST_F(TableTest, ComplexKeys) {
    // 使用表作为键
    Ptr<Table> key1 = make_ptr<Table>();
    Ptr<Table> key2 = make_ptr<Table>();
    
    table->set(Value(key1), Value("table1"));
    table->set(Value(key2), Value("table2"));
    
    EXPECT_EQ("table1", table->get(Value(key1)).asString());
    EXPECT_EQ("table2", table->get(Value(key2)).asString());
    
    // 不同的对象，即使内容相同也是不同的键
    Ptr<Table> key3 = make_ptr<Table>();
    EXPECT_TRUE(table->get(Value(key3)).isNil());
}

// 测试表的数组部分
TEST_F(TableTest, ArrayPart) {
    // 填充一个数组风格的表
    for (int i = 1; i <= 100; i++) {
        table->set(Value(i), Value(i * 10));
    }
    
    // 验证数组部分
    for (int i = 1; i <= 100; i++) {
        EXPECT_TRUE(table->get(Value(i)).isNumber());
        EXPECT_DOUBLE_EQ(i * 10, table->get(Value(i)).asNumber());
    }
    
    // 测试表长度
    EXPECT_EQ(100, table->length());
}

// 测试表的哈希部分
TEST_F(TableTest, HashPart) {
    // 填充哈希表部分
    for (int i = 0; i < 100; i++) {
        std::string key = "key" + std::to_string(i);
        table->set(Value(key), Value(i));
    }
    
    // 验证哈希部分
    for (int i = 0; i < 100; i++) {
        std::string key = "key" + std::to_string(i);
        EXPECT_TRUE(table->get(Value(key)).isNumber());
        EXPECT_DOUBLE_EQ(i, table->get(Value(key)).asNumber());
    }
}
