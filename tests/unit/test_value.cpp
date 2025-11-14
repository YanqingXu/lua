/**
 * @file test_value.cpp
 * @brief Value类单元测试
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "test_framework.hpp"
#include "core/value.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"

using namespace Lua;
using namespace LuaTest;

void testValueBasicTypes(TestSuite& suite) {
    // Test 1: Nil value
    Value nilVal;
    ASSERT_TRUE(suite, nilVal.isNil(), "Nil value creation");
    
    // Test 2: Boolean value
    Value boolVal(true);
    ASSERT_TRUE(suite, boolVal.isBoolean() && boolVal.asBoolean() == true, "Boolean value creation");
    
    // Test 3: Number value
    Value numVal(3.14);
    ASSERT_TRUE(suite, numVal.isNumber() && numVal.asNumber() == 3.14, "Number value creation");
    
    // Test 4: Integer value
    Value intVal(static_cast<LuaInteger>(42));
    ASSERT_TRUE(suite, intVal.isNumber() && intVal.asInteger() == 42, "Integer value creation");
}

void testValueTypeChecking(TestSuite& suite) {
    Value numVal(3.14);
    
    // Test 1: Type checking
    ASSERT_EQ(suite, ValueType::Number, numVal.getType(), "Type checking");
    
    // Test 2: Safe value access
    auto maybeNum = numVal.tryGetNumber();
    ASSERT_TRUE(suite, maybeNum.has_value() && maybeNum.value() == 3.14, "Safe value access");
}

void testValueLuaTruthSemantics(TestSuite& suite) {
    Value nilVal;
    Value falseVal(false);
    Value zeroVal(0.0);
    
    // Test 1: Nil is false
    ASSERT_TRUE(suite, nilVal.isFalse(), "Nil is false");
    
    // Test 2: false is false
    ASSERT_TRUE(suite, falseVal.isFalse(), "false is false");
    
    // Test 3: 0 is true (Lua semantics)
    ASSERT_TRUE(suite, zeroVal.isTrue(), "0 is true in Lua");
}

void testValueEquality(TestSuite& suite) {
    Value num1(42.0);
    Value num2(42.0);
    Value num3(43.0);
    
    // Test 1: Same values are equal
    ASSERT_TRUE(suite, num1 == num2, "Same values are equal");
    
    // Test 2: Different values are not equal
    ASSERT_TRUE(suite, num1 != num3, "Different values are not equal");
}

void testValueToString(TestSuite& suite) {
    Value numVal(3.14);
    Value boolVal(true);
    Value nilVal;
    
    // Test 1: Number toString
    std::string numStr = numVal.toString();
    ASSERT_TRUE(suite, !numStr.empty(), "Number toString");
    
    // Test 2: Boolean toString
    std::string boolStr = boolVal.toString();
    ASSERT_TRUE(suite, boolStr == "true", "Boolean toString");
    
    // Test 3: Nil toString
    std::string nilStr = nilVal.toString();
    ASSERT_TRUE(suite, nilStr == "nil", "Nil toString");
}

void testValueStringType(TestSuite& suite) {
    // 使用StringPool创建字符串（推荐方式）
    StringPool& pool = StringPool::getInstance();
    GCString* str = pool.intern("Hello, Lua!");
    Value strVal(str);

    // Test 1: String value creation
    ASSERT_TRUE(suite, strVal.isString(), "String value creation");

    // Test 2: String value access
    ASSERT_TRUE(suite, strVal.asString() == str, "String value access");

    // 注意：由StringPool管理，不需要手动delete
}

void registerValueTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("Value", "Basic Types", testValueBasicTypes);
    registry.registerTest("Value", "Type Checking", testValueTypeChecking);
    registry.registerTest("Value", "Lua Truth Semantics", testValueLuaTruthSemantics);
    registry.registerTest("Value", "Equality", testValueEquality);
    registry.registerTest("Value", "ToString", testValueToString);
    registry.registerTest("Value", "String Type", testValueStringType);
}

