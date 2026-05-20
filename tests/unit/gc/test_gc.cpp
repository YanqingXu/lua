/**
 * @file test_gc.cpp
 * @brief GC系统单元测试 (GCObject, GarbageCollector, Upvalue)
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "../framework/test_framework.hpp"
#include "core/gc_object.hpp"
#include "core/gc_string.hpp"
#include "core/function.hpp"
#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "gc/garbage_collector.hpp"
#include "lib/baselib.hpp"
#include "vm/lua_state.hpp"

using namespace Lua;
using namespace LuaTest;

// Test GCObject implementation
class TestGCObject : public GCObject {
public:
    TestGCObject() : GCObject(GCObjectType::String) {}
    void mark(GarbageCollector& /*gc*/) override {}
    usize getSize() const override { return sizeof(TestGCObject); }
};

static i32 gcDummyCFunction(LuaState*) {
    return 0;
}

static i32 gFinalizerCalls = 0;
static i32 gFinalizerPayload = 0;

static i32 gcRecordingFinalizer(LuaState* L) {
    gFinalizerCalls++;
    if (L->isUserdata(1)) {
        Userdata* userdata = L->at(1).asUserdata();
        i32* payload = userdata->getTypedData<i32>();
        if (payload != nullptr) {
            gFinalizerPayload = *payload;
        }
    }
    return 0;
}

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

void testGarbageCollectorInstancesAreIndependent(TestSuite& suite) {
    GarbageCollector localGC;
    GarbageCollector& shimGC = GarbageCollector::getInstance();
    shimGC.clearAll();

    GCString* localString = new GCString("local-gc");
    GCString* shimString = new GCString("shim-gc");

    localGC.registerObject(localString);
    shimGC.registerObject(shimString);

    ASSERT_EQ(suite, static_cast<usize>(1), localGC.getObjectCount(), "Local GC tracks its own object");
    ASSERT_EQ(suite, static_cast<usize>(1), shimGC.getObjectCount(), "Shim GC tracks its own object");

    delete localString;

    ASSERT_EQ(suite, static_cast<usize>(0), localGC.getObjectCount(), "Deleting object unregisters from owner GC");
    ASSERT_EQ(suite, static_cast<usize>(1), shimGC.getObjectCount(), "Deleting local object does not affect shim GC");

    shimGC.clearAll();
}

void testGarbageCollectorRegister(TestSuite& suite) {
    GarbageCollector gc;

    // Get initial count (may have fixed objects)
    usize initialCount = gc.getObjectCount();

    GCString* gcStr1 = new GCString("GC Test 1");
    GCString* gcStr2 = new GCString("GC Test 2");
    Table* gcTable = new Table();

    // Test 1: Register objects
    gc.registerObject(gcStr1);
    gc.registerObject(gcStr2);
    gc.registerObject(gcTable);

    usize objCount = gc.getObjectCount();
    ASSERT_EQ(suite, initialCount + 3, objCount, "Register objects");

    // Cleanup
    gc.clearAll();
}

void testGarbageCollectorRoots(TestSuite& suite) {
    GarbageCollector gc;
    
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
    GarbageCollector gc;

    // Get initial count (may have fixed objects)
    usize initialCount = gc.getObjectCount();

    GCString* gcStr1 = new GCString("Root");
    GCString* gcStr2 = new GCString("Garbage");

    gc.registerObject(gcStr1);
    gc.registerObject(gcStr2);
    gc.addRoot(gcStr1);

    usize countBeforeGC = gc.getObjectCount();

    // Test 1: Collect garbage (should collect gcStr2)
    usize collected = gc.collect();
    ASSERT_EQ(suite, (usize)1, collected, "Garbage collected");

    // Test 2: Object count after GC (should be 1 less than before)
    usize objCountAfter = gc.getObjectCount();
    ASSERT_EQ(suite, countBeforeGC - 1, objCountAfter, "Objects after GC");

    // Cleanup
    gc.clearAll();
}

void testGarbageCollectorMarksCompositeObjects(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();

    LuaState* L = LuaState::newState();
    auto& pool = L->getGlobalState().getStringPool();

    Table* root = new Table();
    Userdata* userdata = Userdata::createFull(sizeof(i32));
    Table* userdataMetatable = new Table();
    Function* threadFunc = new Function(gcDummyCFunction);

    gc.registerObject(root);
    gc.registerObject(userdata);
    gc.registerObject(userdataMetatable);
    gc.registerObject(threadFunc);
    gc.addRoot(root);

    userdata->setMetatable(userdataMetatable);
    Thread* thread = Thread::create(L, threadFunc);

    GCString* userdataKey = pool.intern("userdata");
    GCString* threadKey = pool.intern("thread");
    root->set(Value(userdataKey), Value(userdata));
    root->set(Value(threadKey), Value(thread));

    Table* garbage = new Table();
    gc.registerObject(garbage);

    usize collected = gc.collect(L);
    ASSERT_EQ(suite, (usize)1, collected, "Only unreachable object collected");
    ASSERT_TRUE(suite, root->get(Value(userdataKey)).isUserdata(), "Table marks userdata value");
    ASSERT_TRUE(suite, root->get(Value(threadKey)).isThread(), "Table marks thread value");
    ASSERT_TRUE(suite, userdata->getMetatable() == userdataMetatable, "Userdata marks metatable");

    gc.removeRoot(root);
    delete L;
    gc.clearAll();
}

void testCollectGarbageCollectReclaimsMemory(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();

    LuaState* L = LuaState::newState();
    openBaseLib(L);

    usize beforeBytes = gc.getTotalMemory();
    for (i32 i = 0; i < 128; i++) {
        Table* garbage = new Table();
        gc.registerObject(garbage);
        garbage->setArray(1, Value(static_cast<f64>(i)));
        garbage->setArray(64, Value(static_cast<f64>(i)));
    }
    usize midBytes = gc.getTotalMemory();
    ASSERT_TRUE(suite, midBytes > beforeBytes, "Unreachable tables increase GC memory");

    L->setTop(0);
    L->pushString(L->getGlobalState().getStringPool().intern("collect"));
    i32 nresults = luaB_collectgarbage(L);
    usize afterBytes = gc.getTotalMemory();

    ASSERT_EQ(suite, 1, nresults, "collectgarbage('collect') returns one value");
    ASSERT_TRUE(suite, L->top().isNumber(), "collectgarbage('collect') returns numeric status");
    ASSERT_TRUE(suite, afterBytes < midBytes, "collectgarbage('collect') reclaims memory");

    delete L;
    gc.clearAll();
}

void testWeakTableValuesAreCleared(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();

    LuaState* L = LuaState::newState();
    auto& pool = L->getGlobalState().getStringPool();

    Table* weak = new Table();
    Table* metatable = new Table();
    Table* value = new Table();
    gc.registerObject(weak);
    gc.registerObject(metatable);
    gc.registerObject(value);
    gc.addRoot(weak);

    metatable->set(Value(L->getGlobalState().getMetamethodName(TMS::TM_MODE)), Value(pool.intern("v")));
    weak->setMetatable(metatable);

    GCString* key = pool.intern("weak-value-slot");
    weak->set(Value(key), Value(value));

    usize collected = gc.collect(L);

    ASSERT_TRUE(suite, collected >= 1, "Weak value target collected");
    ASSERT_TRUE(suite, weak->get(Value(key)).isNil(), "Weak value entry cleared");

    gc.removeRoot(weak);
    delete L;
    gc.clearAll();
}

void testWeakTableKeysAreCleared(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();

    LuaState* L = LuaState::newState();
    auto& pool = L->getGlobalState().getStringPool();

    Table* weak = new Table();
    Table* metatable = new Table();
    Table* key = new Table();
    gc.registerObject(weak);
    gc.registerObject(metatable);
    gc.registerObject(key);
    gc.addRoot(weak);

    metatable->set(Value(L->getGlobalState().getMetamethodName(TMS::TM_MODE)), Value(pool.intern("k")));
    weak->setMetatable(metatable);
    weak->set(Value(key), Value(pool.intern("weak-key-value")));

    usize collected = gc.collect(L);

    ASSERT_TRUE(suite, collected >= 1, "Weak key target collected");
    ASSERT_EQ(suite, (usize)0, weak->getHashSize(), "Weak key entry removed before sweep");

    gc.removeRoot(weak);
    delete L;
    gc.clearAll();
}

void testCollectGarbageRunsUserdataFinalizer(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();

    gFinalizerCalls = 0;
    gFinalizerPayload = 0;

    LuaState* L = LuaState::newState();
    openBaseLib(L);

    Userdata* userdata = Userdata::createFull(sizeof(i32));
    *userdata->getTypedData<i32>() = 1234;
    Table* metatable = new Table();
    Function* finalizer = new Function(gcRecordingFinalizer);

    gc.registerObject(userdata);
    gc.registerObject(metatable);
    gc.registerObject(finalizer);

    metatable->set(Value(L->getGlobalState().getMetamethodName(TMS::TM_GC)), Value(finalizer));
    userdata->setMetatable(metatable);

    L->setTop(0);
    L->pushString(L->getGlobalState().getStringPool().intern("collect"));
    i32 nresults = luaB_collectgarbage(L);

    ASSERT_EQ(suite, 1, nresults, "collectgarbage('collect') returns after finalizer");
    ASSERT_EQ(suite, 1, gFinalizerCalls, "__gc finalizer called once");
    ASSERT_EQ(suite, 1234, gFinalizerPayload, "__gc receives userdata argument");
    ASSERT_TRUE(suite, (userdata->getMarked() & GCBits::FINALIZED) != 0, "Finalized userdata survives first cycle");

    L->setTop(0);
    L->pushString(L->getGlobalState().getStringPool().intern("collect"));
    luaB_collectgarbage(L);

    ASSERT_EQ(suite, 1, gFinalizerCalls, "__gc finalizer is not called twice");

    delete L;
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
    registry.registerTest("GC", "Independent Instances", testGarbageCollectorInstancesAreIndependent);
    registry.registerTest("GC", "GC Register", testGarbageCollectorRegister);
    registry.registerTest("GC", "GC Roots", testGarbageCollectorRoots);
    registry.registerTest("GC", "GC Collect", testGarbageCollectorCollect);
    registry.registerTest("GC", "Composite Marking", testGarbageCollectorMarksCompositeObjects);
    registry.registerTest("GC", "collectgarbage Collect", testCollectGarbageCollectReclaimsMemory);
    registry.registerTest("GC", "Weak Table Values", testWeakTableValuesAreCleared);
    registry.registerTest("GC", "Weak Table Keys", testWeakTableKeysAreCleared);
    registry.registerTest("GC", "Userdata Finalizer", testCollectGarbageRunsUserdataFinalizer);
    registry.registerTest("GC", "Upvalue Open", testUpvalueOpen);
    registry.registerTest("GC", "Upvalue Closed", testUpvalueClosed);
    registry.registerTest("GC", "Upvalue Close All", testUpvalueCloseAll);
}

