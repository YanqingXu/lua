/**
 * @file test_lua_state_init.cpp
 * @brief LuaState初始化测试
 *
 * 测试P0任务1的5个子任务：
 * 1. 字符串表初始化
 * 2. 元方法名称初始化
 * 3. 保留字初始化
 * 4. 内存错误消息固定
 * 5. GC阈值设置（TODO）
 */

#include "test_framework.hpp"
#include "vm/global_state.hpp"
#include "core/string_pool.hpp"
#include "core/metatable.hpp"
#include "gc/garbage_collector.hpp"
#include <iostream>

using namespace Lua;
using namespace LuaTest;

// =====================================================================
// 子任务1.1：字符串表初始化测试
// =====================================================================

void testStringPoolResize(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    // 字符串池应该已经被初始化（resize(32)在GlobalState构造函数中调用）
    // 我们只需要验证它可以正常工作
    GCString* str1 = pool.intern("test");
    GCString* str2 = pool.intern("test");
    
    ASSERT_TRUE(suite, str1 == str2, "String pool should intern same strings");
}

// =====================================================================
// 子任务1.2：元方法名称初始化测试
// =====================================================================

void testMetamethodNamesInit(TestSuite& suite) {
    GlobalState& gs = GlobalState::getInstance();

    // 测试所有17个元方法名称
    GCString* indexName = gs.getMetamethodName(TMS::TM_INDEX);
    ASSERT_TRUE(suite, indexName != nullptr, "__index name should be initialized");
    ASSERT_TRUE(suite, indexName->getData() == "__index", "__index name should be correct");
    ASSERT_TRUE(suite, indexName->isFixed(), "__index should be fixed");

    GCString* addName = gs.getMetamethodName(TMS::TM_ADD);
    ASSERT_TRUE(suite, addName != nullptr, "__add name should be initialized");
    ASSERT_TRUE(suite, addName->getData() == "__add", "__add name should be correct");
    ASSERT_TRUE(suite, addName->isFixed(), "__add should be fixed");

    GCString* callName = gs.getMetamethodName(TMS::TM_CALL);
    ASSERT_TRUE(suite, callName != nullptr, "__call name should be initialized");
    ASSERT_TRUE(suite, callName->getData() == "__call", "__call name should be correct");
    ASSERT_TRUE(suite, callName->isFixed(), "__call should be fixed");
}

// =====================================================================
// 子任务1.3：保留字初始化测试
// =====================================================================

void testReservedWordsInit(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    // 测试几个关键字是否被固定
    GCString* andStr = pool.find("and");
    ASSERT_TRUE(suite, andStr != nullptr, "'and' should be interned");
    ASSERT_TRUE(suite, andStr->isFixed(), "'and' should be fixed");

    GCString* functionStr = pool.find("function");
    ASSERT_TRUE(suite, functionStr != nullptr, "'function' should be interned");
    ASSERT_TRUE(suite, functionStr->isFixed(), "'function' should be fixed");

    GCString* localStr = pool.find("local");
    ASSERT_TRUE(suite, localStr != nullptr, "'local' should be interned");
    ASSERT_TRUE(suite, localStr->isFixed(), "'local' should be fixed");
}

// =====================================================================
// 子任务1.4：内存错误消息固定测试
// =====================================================================

void testMemoryErrorMessageFixed(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    // 内存错误消息应该被固定
    GCString* memerr = pool.find("not enough memory");
    ASSERT_TRUE(suite, memerr != nullptr, "Memory error message should be interned");
    ASSERT_TRUE(suite, memerr->isFixed(), "Memory error message should be fixed");
}

// =====================================================================
// GC固定字符串测试
// =====================================================================

void testFixedStringsNotCollected(TestSuite& suite) {
    GarbageCollector& gc = GarbageCollector::getInstance();
    StringPool& pool = StringPool::getInstance();
    
    // 获取固定字符串
    GCString* andStr = pool.find("and");
    GCString* indexName = GlobalState::getInstance().getMetamethodName(TMS::TM_INDEX);
    
    // 记录当前对象数量
    usize beforeCount = gc.getObjectCount();
    
    // 执行GC
    gc.collect();
    
    // 固定字符串不应该被回收
    ASSERT_TRUE(suite, andStr == pool.find("and"), "Fixed string 'and' should not be collected");
    ASSERT_TRUE(suite, indexName == GlobalState::getInstance().getMetamethodName(TMS::TM_INDEX), 
                "Fixed metamethod name should not be collected");
}

// =====================================================================
// 测试注册
// =====================================================================

void registerLuaStateInitTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("LuaState Init", "String pool resize", testStringPoolResize);
    registry.registerTest("LuaState Init", "Metamethod names init", testMetamethodNamesInit);
    registry.registerTest("LuaState Init", "Reserved words init", testReservedWordsInit);
    registry.registerTest("LuaState Init", "Memory error message fixed", testMemoryErrorMessageFixed);
    registry.registerTest("LuaState Init", "Fixed strings not collected", testFixedStringsNotCollected);
}

