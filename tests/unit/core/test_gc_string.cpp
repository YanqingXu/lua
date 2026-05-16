/**
 * @file test_gc_string.cpp
 * @brief GCString和StringPool类单元测试
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "../framework/test_framework.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"

using namespace Lua;
using namespace LuaTest;

void testGCStringCreation(TestSuite& suite) {
    GCString* str1 = new GCString("Hello, Lua!");
    
    // Test 1: String creation
    ASSERT_TRUE(suite, str1 != nullptr, "String creation");
    
    // Test 2: Get length
    ASSERT_EQ(suite, (usize)11, str1->getLength(), "String length");
    
    // Test 3: Get data
    ASSERT_TRUE(suite, std::string(str1->getData()) == "Hello, Lua!", "String data");
    
    // Test 4: c_str method
    ASSERT_TRUE(suite, std::string(str1->c_str()) == "Hello, Lua!", "c_str method");
    
    // Test 5: GC type
    ASSERT_EQ(suite, GCObjectType::String, str1->getType(), "GC type");
    
    delete str1;
}

void testGCStringHash(TestSuite& suite) {
    GCString* str1 = new GCString("Hello, Lua!");
    GCString* str2 = new GCString("Hello, Lua!");
    GCString* str3 = new GCString("Different");
    
    // Test 1: Hash computation
    usize hash1 = str1->getHash();
    ASSERT_TRUE(suite, hash1 != 0, "Hash computation");
    
    // Test 2: Same content should have same hash
    ASSERT_EQ(suite, str1->getHash(), str2->getHash(), "Same hash for same content");
    
    // Test 3: Different content should have different hash
    ASSERT_TRUE(suite, str1->getHash() != str3->getHash(), "Different hash for different content");
    
    // Test 4: Pointer comparison (not equal yet, need StringPool)
    ASSERT_TRUE(suite, str1 != str2, "Different objects before interning");
    
    delete str3;
    delete str2;
    delete str1;
}

void testStringPoolIntern(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    // ⚠️ 重要：不要调用pool.clear()！
    // StringPool是单例，GlobalState初始化时已经加入了保留字和元方法名称
    // 清空池会破坏其他测试的前提条件

    // 记录当前池大小
    usize initialSize = pool.size();

    // Test 1: Intern first string
    GCString* poolStr1 = pool.intern("Hello, World!");
    ASSERT_TRUE(suite, poolStr1 != nullptr, "Intern string");

    // Test 2: Intern same string should return same pointer
    GCString* poolStr2 = pool.intern("Hello, World!");
    ASSERT_TRUE(suite, poolStr1 == poolStr2, "Same pointer for same string");

    // Test 3: Intern different string
    GCString* poolStr3 = pool.intern("Different");
    ASSERT_TRUE(suite, poolStr1 != poolStr3, "Different pointer for different string");

    // Test 4: Pool size (应该增加了2个字符串)
    ASSERT_EQ(suite, initialSize + 2, pool.size(), "Pool size");

    // Test 5: Find existing string
    GCString* found = pool.find("Hello, World!");
    ASSERT_TRUE(suite, found == poolStr1, "Find existing string");

    // Test 6: Find non-existing string
    GCString* notFound = pool.find("Not exists");
    ASSERT_TRUE(suite, notFound == nullptr, "Find non-existing string");

    // Cleanup: 只从池中移除测试字符串；对象本身由GC统一释放
    pool.remove(poolStr3);
    pool.remove(poolStr1);
}

void testStringPoolStringView(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    // 记录初始大小
    usize initialSize = pool.size();

    // Test 1: Intern with string_view
    std::string_view sv("Test view");
    GCString* poolStr4 = pool.intern(sv);
    ASSERT_TRUE(suite, poolStr4 != nullptr, "Intern string_view");

    // Test 2: Intern same string_view
    GCString* poolStr5 = pool.intern(sv);
    ASSERT_TRUE(suite, poolStr4 == poolStr5, "Same pointer for same string_view");

    // Test 3: Pool size (应该增加1个字符串)
    ASSERT_EQ(suite, initialSize + 1, pool.size(), "Pool size after string_view");

    // Cleanup: 只从池中移除测试字符串；对象本身由GC统一释放
    pool.remove(poolStr4);
}

void testStringPoolRemove(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    // 记录初始大小
    usize initialSize = pool.size();

    GCString* str1 = pool.intern("String 1");
    GCString* str2 = pool.intern("String 2");

    // Test 1: Initial size (应该增加2个字符串)
    ASSERT_EQ(suite, initialSize + 2, pool.size(), "Initial pool size");

    // Test 2: Remove string
    pool.remove(str2);
    ASSERT_EQ(suite, initialSize + 1, pool.size(), "Pool size after remove");

    // Cleanup: 只从池中移除测试字符串；对象本身由GC统一释放
    pool.remove(str1);
}

void registerGCStringTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("GCString", "Creation", testGCStringCreation);
    registry.registerTest("GCString", "Hash", testGCStringHash);
    registry.registerTest("StringPool", "Intern", testStringPoolIntern);
    registry.registerTest("StringPool", "StringView", testStringPoolStringView);
    registry.registerTest("StringPool", "Remove", testStringPoolRemove);
}

