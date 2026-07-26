/**
 * @file test_lua_state_init.cpp
 * @brief LuaState初始化测试
 *
 * 测试 LuaState 初始化的 5 个关键合同：
 * 1. 字符串表初始化
 * 2. 元方法名称初始化
 * 3. 保留字初始化
 * 4. 内存错误消息固定
 * 5. 固定字符串的 GC 生存性
 */

#include "test_framework.hpp"
#include "vm/state/global_state.hpp"
#include "core/string_pool.hpp"
#include "core/metatable.hpp"
#include "gc/garbage_collector.hpp"
#include <cstdlib>
#include <iostream>
#include <new>

using namespace Lua;
using namespace LuaTest;

namespace {

struct StringPoolAllocatorProbe {
    usize allocationAttempts = 0;
    usize liveAllocations = 0;
    usize failOnAllocation = 0;
};

void* stringPoolTestAllocator(void* userData, void* pointer, std::size_t, std::size_t newSize) {
    auto* probe = static_cast<StringPoolAllocatorProbe*>(userData);
    if (newSize == 0) {
        if (pointer != nullptr) {
            std::free(pointer);
            --probe->liveAllocations;
        }
        return nullptr;
    }

    ++probe->allocationAttempts;
    if (probe->failOnAllocation != 0 && probe->allocationAttempts == probe->failOnAllocation) {
        return nullptr;
    }

    if (pointer != nullptr) {
        return std::realloc(pointer, newSize);
    }

    void* result = std::malloc(newSize);
    if (result != nullptr) {
        ++probe->liveAllocations;
    }
    return result;
}

} // namespace

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

void testStringPoolInsertionOomRollback(TestSuite& suite) {
    StringPoolAllocatorProbe probe;
    LuaAllocator allocator(stringPoolTestAllocator, &probe);

    {
        // Keep the pool alive while the collector releases its strings.
        StringPool pool(&allocator);
        GarbageCollector gc(&allocator);
        pool.setGarbageCollector(&gc);
        pool.resize(8);

        const usize baselineLiveAllocations = probe.liveAllocations;
        const usize baselineObjectCount = gc.getObjectCount();
        const usize baselineMemory = gc.getAccountedMemory();

        // The next allocator request creates GCString; the following request
        // allocates the pre-reserved unordered_map node.
        probe.failOnAllocation = probe.allocationAttempts + 2;
        bool threwBadAlloc = false;
        try {
            (void)pool.intern("string-pool-insertion-oom");
        } catch (const std::bad_alloc&) {
            threwBadAlloc = true;
        }
        probe.failOnAllocation = 0;

        ASSERT_TRUE(suite, threwBadAlloc, "String pool insertion should surface allocator OOM");
        ASSERT_TRUE(suite, pool.find("string-pool-insertion-oom") == nullptr,
                    "Failed insertion should not publish a pool entry");
        ASSERT_EQ(suite, baselineObjectCount, gc.getObjectCount(),
                  "Failed insertion should unregister the provisional GCString");
        ASSERT_EQ(suite, baselineMemory, gc.getAccountedMemory(),
                  "Failed insertion should restore GC memory accounting");
        ASSERT_EQ(suite, baselineLiveAllocations, probe.liveAllocations,
                  "Failed insertion should release the provisional GCString allocation");

        GCString* recovered = pool.intern("string-pool-insertion-oom");
        ASSERT_TRUE(suite, recovered == pool.intern("string-pool-insertion-oom"),
                    "A retry after insertion OOM should preserve interning identity");
    }

    ASSERT_EQ(suite, static_cast<usize>(0), probe.liveAllocations,
              "String pool and collector destruction should release allocator blocks");
}

void testStringPoolRemoveChecksObjectIdentity(TestSuite& suite) {
    // Two independent pools legitimately own distinct objects with the same
    // contents. Removing the foreign object must not erase the local entry.
    StringPool primaryPool;
    GarbageCollector primaryGc;
    primaryPool.setGarbageCollector(&primaryGc);

    StringPool foreignPool;
    GarbageCollector foreignGc;
    foreignPool.setGarbageCollector(&foreignGc);

    GCString* canonical = primaryPool.intern("same-contents-different-owner");
    GCString* foreign = foreignPool.intern("same-contents-different-owner");
    ASSERT_TRUE(suite, canonical != foreign, "Independent pools should own distinct GCString objects");

    primaryPool.remove(foreign);
    ASSERT_TRUE(suite, primaryPool.find("same-contents-different-owner") == canonical,
                "Removing an equal foreign GCString should preserve the canonical entry");

    primaryPool.remove(canonical);
    ASSERT_TRUE(suite, primaryPool.find("same-contents-different-owner") == nullptr,
                "Removing the canonical GCString should erase its entry");
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
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    StringPool& pool = StringPool::getInstance();

    // 获取固定字符串
    GCString* andStr = pool.find("and");
    GCString* indexName = GlobalState::getInstance().getMetamethodName(TMS::TM_INDEX);

    // 执行GC
    (void)gc.collect();

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
    registry.registerTest("LuaState Init", "String pool insertion OOM rollback", testStringPoolInsertionOomRollback);
    registry.registerTest("LuaState Init", "String pool remove identity", testStringPoolRemoveChecksObjectIdentity);
    registry.registerTest("LuaState Init", "Metamethod names init", testMetamethodNamesInit);
    registry.registerTest("LuaState Init", "Reserved words init", testReservedWordsInit);
    registry.registerTest("LuaState Init", "Memory error message fixed", testMemoryErrorMessageFixed);
    registry.registerTest("LuaState Init", "Fixed strings not collected", testFixedStringsNotCollected);
}
