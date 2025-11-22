/**
 * @file test_gc.cpp
 * @brief GC系统单元测试 (GCObject, GarbageCollector, Upvalue)
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "test_framework.hpp"
#include "core/gc_object.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "gc/garbage_collector.hpp"
#include "vm/lua_state.hpp"

using namespace Lua;
using namespace LuaTest;

// Test GCObject implementation
class TestGCObject : public GCObject {
public:
    TestGCObject() : GCObject(GCObjectType::String) {}
    void mark() override {}
    usize getSize() const override { return sizeof(TestGCObject); }
};

void testGCObjectBasics(TestSuite& suite) {
    TestGCObject* obj = new TestGCObject();
    
    // Test 1: GCObject creation
    ASSERT_TRUE(suite, obj != nullptr, "GCObject creation");
    
    // Test 2: Type checking
    ASSERT_EQ(suite, GCObjectType::String, obj->getType(), "Type checking");
    
    // Test 3: Initial color (white)
    obj->setColor(GCColor::White);
    ASSERT_TRUE(suite, obj->isWhite(), "Initial color (white)");
    
    // Test 4: Set to gray
    obj->setColor(GCColor::Gray);
    ASSERT_TRUE(suite, obj->isGray(), "Set to gray");
    
    // Test 5: Set to black
    obj->setColor(GCColor::Black);
    ASSERT_TRUE(suite, obj->isBlack(), "Set to black");
    
    // Test 6: isMarked (black is marked)
    ASSERT_TRUE(suite, obj->isMarked(), "isMarked (black)");
    
    delete obj;
}

void testGCObjectChaining(TestSuite& suite) {
    TestGCObject* obj1 = new TestGCObject();
    TestGCObject* obj2 = new TestGCObject();
    
    // Test 1: Chain objects
    obj1->setNext(obj2);
    ASSERT_TRUE(suite, obj1->getNext() == obj2, "Chain objects");
    
    delete obj2;
    delete obj1;
}

void testGarbageCollectorRegister(TestSuite& suite) {
    GarbageCollector& gc = GarbageCollector::getInstance();
    gc.clearAll(); // Start fresh
    
    GCString* gcStr1 = new GCString("GC Test 1");
    GCString* gcStr2 = new GCString("GC Test 2");
    Table* gcTable = new Table();
    
    // Test 1: Register objects
    gc.registerObject(gcStr1);
    gc.registerObject(gcStr2);
    gc.registerObject(gcTable);
    
    usize objCount = gc.getObjectCount();
    ASSERT_EQ(suite, (usize)3, objCount, "Register objects");
    
    // Cleanup
    gc.clearAll();
}

void testGarbageCollectorRoots(TestSuite& suite) {
    GarbageCollector& gc = GarbageCollector::getInstance();
    gc.clearAll();
    
    GCString* gcStr1 = new GCString("Root 1");
    GCString* gcStr2 = new GCString("Not Root");
    
    gc.registerObject(gcStr1);
    gc.registerObject(gcStr2);
    
    // Test 1: Add root objects
    gc.addRoot(gcStr1);
    
    usize rootCount = gc.getRootCount();
    ASSERT_EQ(suite, (usize)1, rootCount, "Add root objects");
    
    // Test 2: Check if root
    bool isRoot1 = gc.isRoot(gcStr1);
    bool isRoot2 = gc.isRoot(gcStr2);
    ASSERT_TRUE(suite, isRoot1 && !isRoot2, "isRoot check");
    
    // Cleanup
    gc.clearAll();
}

void testGarbageCollectorCollect(TestSuite& suite) {
    GarbageCollector& gc = GarbageCollector::getInstance();
    gc.clearAll();
    
    GCString* gcStr1 = new GCString("Root");
    GCString* gcStr2 = new GCString("Garbage");
    
    gc.registerObject(gcStr1);
    gc.registerObject(gcStr2);
    gc.addRoot(gcStr1);
    
    // Test 1: Collect garbage (should collect gcStr2)
    usize collected = gc.collect();
    ASSERT_EQ(suite, (usize)1, collected, "Garbage collected");
    
    // Test 2: Object count after GC
    usize objCountAfter = gc.getObjectCount();
    ASSERT_EQ(suite, (usize)1, objCountAfter, "Objects after GC");
    
    // Cleanup
    gc.clearAll();
}

void testUpvalueOpen(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    L->pushNumber(42.0);
    L->pushNumber(100.0);
    
    // Test 1: Create open upvalue
    Upvalue* uv1 = L->findOrCreateUpvalue(1);
    ASSERT_TRUE(suite, uv1 != nullptr && uv1->isOpen(), "Create open upvalue");

    // Test 2: Upvalue value (✅ 改进：传入stack引用)
    ASSERT_TRUE(suite, uv1->getValue(L->getStack()).asNumber() == 42.0, "Upvalue value");

    // Test 3: Upvalue sharing
    Upvalue* uv2 = L->findOrCreateUpvalue(1);
    ASSERT_TRUE(suite, uv1 == uv2, "Upvalue sharing");

    delete L;
}

void testUpvalueClosed(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    L->pushNumber(42.0);

    Upvalue* uv1 = L->findOrCreateUpvalue(1);

    // Test 1: Close upvalue (✅ 改进：传入stack引用)
    uv1->close(L->getStack());
    ASSERT_TRUE(suite, uv1->isClosed(), "Close upvalue");

    // Test 2: Closed value preserved (✅ 改进：传入stack引用)
    ASSERT_TRUE(suite, uv1->getValue(L->getStack()).asNumber() == 42.0, "Closed value preserved");

    delete L;
}

void testUpvalueCloseAll(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    L->pushNumber(42.0);
    L->pushNumber(100.0);
    
    Upvalue* uv1 = L->findOrCreateUpvalue(1);
    Upvalue* uv2 = L->findOrCreateUpvalue(2);
    
    // Test 1: Close all upvalues
    L->closeUpvalues(1);
    ASSERT_TRUE(suite, uv1->isClosed() && uv2->isClosed(), "Close all upvalues");
    
    delete L;
}

void registerGCTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("GC", "GCObject Basics", testGCObjectBasics);
    registry.registerTest("GC", "GCObject Chaining", testGCObjectChaining);
    registry.registerTest("GC", "GC Register", testGarbageCollectorRegister);
    registry.registerTest("GC", "GC Roots", testGarbageCollectorRoots);
    registry.registerTest("GC", "GC Collect", testGarbageCollectorCollect);
    registry.registerTest("GC", "Upvalue Open", testUpvalueOpen);
    registry.registerTest("GC", "Upvalue Closed", testUpvalueClosed);
    registry.registerTest("GC", "Upvalue Close All", testUpvalueCloseAll);
}

