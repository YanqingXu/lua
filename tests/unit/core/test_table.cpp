/**
 * @file test_table.cpp
 * @brief Table类单元测试
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "../framework/test_framework.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "core/gc_string.hpp"

using namespace Lua;
using namespace LuaTest;

void testTableCreation(TestSuite& suite) {
    Table* table = new Table();
    
    // Test 1: Table creation
    ASSERT_TRUE(suite, table != nullptr, "Table creation");
    
    // Test 2: GC type
    ASSERT_EQ(suite, GCObjectType::Table, table->getType(), "GC type");
    
    delete table;
}

void testTableArrayOperations(TestSuite& suite) {
    Table* table = new Table();
    
    // Test 1: Set array elements
    table->setArray(1, Value(42.0));
    table->setArray(2, Value(true));
    
    // Test 2: Get array elements
    Value arr1 = table->getArray(1);
    Value arr2 = table->getArray(2);
    ASSERT_TRUE(suite, arr1.asNumber() == 42.0 && arr2.asBoolean() == true, "Array set/get");
    
    // Test 3: Array size
    ASSERT_EQ(suite, (usize)2, table->getArraySize(), "Array size");
    
    // Test 4: Table length
    usize len = table->length();
    ASSERT_EQ(suite, (usize)2, len, "Table length");
    
    delete table;
}

void testTableHashOperations(TestSuite& suite) {
    Table* table = new Table();
    GCString* keyStr = new GCString("name");
    GCString* valueStr = new GCString("Lua");
    
    // Test 1: Set hash element
    table->set(Value(keyStr), Value(valueStr));
    
    // Test 2: Get hash element
    Value hashVal = table->get(Value(keyStr));
    ASSERT_TRUE(suite, hashVal.isString(), "Hash set/get");
    
    // Test 3: Hash size
    ASSERT_EQ(suite, (usize)1, table->getHashSize(), "Hash size");
    
    // Test 4: Has key
    bool hasKey = table->has(Value(keyStr));
    ASSERT_TRUE(suite, hasKey, "Has key");
    
    // Test 5: Remove key
    table->remove(Value(keyStr));
    bool hasKeyAfter = table->has(Value(keyStr));
    ASSERT_FALSE(suite, hasKeyAfter, "Remove key");
    
    delete valueStr;
    delete keyStr;
    delete table;
}

void testTableMetatable(TestSuite& suite) {
    Table* table = new Table();
    Table* mt = new Table();
    
    // Test 1: Set metatable
    table->setMetatable(mt);
    
    // Test 2: Get metatable
    ASSERT_TRUE(suite, table->getMetatable() == mt, "Metatable set/get");
    
    delete mt;
    delete table;
}

void testTableMixedStorage(TestSuite& suite) {
    Table* table = new Table();
    GCString* keyStr = new GCString("key");
    
    // Test 1: Set both array and hash elements
    table->setArray(1, Value(10.0));
    table->setArray(2, Value(20.0));
    table->set(Value(keyStr), Value(30.0));
    
    // Test 2: Array size
    ASSERT_EQ(suite, (usize)2, table->getArraySize(), "Mixed: Array size");
    
    // Test 3: Hash size
    ASSERT_EQ(suite, (usize)1, table->getHashSize(), "Mixed: Hash size");
    
    // Test 4: Table length (only counts array part)
    ASSERT_EQ(suite, (usize)2, table->length(), "Mixed: Table length");
    
    delete keyStr;
    delete table;
}

void registerTableTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("Table", "Creation", testTableCreation);
    registry.registerTest("Table", "Array Operations", testTableArrayOperations);
    registry.registerTest("Table", "Hash Operations", testTableHashOperations);
    registry.registerTest("Table", "Metatable", testTableMetatable);
    registry.registerTest("Table", "Mixed Storage", testTableMixedStorage);
}

