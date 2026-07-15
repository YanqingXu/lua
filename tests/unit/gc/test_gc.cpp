/**
 * @file test_gc.cpp
 * @brief GC系统单元测试 (GCObject, GarbageCollector, Upvalue)
 *
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "compiler/parser/parser.hpp"
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
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
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
    usize getSize() const override {
        return sizeof(TestGCObject);
    }
};

class ThrowingGCObject : public GCObject {
public:
    ThrowingGCObject() : GCObject(GCObjectType::String) {
        throw std::runtime_error("factory construction failure");
    }

    void mark(GarbageCollector& /*gc*/) override {}
    usize getSize() const override {
        return sizeof(ThrowingGCObject);
    }
};

class ThrowingMarkGCObject : public GCObject {
public:
    explicit ThrowingMarkGCObject(i32& destructorCalls)
        : GCObject(GCObjectType::String), destructorCalls_(destructorCalls) {}

    ~ThrowingMarkGCObject() override {
        ++destructorCalls_;
    }

    void mark(GarbageCollector& /*gc*/) override {
        throw std::runtime_error("registration mark failure");
    }

    usize getSize() const override {
        return sizeof(ThrowingMarkGCObject);
    }

private:
    i32& destructorCalls_;
};

struct GCAllocatorProbe {
    usize allocations = 0;
    usize deallocations = 0;
};

static void* gcTrackingAllocator(void* userData, void* pointer, std::size_t oldSize, std::size_t newSize) {
    auto* probe = static_cast<GCAllocatorProbe*>(userData);
    if (newSize == 0) {
        if (pointer != nullptr) {
            ++probe->deallocations;
            ::operator delete(pointer);
        }
        return static_cast<void*>(nullptr);
    }

    void* replacement = ::operator new(newSize, std::nothrow);
    if (replacement == nullptr) {
        return replacement;
    }

    ++probe->allocations;
    if (pointer != nullptr) {
        std::memcpy(replacement, pointer, std::min(oldSize, newSize));
        ++probe->deallocations;
        ::operator delete(pointer);
    }
    return replacement;
}

static i32 gcDummyCFunction(LuaState*) {
    return 0;
}

static i32 gFinalizerCalls = 0;
static i32 gFinalizerPayload = 0;
static i32 gReentrantFinalizerCalls = 0;

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
#pragma warning(disable : 4996)
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
    GarbageCollector secondGC;

    ASSERT_TRUE(suite, &shimGC != &GlobalState::getInstance().getGC(),
                "Deprecated collector shim remains independent of the process GlobalState collector");

    GCString* localString = new GCString("local-gc");
    GCString* secondString = new GCString("second-gc");

    localGC.registerObject(localString);
    secondGC.registerObject(secondString);

    ASSERT_EQ(suite, static_cast<usize>(1), localGC.getObjectCount(), "Local GC tracks its own object");
    ASSERT_EQ(suite, static_cast<usize>(1), secondGC.getObjectCount(), "Second GC tracks its own object");

    delete localString;

    ASSERT_EQ(suite, static_cast<usize>(0), localGC.getObjectCount(), "Deleting object unregisters from owner GC");
    ASSERT_EQ(suite, static_cast<usize>(1), secondGC.getObjectCount(),
              "Deleting a local object does not affect another collector");

    secondGC.clearAll();
}

void testGarbageCollectorSweepUsesExplicitStringPool(TestSuite& suite) {
    using SweepResult = decltype(std::declval<GarbageCollector&>().sweep(std::declval<StringPool&>()));
    constexpr bool sweepReturnsCount = std::is_same_v<SweepResult, usize>;

    GarbageCollector gc;
    StringPool pool;
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

static i32 gcReentrantFinalizer(LuaState* L) {
    ++gReentrantFinalizerCalls;
    (void)L->getGlobalState().getGC().collect(L);
    return 0;
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

void testAllocatorFactoryCleansUpRegistrationFailure(TestSuite& suite) {
    GCAllocatorProbe probe;
    LuaAllocator allocator(gcTrackingAllocator, &probe);
    GarbageCollector gc(&allocator);
    (void)gc.createRoot<Table>();

    // Registration scans constructor-owned edges during an active cycle.
    // Inject a non-allocation mark failure after placement construction so
    // the allocator-backed factory must roll back every ownership layer.
    (void)gc.step(nullptr, 0);
    const usize objectCountBefore = gc.getObjectCount();
    const usize liveAllocationsBefore = probe.allocations - probe.deallocations;
    i32 destructorCalls = 0;
    bool threw = false;
    try {
        [[maybe_unused]] ThrowingMarkGCObject* ignored = gc.create<ThrowingMarkGCObject>(destructorCalls);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    ASSERT_TRUE(suite, threw, "allocator-backed create<T> propagates registration mark failures");
    ASSERT_EQ(suite, 1, destructorCalls, "registration failure destroys the placement-constructed object");
    ASSERT_EQ(suite, objectCountBefore, gc.getObjectCount(), "registration failure restores the collector object list");
    ASSERT_EQ(suite, liveAllocationsBefore, probe.allocations - probe.deallocations,
              "registration failure returns the object block to the configured allocator");
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

    ASSERT_EQ(suite, Str("mark-sweep"), Str(gc.getStrategyName()), "Default GC strategy should be mark-sweep");
    ASSERT_EQ(suite, Str("mark-sweep"), Str(markSweepGCStrategy().name()), "MarkSweepGC strategy exposes its name");
    ASSERT_EQ(suite, Str("incremental"), Str(incrementalGCStrategy().name()),
              "IncrementalGC strategy exposes its name");

    ASSERT_TRUE(suite, gc.useStrategy("incremental"), "Collector should accept incremental strategy");
    ASSERT_EQ(suite, Str("incremental"), Str(gc.getStrategyName()), "Collector should switch active strategy by name");

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
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    gc.useStrategy("mark-sweep");
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
    ASSERT_EQ(suite, 1, nresults, "collectgarbage('strategy', 'incremental') returns one value");
    ASSERT_EQ(suite, Str("incremental"), Str(gc.getStrategyName()), "collectgarbage should switch the active strategy");
    ASSERT_TRUE(suite, L->top().isString(), "strategy switch returns the active strategy name");
    ASSERT_EQ(suite, Str("incremental"), Str(L->top().asString()->c_str()), "strategy switch returns incremental");

    gc.useStrategy("mark-sweep");
    delete L;
}

void testCollectGarbageControlParameters(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    (void)gc.setPause(200);
    (void)gc.setStepMultiplier(200);

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
    ASSERT_EQ(suite, static_cast<usize>(2), gc.getObjectCount(), "Large step reclaims newly unreachable object");

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
    ASSERT_TRUE(suite, gc.getDebtBytes() <= 0, "Completed incremental cycle clears positive allocation debt");
    ASSERT_TRUE(suite, gc.getAutomaticThresholdBytes() >= usize{64} * 1024,
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
    ASSERT_EQ(suite, static_cast<usize>(0), gc.sweep(pool), "Barriered table graph survives sweep");

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
    ASSERT_TRUE(suite, userdataMetatableChild->isBlack(), "Userdata metatable barrier propagates metatable graph");
    ASSERT_TRUE(suite, env->isBlack(), "Function environment barrier marks env table");
    ASSERT_TRUE(suite, envChild->isBlack(), "Function environment barrier propagates env graph");
    ASSERT_TRUE(suite, closureUpvalue->isBlack(), "Function upvalue barrier marks newly associated upvalue");
    ASSERT_TRUE(suite, closureUpvalueChild->isBlack(), "Function upvalue barrier propagates closed upvalue value");
    ASSERT_TRUE(suite, rootUpvalueChild->isBlack(), "Upvalue write barrier marks new value");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.sweep(pool),
              "Barriered metatable/function/upvalue graph survives sweep");

    gc.removeRoot(userdata);
    gc.removeRoot(function);
    gc.removeRoot(rootUpvalue);
    gc.clearAll();
}

void testGarbageCollectorMarksCompositeObjects(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
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
}

void testCollectGarbageCollectReclaimsMemory(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
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
}

void testFastMemoryAccountingTracksDynamicObjects(TestSuite& suite) {
    GarbageCollector gc;

    Table* growing = new Table();
    Table* survivor = new Table();
    Proto* proto = new Proto();
    gc.registerObject(growing);
    gc.registerObject(survivor);
    gc.registerObject(proto);

    for (i32 i = 1; i <= 512; ++i) {
        growing->setArray(i, Value(static_cast<LuaNumber>(i)));
        (void)proto->addInstruction(CREATE_ABC(OpCode::MOVE, 0, 0, 0));
        (void)proto->addConstant(Value(static_cast<LuaNumber>(i)));
    }

    ASSERT_EQ(suite, gc.getTotalMemory(), gc.getAccountedMemory(),
              "fast memory ledger tracks Table and Proto capacity growth");

    delete growing;
    ASSERT_TRUE(suite, gc.getAccountedMemory() > 0,
                "unregistering one object does not clear the remaining memory ledger");
    ASSERT_EQ(suite, gc.getTotalMemory(), gc.getAccountedMemory(),
              "fast memory ledger remains exact after unregistering one object");

    delete survivor;
    delete proto;
    ASSERT_EQ(suite, static_cast<usize>(0), gc.getAccountedMemory(),
              "fast memory ledger returns to zero after all objects unregister");
}

void testIncrementalBarriersPublishConstructedAndProtoGraphs(TestSuite& suite) {
    GarbageCollector gc;
    StringPool pool;
    pool.setGarbageCollector(&gc);

    Proto* functionProto = gc.create<Proto>();
    Table* closedValue = gc.create<Table>();
    GCString* source = pool.intern("incremental-source");
    Table* constant = gc.create<Table>();
    GCString* slot = pool.intern("incremental-slot");
    Proto* subProto = gc.create<Proto>();
    GCString* localName = pool.intern("incremental-local");
    GCString* upvalueName = pool.intern("incremental-upvalue");

    // Start a cycle after every child exists. They are white, while objects
    // allocated from this point onward are black and must publish both their
    // constructor-owned edges and any later Proto mutations.
    (void)gc.step(nullptr, 0);
    ASSERT_TRUE(suite, functionProto->isWhite() && closedValue->isWhite() && source->isWhite(),
                "pre-existing children begin the active cycle white");

    Function* function = gc.createRoot<Function>(functionProto);
    Upvalue* upvalue = gc.createRoot<Upvalue>(Value(closedValue));
    Proto* owner = gc.createRoot<Proto>();

    owner->setSource(source);
    (void)owner->addConstant(Value(constant));
    (void)owner->appendConstantSlot(Value(slot));
    (void)owner->addProto(subProto);
    (void)owner->addLocVar(localName, 0, 1, 0);
    (void)owner->addUpvalueName(upvalueName);

    ASSERT_TRUE(suite, functionProto->isBlack(), "registration scans Function constructor edges");
    ASSERT_TRUE(suite, closedValue->isBlack(), "registration scans closed Upvalue constructor edges");
    ASSERT_TRUE(suite,
                source->isBlack() && constant->isBlack() && slot->isBlack() && subProto->isBlack() &&
                    localName->isBlack() && upvalueName->isBlack(),
                "Proto mutators barrier every GC-managed edge");

    bool finished = false;
    for (i32 i = 0; i < 32 && !finished; ++i) {
        finished = gc.step(nullptr, 0);
    }

    Stack unusedStack;
    ASSERT_TRUE(suite, finished, "incremental cycle with published graphs completes");
    ASSERT_TRUE(suite, function->getProto() == functionProto,
                "Function retains its constructor-owned Proto after incremental sweep");
    ASSERT_TRUE(suite,
                upvalue->getValue(unusedStack).isTable() && upvalue->getValue(unusedStack).asTable() == closedValue,
                "closed Upvalue retains its constructor-owned value after incremental sweep");
    ASSERT_TRUE(suite,
                owner->getSource() == source && owner->getConstant(0).asTable() == constant &&
                    owner->getConstant(1).asString() == slot && owner->getSubProto(0) == subProto &&
                    owner->getLocVar(0).varname == localName && owner->getUpvalueName(0) == upvalueName,
                "Proto retains all barriered edges after incremental sweep");
    ASSERT_EQ(suite, static_cast<usize>(11), gc.getObjectCount(),
              "incremental sweep keeps the complete rooted constructor and Proto graphs");

    gc.removeRoot(function);
    gc.removeRoot(upvalue);
    gc.removeRoot(owner);
    gc.clearAll(pool);
}

void testIncrementalSweepRegistrationPreservesCursor(TestSuite& suite) {
    GarbageCollector gc;
    (void)gc.create<Table>();

    // Pause -> Propagate -> Atomic -> Sweep. The original head is white and
    // is the next object the sweep cursor will remove.
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);

    Table* fresh = gc.createRoot<Table>();
    bool finished = false;
    for (i32 i = 0; i < 16 && !finished; ++i) {
        finished = gc.step(nullptr, 0);
    }

    ASSERT_TRUE(suite, finished, "sweep-time registration preserves the active cursor and completes the cycle");
    ASSERT_EQ(suite, static_cast<usize>(1), gc.getObjectCount(),
              "active sweep removes the old white object but keeps the new rooted allocation");
    ASSERT_TRUE(suite, gc.getTotalMemory() >= fresh->getSize(),
                "new allocation remains linked in the collector object list");
    ASSERT_EQ(suite, gc.getTotalMemory(), gc.getAccountedMemory(),
              "sweep-time insertion keeps exact and fast ledgers aligned");

    gc.removeRoot(fresh);
    (void)gc.collect(StringPool::getInstance());
    ASSERT_EQ(suite, static_cast<usize>(0), gc.getObjectCount(),
              "the preserved sweep-time allocation remains collectible later");
}

void testIncrementalSweepRegistrationCleansPrefilledWeakTable(TestSuite& suite) {
    GarbageCollector gc;
    GlobalState& global = GlobalState::getInstance();
    Table* child = gc.create<Table>();
    Table* metatable = gc.create<Table>();
    metatable->set(Value(global.getMetamethodName(TMS::TM_MODE)), Value(global.getStringPool().intern("v")));

    // Reach Sweep after Atomic has already completed weak-entry cleanup.
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);

    auto weakOwner = std::make_unique<Table>();
    Table* weak = weakOwner.get();
    weak->setMetatable(metatable);
    weak->setArray(1, Value(child));
    gc.registerObject(weak);
    [[maybe_unused]] const auto releasedWeak = weakOwner.release();
    gc.addRoot(weak);

    bool finished = false;
    for (i32 i = 0; i < 16 && !finished; ++i) {
        finished = gc.step(nullptr, 0);
    }

    ASSERT_TRUE(suite, finished, "pre-filled weak table registration restarts and completes a fresh cycle");
    ASSERT_TRUE(suite, weak->getArray(1).isNil(),
                "fresh atomic weak cleanup removes the value before its target is swept");
    ASSERT_EQ(suite, static_cast<usize>(2), gc.getObjectCount(),
              "restarted sweep keeps only the rooted weak table and its metatable");

    gc.removeRoot(weak);
    gc.clearAll();
}

void testAutomaticCollectorFinishesCycleBelowStartThreshold(TestSuite& suite) {
    GarbageCollector gc;
    (void)gc.create<Table>();
    (void)gc.create<Table>();
    (void)gc.create<Table>();

    // Enter Sweep explicitly while the heap and debt are still below the
    // automatic start threshold.
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);

    usize reportedCollected = 0;
    for (i32 i = 0; i < 8; ++i) {
        reportedCollected += gc.maybeCollectAutomatic(nullptr);
    }

    ASSERT_EQ(suite, static_cast<usize>(0), gc.getObjectCount(),
              "automatic checkpoints finish an in-progress sweep below the start threshold");
    ASSERT_EQ(suite, static_cast<usize>(3), reportedCollected,
              "automatic checkpoint reports the objects collected by the completed cycle");
}

void testClosingUpvalueDuringIncrementalSweepRestartsMark(TestSuite& suite) {
    GarbageCollector gc;
    Stack stack;

    Table* grandchild = gc.create<Table>();
    Table* child = gc.create<Table>();
    child->setArray(1, Value(grandchild));
    stack.push(Value());
    Upvalue* upvalue = gc.create<Upvalue>(0, stack);
    gc.addRoot(upvalue);

    // Pause -> Propagate -> Atomic -> Sweep.  An open upvalue does not mark
    // its stack slot when no LuaState root is supplied, so child remains
    // white while the rooted upvalue is black.
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);
    ASSERT_TRUE(suite, upvalue->isBlack(), "root upvalue reaches incremental sweep as black");
    ASSERT_TRUE(suite, child->isWhite(), "open upvalue stack value is white before close");

    // Stack slots have no write barrier.  Install the white graph only after
    // the open upvalue itself has been scanned.
    stack[0] = Value(child);
    upvalue->close(stack);
    bool finished = false;
    for (i32 i = 0; i < 16 && !finished; ++i) {
        finished = gc.step(nullptr, 0);
    }

    ASSERT_TRUE(suite, finished, "closing during sweep restarts and completes a safe mark cycle");
    ASSERT_EQ(suite, static_cast<usize>(3), gc.getObjectCount(),
              "closed upvalue preserves its complete child graph across sweep restart");
    ASSERT_TRUE(suite, upvalue->getValue(stack).isTable() && upvalue->getValue(stack).asTable() == child,
                "closed upvalue keeps the original child value");
    ASSERT_TRUE(suite, child->getArray(1).isTable() && child->getArray(1).asTable() == grandchild,
                "closed upvalue child keeps its grandchild");

    gc.removeRoot(upvalue);
    gc.clearAll();
}

void testExplicitCollectionKeepsAnonymousTemporaries(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    openBaseLib(L);

    const Str source = R"(
        local weak = setmetatable({}, {__mode = 'v'})
        local function make()
            local value = {}
            weak[1] = value
            return value
        end
        local result = {make(), collectgarbage('collect')}
        assert(type(result[1]) == 'table' and weak[1] == result[1])
    )";
    RuntimeServices services(L->getGlobalState());
    Parser parser(source, services);
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, parsed.has_value(), "anonymous-temporary GC regression parses");
    if (!parsed.has_value()) {
        delete L;
        return;
    }

    Chunk chunk = std::move(*parsed);
    CodeGenerator codegen(services);
    Proto* proto = codegen.generate(chunk, "gc_anonymous_temporary");
    ASSERT_TRUE(suite, proto != nullptr, "anonymous-temporary GC regression compiles");
    if (proto != nullptr) {
        Function* function = gc.create<Function>(proto);
        function->setEnv(L->getGlobalTable());
        L->pushFunction(function);
        ASSERT_EQ(suite, LUA_OK, L->pcall(0, 0, 0), "explicit collection keeps anonymous expression temporaries alive");
    }

    delete L;
}

void testWeakTableValuesAreCleared(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
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
}

void testWeakTableKeysAreCleared(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
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
}

void testIncrementalWeakToStrongModeTransition(TestSuite& suite) {
    GarbageCollector gc;
    GlobalState& global = GlobalState::getInstance();
    StringPool& pool = global.getStringPool();

    Table* weak = gc.create<Table>();
    Table* metatable = gc.create<Table>();
    Table* child = gc.create<Table>();
    gc.addRoot(weak);

    GCString* modeKey = global.getMetamethodName(TMS::TM_MODE);
    metatable->set(Value(modeKey), Value(pool.intern("v")));
    weak->setMetatable(metatable);
    weak->setArray(1, Value(child));

    // Scan the table while it is weak-valued, then make the same edge strong
    // before the atomic phase clears weak entries.
    (void)gc.step(nullptr, 0);
    (void)gc.step(nullptr, 0);
    ASSERT_TRUE(suite, weak->isBlack(), "weak table is scanned before mode mutation");
    ASSERT_TRUE(suite, child->isWhite(), "weak value remains white before atomic reconciliation");
    metatable->set(Value(modeKey), Value());
    ASSERT_TRUE(suite, metatable->get(Value(modeKey)).isNil(), "weak mode key is removed before atomic");

    (void)gc.step(nullptr, 0);
    ASSERT_TRUE(suite, (weak->getMarked() & GCBits::WEAKVALUE) == 0,
                "atomic reconciliation refreshes cached weak-mode bits");
    ASSERT_TRUE(suite, child->isBlack(), "atomic mode reconciliation marks the newly strong value");
    ASSERT_TRUE(suite, weak->getArray(1).isTable(), "atomic reconciliation leaves the strong entry intact");

    bool finished = false;
    for (i32 i = 0; i < 16 && !finished; ++i) {
        finished = gc.step(nullptr, 0);
    }

    ASSERT_TRUE(suite, finished, "incremental cycle completes after weak mode mutation");
    ASSERT_EQ(suite, static_cast<usize>(3), gc.getObjectCount(),
              "weak-to-strong transition preserves the newly strong value");
    ASSERT_TRUE(suite, weak->getArray(1).isTable() && weak->getArray(1).asTable() == child,
                "atomic mode reconciliation retains the table entry");

    gc.removeRoot(weak);
}

void testWeakToStrongUserdataIsNotFinalizedEarly(TestSuite& suite) {
    gFinalizerCalls = 0;
    gFinalizerPayload = 0;

    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    StringPool& pool = L->getGlobalState().getStringPool();
    (void)gc.step(L, 10000);

    Table* weak = gc.createRoot<Table>();
    Table* weakMetatable = gc.create<Table>();
    Userdata* userdata = Userdata::createFull(sizeof(i32));
    gc.registerObject(userdata);
    *userdata->getTypedData<i32>() = 4321;
    Table* userdataMetatable = gc.create<Table>();
    Function* finalizer = gc.create<Function>(gcRecordingFinalizer);

    GCString* modeKey = L->getGlobalState().getMetamethodName(TMS::TM_MODE);
    GCString* gcKey = L->getGlobalState().getMetamethodName(TMS::TM_GC);
    weakMetatable->set(Value(modeKey), Value(pool.intern("v")));
    weak->setMetatable(weakMetatable);
    weak->setArray(1, Value(userdata));
    userdataMetatable->set(Value(gcKey), Value(finalizer));
    userdata->setMetatable(userdataMetatable);

    // Change the mode after this table's weak value was skipped but before
    // the next Atomic step.  Runtime roots can add a few gray objects ahead
    // of the fixture, so drive propagation until this table is black.
    (void)gc.step(L, 0);
    for (i32 i = 0; i < 64 && !weak->isBlack(); ++i) {
        (void)gc.step(L, 0);
    }
    ASSERT_TRUE(suite, weak->isBlack(), "weak userdata table is scanned before mode mutation");
    ASSERT_TRUE(suite, userdata->isWhite(), "weak userdata remains white before atomic reconciliation");
    weakMetatable->set(Value(modeKey), Value());
    (void)gc.step(L, 10000);

    ASSERT_EQ(suite, 0, gFinalizerCalls, "newly strong userdata is not finalized in the current cycle");
    ASSERT_TRUE(suite, (userdata->getMarked() & GCBits::FINALIZED) == 0,
                "newly strong userdata is not marked finalized");
    ASSERT_TRUE(suite, weak->getArray(1).isUserdata() && weak->getArray(1).asUserdata() == userdata,
                "weak-to-strong transition retains userdata identity");

    weak->setArray(1, Value());
    (void)gc.step(L, 10000);
    ASSERT_EQ(suite, 1, gFinalizerCalls, "userdata finalizer runs after the strong reference is removed");
    ASSERT_EQ(suite, 4321, gFinalizerPayload, "deferred userdata finalizer receives the original payload");

    gc.removeRoot(weak);
    delete L;
}

void testCollectGarbageRunsUserdataFinalizer(TestSuite& suite) {
    gFinalizerCalls = 0;
    gFinalizerPayload = 0;

    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
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
}

void testFinalizerBudgetSlicesFullCollections(TestSuite& suite) {
    gFinalizerCalls = 0;

    auto stateOwner = std::unique_ptr<LuaState>(LuaState::newIsolatedState());
    LuaState* L = stateOwner.get();
    GlobalState& globalState = L->getGlobalState();
    GarbageCollector& gc = globalState.getGC();
    ExecutionPolicy::Limits limits;
    limits.finalizerBudgetPerDrain = 2;
    globalState.getExecutionPolicy().configure(limits);

    Table* metatable = gc.create<Table>();
    Function* finalizer = gc.create<Function>(gcRecordingFinalizer);
    metatable->set(Value(globalState.getMetamethodName(TMS::TM_GC)), Value(finalizer));
    for (i32 payload = 1; payload <= 5; ++payload) {
        Userdata* userdata = Userdata::createFull(sizeof(i32));
        *userdata->getTypedData<i32>() = payload;
        gc.registerObject(userdata);
        userdata->setMetatable(metatable);
    }

    (void)gc.collect(L);
    ASSERT_EQ(suite, 2, gFinalizerCalls, "one full collection enters at most two finalizer callbacks");
    ASSERT_EQ(suite, static_cast<usize>(3), gc.getPendingFinalizerCount(),
              "unspent finalizers remain rooted for a later collection");

    (void)gc.collect(L);
    ASSERT_EQ(suite, 4, gFinalizerCalls, "the next full collection receives a fresh finalizer slice");
    ASSERT_EQ(suite, static_cast<usize>(1), gc.getPendingFinalizerCount(),
              "only one finalizer remains after the second slice");

    (void)gc.collect(L);
    ASSERT_EQ(suite, 5, gFinalizerCalls, "the last slice drains the remaining finalizer exactly once");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.getPendingFinalizerCount(),
              "all deferred finalizers leave the rooted pending queue");

    (void)gc.collect(L);
    ASSERT_EQ(suite, 5, gFinalizerCalls, "later collections never repeat a budgeted finalizer");

    globalState.getExecutionPolicy().reset();
}

void testFinalizerBudgetSurvivesReentrantCollection(TestSuite& suite) {
    gReentrantFinalizerCalls = 0;

    auto stateOwner = std::unique_ptr<LuaState>(LuaState::newIsolatedState());
    LuaState* L = stateOwner.get();
    GlobalState& globalState = L->getGlobalState();
    GarbageCollector& gc = globalState.getGC();
    ExecutionPolicy::Limits limits;
    limits.finalizerBudgetPerDrain = 1;
    globalState.getExecutionPolicy().configure(limits);

    Table* metatable = gc.create<Table>();
    Function* finalizer = gc.create<Function>(gcReentrantFinalizer);
    metatable->set(Value(globalState.getMetamethodName(TMS::TM_GC)), Value(finalizer));
    for (i32 index = 0; index < 2; ++index) {
        Userdata* userdata = Userdata::createFull(0);
        gc.registerObject(userdata);
        userdata->setMetatable(metatable);
    }

    (void)gc.collect(L);
    ASSERT_EQ(suite, 1, gReentrantFinalizerCalls,
              "nested collection cannot bypass the outer finalizer callback budget");
    ASSERT_EQ(suite, static_cast<usize>(1), gc.getPendingFinalizerCount(),
              "reentrant collection keeps the deferred userdata rooted");

    (void)gc.collect(L);
    ASSERT_EQ(suite, 2, gReentrantFinalizerCalls, "a later drain executes the deferred reentrant finalizer");
    ASSERT_EQ(suite, static_cast<usize>(0), gc.getPendingFinalizerCount(),
              "reentrant finalizer slices eventually drain the queue");

    globalState.getExecutionPolicy().reset();
}

void testFinalizerCanReenterCollection(TestSuite& suite) {
    gReentrantFinalizerCalls = 0;

    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    Table* metatable = new Table();
    Function* finalizer = new Function(gcReentrantFinalizer);
    Userdata* first = Userdata::createFull(0);
    Userdata* second = Userdata::createFull(0);

    gc.registerObject(metatable);
    gc.registerObject(finalizer);
    gc.registerObject(first);
    gc.registerObject(second);
    metatable->set(Value(L->getGlobalState().getMetamethodName(TMS::TM_GC)), Value(finalizer));
    first->setMetatable(metatable);
    second->setMetatable(metatable);

    (void)gc.collect(L);

    ASSERT_EQ(suite, 2, gReentrantFinalizerCalls,
              "nested collection keeps every outer pending userdata alive until its finalizer runs");

    delete L;
}

void testFinalizerHelperSlotsDoNotRetainFinalizedUserdata(TestSuite& suite) {
    gFinalizerCalls = 0;
    gFinalizerPayload = 0;

    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    Table* metatable = gc.create<Table>();
    Function* finalizer = gc.create<Function>(gcRecordingFinalizer);
    Userdata* userdata = Userdata::createFull(sizeof(i32));
    *userdata->getTypedData<i32>() = 9753;
    gc.registerObject(userdata);
    metatable->set(Value(L->getGlobalState().getMetamethodName(TMS::TM_GC)), Value(finalizer));
    userdata->setMetatable(metatable);

    // Model a VM frame whose physical helper window is wider than the
    // logical API top. runFinalizers writes its function and userdata into
    // this interval, which the next collection scans conservatively.
    L->getStack().setTop(8);
    L->setAbsoluteTop(0);
    (void)gc.collect(L);
    const usize objectsAfterFinalization = gc.getObjectCount();
    const usize secondCycleCollected = gc.collect(L);

    ASSERT_EQ(suite, 1, gFinalizerCalls, "userdata finalizer runs once before reclamation");
    ASSERT_EQ(suite, 9753, gFinalizerPayload, "finalizer receives the original userdata payload");
    ASSERT_TRUE(suite, secondCycleCollected >= 3,
                "next collection reclaims finalized userdata, metatable, and finalizer closure");
    ASSERT_TRUE(suite, objectsAfterFinalization >= gc.getObjectCount() + 3,
                "cleared finalizer helper slots do not retain the finalized object graph");

    delete L;
}

void testAutomaticCollectionReportsOuterCountAcrossReentrantFinalizer(TestSuite& suite) {
    gReentrantFinalizerCalls = 0;

    LuaState* L = LuaState::newIsolatedState();
    GarbageCollector& gc = L->getGlobalState().getGC();
    Table* metatable = gc.create<Table>();
    Function* finalizer = gc.create<Function>(gcReentrantFinalizer);
    Userdata* userdata = Userdata::createFull(0);
    gc.registerObject(userdata);
    metatable->set(Value(L->getGlobalState().getMetamethodName(TMS::TM_GC)), Value(finalizer));
    userdata->setMetatable(metatable);
    (void)gc.create<Table>();

    // Start an incremental cycle explicitly, then let automatic checkpoints
    // finish it. The finalizer runs a nested full collection during Finalize.
    (void)gc.step(L, 0);
    usize reportedCollected = 0;
    for (i32 i = 0; i < 128 && reportedCollected == 0; ++i) {
        reportedCollected += gc.maybeCollectAutomatic(L);
    }

    ASSERT_EQ(suite, 1, gReentrantFinalizerCalls, "automatic incremental finalizer runs exactly once");
    ASSERT_EQ(suite, static_cast<usize>(1), reportedCollected,
              "automatic checkpoint preserves the outer sweep count across nested collection");

    delete L;
}

void testUpvalueOpen(TestSuite& suite) {
    LuaState* L = LuaState::newIsolatedState();
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
    LuaState* L = LuaState::newIsolatedState();
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
    LuaState* L = LuaState::newIsolatedState();
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
    registry.registerTest("GC", "Allocator Factory Registration Rollback",
                          testAllocatorFactoryCleansUpRegistrationFailure);
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
    registry.registerTest("GC", "Write Barrier Object References",
                          testWriteBarrierPreservesMetatableFunctionAndUpvalueRefs);
    registry.registerTest("GC", "Composite Marking", testGarbageCollectorMarksCompositeObjects);
    registry.registerTest("GC", "collectgarbage Collect", testCollectGarbageCollectReclaimsMemory);
    registry.registerTest("GC", "Fast Memory Accounting", testFastMemoryAccountingTracksDynamicObjects);
    registry.registerTest("GC", "Incremental Constructor And Proto Barriers",
                          testIncrementalBarriersPublishConstructedAndProtoGraphs);
    registry.registerTest("GC", "Incremental Sweep Registration Cursor",
                          testIncrementalSweepRegistrationPreservesCursor);
    registry.registerTest("GC", "Incremental Sweep Prefilled Weak Registration",
                          testIncrementalSweepRegistrationCleansPrefilledWeakTable);
    registry.registerTest("GC", "Automatic Cycle Completes Below Threshold",
                          testAutomaticCollectorFinishesCycleBelowStartThreshold);
    registry.registerTest("GC", "Incremental Sweep Upvalue Close",
                          testClosingUpvalueDuringIncrementalSweepRestartsMark);
    registry.registerTest("GC", "Explicit Collection Anonymous Temporaries",
                          testExplicitCollectionKeepsAnonymousTemporaries);
    registry.registerTest("GC", "Weak Table Values", testWeakTableValuesAreCleared);
    registry.registerTest("GC", "Weak Table Keys", testWeakTableKeysAreCleared);
    registry.registerTest("GC", "Incremental Weak Mode Mutation", testIncrementalWeakToStrongModeTransition);
    registry.registerTest("GC", "Weak Mode Mutation Defers Finalizer", testWeakToStrongUserdataIsNotFinalizedEarly);
    registry.registerTest("GC", "Userdata Finalizer", testCollectGarbageRunsUserdataFinalizer);
    registry.registerTest("GC", "Finalizer Budget Slices Full Collections", testFinalizerBudgetSlicesFullCollections);
    registry.registerTest("GC", "Finalizer Budget Survives Reentrant Collection",
                          testFinalizerBudgetSurvivesReentrantCollection);
    registry.registerTest("GC", "Reentrant Userdata Finalizers", testFinalizerCanReenterCollection);
    registry.registerTest("GC", "Finalizer Helper Slots Release Userdata",
                          testFinalizerHelperSlotsDoNotRetainFinalizedUserdata);
    registry.registerTest("GC", "Automatic Count Across Reentrant Finalizer",
                          testAutomaticCollectionReportsOuterCountAcrossReentrantFinalizer);
    registry.registerTest("GC", "Upvalue Open", testUpvalueOpen);
    registry.registerTest("GC", "Upvalue Closed", testUpvalueClosed);
    registry.registerTest("GC", "Upvalue Close All", testUpvalueCloseAll);
}
