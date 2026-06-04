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
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "gc/garbage_collector.hpp"
#include "gc/gc_strategy.hpp"
#include "lib/baselib.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"

#include <type_traits>
#include <stdexcept>
#include <utility>

using namespace Lua;
using namespace LuaTest;

// Test GCObject implementation
class TestGCObject : public GCObject {
public:
    TestGCObject() : GCObject(GCObjectType::String) {}
    void mark(GarbageCollector& /*gc*/) override {}
    usize getSize() const override { return sizeof(TestGCObject); }
};

class ThrowingGCObject : public GCObject {
public:
    ThrowingGCObject() : GCObject(GCObjectType::String) {
        throw std::runtime_error("factory construction failure");
    }

    void mark(GarbageCollector& /*gc*/) override {}
    usize getSize() const override { return sizeof(ThrowingGCObject); }
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

struct StrategyFixtureResult {
    usize collected = 0;
    usize objectsAfter = 0;
    bool rootKeptChild = false;
};

static StrategyFixtureResult runStrategyFixture(StrView strategyName) {
    GarbageCollector gc;
    gc.useStrategy(strategyName);

    Table* root = new Table();
    Table* child = new Table();
    Table* garbage = new Table();

    gc.registerObject(root);
    gc.registerObject(child);
    gc.registerObject(garbage);
    gc.addRoot(root);

    root->setArray(1, Value(child));

    StrategyFixtureResult result;
    result.collected = gc.collect(StringPool::getInstance());
    result.objectsAfter = gc.getObjectCount();
    result.rootKeptChild = root->getArray(1).isTable() && root->getArray(1).asTable() == child;

    gc.removeRoot(root);
    gc.clearAll();
    return result;
}

static GarbageCollector& legacyGarbageCollectorForTest() {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#elif defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    GarbageCollector& gc = GarbageCollector::getInstance();
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    return gc;
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
    GarbageCollector& shimGC = legacyGarbageCollectorForTest();
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

void testGarbageCollectorSweepUsesExplicitStringPool(TestSuite& suite) {
    using SweepResult =
        decltype(std::declval<GarbageCollector&>().sweep(std::declval<StringPool&>()));
    constexpr bool sweepReturnsCount = std::is_same_v<SweepResult, usize>;

    GarbageCollector gc;
    StringPool& pool = StringPool::getInstance();
    GCString* gcStr = new GCString("explicit-sweep-pool");
    gc.registerObject(gcStr);

    usize collected = gc.sweep(pool);

    ASSERT_TRUE(suite, sweepReturnsCount, "sweep should accept an explicit StringPool and return count");
    ASSERT_EQ(suite, static_cast<usize>(1), collected, "Explicit-pool sweep should collect white string");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.getObjectCount(), "Explicit-pool sweep should unlink collected string");
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

void testUserdataOwnedFactory(TestSuite& suite) {
    UPtr<Userdata> userdata = Userdata::createFullOwned(sizeof(i32));

    ASSERT_TRUE(suite, userdata != nullptr, "createFullOwned returns unique ownership");
    ASSERT_TRUE(suite, userdata->getData() != nullptr, "owned userdata has a backing buffer");
    ASSERT_EQ(suite, sizeof(i32), userdata->getDataSize(), "owned userdata preserves requested size");

    *userdata->getTypedData<i32>() = 2026;
    ASSERT_EQ(suite, 2026, *userdata->getTypedData<i32>(), "owned userdata exposes typed payload");
}

void testGarbageCollectorCreateFactories(TestSuite& suite) {
    GarbageCollector gc;

    const usize initialCount = gc.getObjectCount();
    Table* table = gc.create<Table>();

    ASSERT_TRUE(suite, table != nullptr, "create<T> returns object");
    ASSERT_TRUE(suite, table->getOwnerCollector() == &gc, "create<T> registers object owner");
    ASSERT_EQ(suite, initialCount + 1, gc.getObjectCount(), "create<T> increments object count");

    const usize beforeThrowCount = gc.getObjectCount();
    bool threw = false;
    try {
        [[maybe_unused]] ThrowingGCObject* ignored = gc.create<ThrowingGCObject>();
    } catch (const std::runtime_error&) {
        threw = true;
    }

    ASSERT_TRUE(suite, threw, "create<T> propagates construction exceptions");
    ASSERT_EQ(suite, beforeThrowCount, gc.getObjectCount(), "create<T> leaves count unchanged on construction failure");

    Table* root = gc.createRoot<Table>();
    ASSERT_TRUE(suite, gc.isRoot(root), "createRoot<T> registers root");

    Table* fixedRoot = gc.createFixedRoot<Table>();
    ASSERT_TRUE(suite, gc.isRoot(fixedRoot), "createFixedRoot<T> registers root");
    ASSERT_TRUE(suite, (fixedRoot->getMarked() & GCBits::FIXED) != 0, "createFixedRoot<T> marks object fixed");

    gc.removeRoot(root);
    gc.removeRoot(fixedRoot);
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

void testGarbageCollectorStrategySelection(TestSuite& suite) {
    GarbageCollector gc;

    ASSERT_EQ(suite, Str("mark-sweep"), Str(gc.getStrategyName()),
              "Default GC strategy should be mark-sweep");
    ASSERT_EQ(suite, Str("mark-sweep"), Str(markSweepGCStrategy().name()),
              "MarkSweepGC strategy exposes its name");
    ASSERT_EQ(suite, Str("incremental"), Str(incrementalGCStrategy().name()),
              "IncrementalGC strategy exposes its name");

    ASSERT_TRUE(suite, gc.useStrategy("incremental"), "Collector should accept incremental strategy");
    ASSERT_EQ(suite, Str("incremental"), Str(gc.getStrategyName()),
              "Collector should switch active strategy by name");

    ASSERT_TRUE(suite, !gc.useStrategy("generational"), "Unknown GC strategy should be rejected");
    ASSERT_EQ(suite, Str("incremental"), Str(gc.getStrategyName()),
              "Rejected strategy should not change the active strategy");

    ASSERT_TRUE(suite, gc.useStrategy("mark-sweep"), "Collector should switch back to mark-sweep");
}

void testGCStrategiesHaveEquivalentReachability(TestSuite& suite) {
    const StrategyFixtureResult markSweep = runStrategyFixture("mark-sweep");
    const StrategyFixtureResult incremental = runStrategyFixture("incremental");

    ASSERT_EQ(suite, markSweep.collected, incremental.collected,
              "Strategies should collect the same unreachable object count");
    ASSERT_EQ(suite, markSweep.objectsAfter, incremental.objectsAfter,
              "Strategies should leave the same live object count");
    ASSERT_TRUE(suite, markSweep.rootKeptChild, "Mark-sweep should preserve reachable child");
    ASSERT_TRUE(suite, incremental.rootKeptChild, "Incremental placeholder should preserve reachable child");
}

void testCollectGarbageStrategyCommand(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();
    gc.useStrategy("mark-sweep");

    LuaState* L = LuaState::newState();
    openBaseLib(L);
    StringPool& pool = L->getGlobalState().getStringPool();

    L->setTop(0);
    L->pushString(pool.intern("strategy"));
    i32 nresults = luaB_collectgarbage(L);
    ASSERT_EQ(suite, 1, nresults, "collectgarbage('strategy') returns one value");
    ASSERT_TRUE(suite, L->top().isString(), "collectgarbage('strategy') returns a string");
    ASSERT_EQ(suite, Str("mark-sweep"), Str(L->top().asString()->c_str()),
              "collectgarbage('strategy') reports active strategy");

    L->setTop(0);
    L->pushString(pool.intern("strategy"));
    L->pushString(pool.intern("incremental"));
    nresults = luaB_collectgarbage(L);
    ASSERT_EQ(suite, 1, nresults,
              "collectgarbage('strategy', 'incremental') returns one value");
    ASSERT_EQ(suite, Str("incremental"), Str(gc.getStrategyName()),
              "collectgarbage should switch the active strategy");
    ASSERT_TRUE(suite, L->top().isString(), "strategy switch returns the active strategy name");
    ASSERT_EQ(suite, Str("incremental"), Str(L->top().asString()->c_str()),
              "strategy switch returns incremental");

    gc.useStrategy("mark-sweep");
    delete L;
    gc.clearAll();
}

void testCollectGarbageControlParameters(TestSuite& suite) {
    GarbageCollector& gc = GlobalState::getInstance().getGC();
    gc.clearAll();
    (void)gc.setPause(200);
    (void)gc.setStepMultiplier(200);

    LuaState* L = LuaState::newState();
    openBaseLib(L);
    StringPool& pool = L->getGlobalState().getStringPool();

    L->setTop(0);
    L->pushString(pool.intern("setpause"));
    L->pushNumber(150.0);
    i32 nresults = luaB_collectgarbage(L);
    ASSERT_EQ(suite, 1, nresults, "collectgarbage('setpause') returns one value");
    ASSERT_TRUE(suite, L->top().isNumber() && L->top().asNumber() == 200.0,
                "setpause returns the previous pause value");
    ASSERT_EQ(suite, 150, gc.getPause(), "setpause stores the new pause value");

    L->setTop(0);
    L->pushString(pool.intern("setpause"));
    L->pushString(pool.intern("250"));
    nresults = luaB_collectgarbage(L);
    ASSERT_EQ(suite, 1, nresults, "collectgarbage('setpause', numeric_string) returns one value");
    ASSERT_TRUE(suite, L->top().isNumber() && L->top().asNumber() == 150.0,
                "setpause accepts numeric strings and returns the old value");
    ASSERT_EQ(suite, 250, gc.getPause(), "setpause stores numeric string arguments");

    L->setTop(0);
    L->pushString(pool.intern("setstepmul"));
    L->pushNumber(400.0);
    nresults = luaB_collectgarbage(L);
    ASSERT_EQ(suite, 1, nresults, "collectgarbage('setstepmul') returns one value");
    ASSERT_TRUE(suite, L->top().isNumber() && L->top().asNumber() == 200.0,
                "setstepmul returns the previous step multiplier");
    ASSERT_EQ(suite, 400, gc.getStepMultiplier(), "setstepmul stores the new step multiplier");

    (void)gc.setPause(200);
    (void)gc.setStepMultiplier(200);
    delete L;
    gc.clearAll();
}

void testCollectGarbageStepRunsIncrementalCycle(TestSuite& suite) {
    GarbageCollector gc;
    StringPool& pool = StringPool::getInstance();
    gc.setStringPool(&pool);
    (void)gc.setStepMultiplier(200);

    Table* root = new Table();
    Table* child = new Table();
    gc.registerObject(root);
    gc.registerObject(child);
    gc.addRoot(root);
    root->setArray(1, Value(child));

    for (i32 i = 0; i < 24; i++) {
        Table* garbage = new Table();
        gc.registerObject(garbage);
        garbage->setArray(1, Value(static_cast<f64>(i)));
    }

    bool firstFinished = gc.step(nullptr, 0);
    ASSERT_TRUE(suite, !firstFinished, "A tiny GC step should not finish a fresh cycle");

    i32 steps = 1;
    bool finished = false;
    while (steps < 128) {
        ++steps;
        if (gc.step(nullptr, 0)) {
            finished = true;
            break;
        }
    }

    ASSERT_TRUE(suite, finished, "Repeated tiny steps should eventually finish the cycle");
    ASSERT_TRUE(suite, steps > 2, "Tiny steps should expose phased incremental progress");
    ASSERT_EQ(suite, static_cast<usize>(2), gc.getObjectCount(),
              "Incremental cycle keeps the reachable root graph only");

    Table* moreGarbage = new Table();
    gc.registerObject(moreGarbage);
    ASSERT_TRUE(suite, gc.step(nullptr, 10000), "Large GC step should complete a cycle");
    ASSERT_EQ(suite, static_cast<usize>(2), gc.getObjectCount(),
              "Large step reclaims newly unreachable object");

    gc.removeRoot(root);
    gc.clearAll();
}

void testIncrementalGCDebtTracksAllocationAndCycleCompletion(TestSuite& suite) {
    GarbageCollector gc;
    StringPool& pool = StringPool::getInstance();
    gc.setStringPool(&pool);
    (void)gc.setPause(200);
    (void)gc.setStepMultiplier(200);

    const isize initialDebt = gc.getDebtBytes();
    const usize initialThreshold = gc.getAutomaticThresholdBytes();

    Table* garbage = new Table();
    gc.registerObject(garbage);

    ASSERT_TRUE(suite, gc.getDebtBytes() > initialDebt,
                "Registering an object increases GC debt like Lua 5.1 totalbytes debt");
    ASSERT_TRUE(suite, gc.getAutomaticThresholdBytes() >= initialThreshold,
                "GC keeps an automatic collection threshold while tracking debt");

    ASSERT_TRUE(suite, gc.step(nullptr, 10000), "Large incremental step completes the current cycle");
    ASSERT_TRUE(suite, gc.getDebtBytes() <= 0,
                "Completed incremental cycle clears positive allocation debt");
    ASSERT_TRUE(suite, gc.getAutomaticThresholdBytes() >= 64 * 1024,
                "Completed incremental cycle refreshes the automatic threshold floor");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.getObjectCount(),
              "Unreachable allocation is reclaimed by the debt-driven cycle");

    gc.clearAll();
}

void testWriteBarrierPreservesTableReferenceGraph(TestSuite& suite) {
    GarbageCollector gc;
    StringPool& pool = StringPool::getInstance();

    Table* root = new Table();
    Table* child = new Table();
    Table* grandchild = new Table();

    gc.registerObject(root);
    gc.registerObject(child);
    gc.registerObject(grandchild);
    gc.addRoot(root);

    child->setArray(1, Value(grandchild));
    gc.mark();

    ASSERT_TRUE(suite, root->isBlack(), "Root table is black after marking");
    ASSERT_TRUE(suite, child->isWhite(), "Unlinked child remains white before barriered write");

    root->setArray(1, Value(child));

    ASSERT_TRUE(suite, child->isBlack(), "Table write barrier marks newly linked child");
    ASSERT_TRUE(suite, grandchild->isBlack(), "Table write barrier propagates child graph");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.sweep(pool),
              "Barriered table graph survives sweep");

    gc.removeRoot(root);
    gc.clearAll();
}

void testWriteBarrierPreservesMetatableFunctionAndUpvalueRefs(TestSuite& suite) {
    GarbageCollector gc;
    StringPool& pool = StringPool::getInstance();
    Stack dummyStack;

    Userdata* userdata = Userdata::createFull(sizeof(i32));
    Table* userdataMetatable = new Table();
    Table* userdataMetatableChild = new Table();
    Function* function = new Function(gcDummyCFunction);
    Table* env = new Table();
    Table* envChild = new Table();
    Table* closureUpvalueChild = new Table();
    Upvalue* closureUpvalue = Upvalue::createClosed(Value(closureUpvalueChild));
    Upvalue* rootUpvalue = Upvalue::createClosed(Value());
    Table* rootUpvalueChild = new Table();

    gc.registerObject(userdata);
    gc.registerObject(userdataMetatable);
    gc.registerObject(userdataMetatableChild);
    gc.registerObject(function);
    gc.registerObject(env);
    gc.registerObject(envChild);
    gc.registerObject(closureUpvalueChild);
    gc.registerObject(closureUpvalue);
    gc.registerObject(rootUpvalue);
    gc.registerObject(rootUpvalueChild);

    gc.addRoot(userdata);
    gc.addRoot(function);
    gc.addRoot(rootUpvalue);

    userdataMetatable->setArray(1, Value(userdataMetatableChild));
    env->setArray(1, Value(envChild));
    gc.mark();

    ASSERT_TRUE(suite, userdata->isBlack(), "Root userdata is black after marking");
    ASSERT_TRUE(suite, function->isBlack(), "Root function is black after marking");
    ASSERT_TRUE(suite, rootUpvalue->isBlack(), "Root upvalue is black after marking");

    userdata->setMetatable(userdataMetatable);
    function->setEnv(env);
    function->addUpvalue(closureUpvalue);
    rootUpvalue->setValue(dummyStack, Value(rootUpvalueChild));

    ASSERT_TRUE(suite, userdataMetatable->isBlack(), "Userdata metatable barrier marks metatable");
    ASSERT_TRUE(suite, userdataMetatableChild->isBlack(),
                "Userdata metatable barrier propagates metatable graph");
    ASSERT_TRUE(suite, env->isBlack(), "Function environment barrier marks env table");
    ASSERT_TRUE(suite, envChild->isBlack(), "Function environment barrier propagates env graph");
    ASSERT_TRUE(suite, closureUpvalue->isBlack(),
                "Function upvalue barrier marks newly associated upvalue");
    ASSERT_TRUE(suite, closureUpvalueChild->isBlack(),
                "Function upvalue barrier propagates closed upvalue value");
    ASSERT_TRUE(suite, rootUpvalueChild->isBlack(), "Upvalue write barrier marks new value");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.sweep(pool),
              "Barriered metatable/function/upvalue graph survives sweep");

    gc.removeRoot(userdata);
    gc.removeRoot(function);
    gc.removeRoot(rootUpvalue);
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
    registry.registerTest("GC", "Explicit StringPool Sweep", testGarbageCollectorSweepUsesExplicitStringPool);
    registry.registerTest("GC", "GC Register", testGarbageCollectorRegister);
    registry.registerTest("GC", "Userdata Owned Factory", testUserdataOwnedFactory);
    registry.registerTest("GC", "GC Create Factories", testGarbageCollectorCreateFactories);
    registry.registerTest("GC", "GC Roots", testGarbageCollectorRoots);
    registry.registerTest("GC", "GC Collect", testGarbageCollectorCollect);
    registry.registerTest("GC", "GC Strategy Selection", testGarbageCollectorStrategySelection);
    registry.registerTest("GC", "GC Strategy Equivalence", testGCStrategiesHaveEquivalentReachability);
    registry.registerTest("GC", "collectgarbage Strategy", testCollectGarbageStrategyCommand);
    registry.registerTest("GC", "collectgarbage Control Parameters", testCollectGarbageControlParameters);
    registry.registerTest("GC", "collectgarbage Incremental Step", testCollectGarbageStepRunsIncrementalCycle);
    registry.registerTest("GC", "Incremental GC Debt Tracks Allocation And Cycle Completion",
                          testIncrementalGCDebtTracksAllocationAndCycleCompletion);
    registry.registerTest("GC", "Write Barrier Table Graph", testWriteBarrierPreservesTableReferenceGraph);
    registry.registerTest("GC", "Write Barrier Object References", testWriteBarrierPreservesMetatableFunctionAndUpvalueRefs);
    registry.registerTest("GC", "Composite Marking", testGarbageCollectorMarksCompositeObjects);
    registry.registerTest("GC", "collectgarbage Collect", testCollectGarbageCollectReclaimsMemory);
    registry.registerTest("GC", "Weak Table Values", testWeakTableValuesAreCleared);
    registry.registerTest("GC", "Weak Table Keys", testWeakTableKeysAreCleared);
    registry.registerTest("GC", "Userdata Finalizer", testCollectGarbageRunsUserdataFinalizer);
    registry.registerTest("GC", "Upvalue Open", testUpvalueOpen);
    registry.registerTest("GC", "Upvalue Closed", testUpvalueClosed);
    registry.registerTest("GC", "Upvalue Close All", testUpvalueCloseAll);
}

