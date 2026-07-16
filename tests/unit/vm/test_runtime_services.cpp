/**
 * @file test_runtime_services.cpp
 * @brief RuntimeServices compatibility and explicit-context entry tests.
 */

#include "../framework/test_framework.hpp"
#include "common/lua_error.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/thread.hpp"
#include "lib/lib_manager.hpp"
#include "lua.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_internal.hpp"
#include "vm/vm.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <expected>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Runtime Services";

Proto* compileChunk(RuntimeServices& services, const char* source, const char* sourceName) {
    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(services);
    return codegen.generate(chunk, sourceName);
}

Function* createFunction(RuntimeServices& services, LuaState* L, Proto* proto) {
    Function* func = new Function(proto);
    func->setEnv(L->getGlobalTable());
    services.gc.registerObject(func);
    return func;
}

i32 runProtectedChunk(RuntimeServices& services, LuaState* L, Proto* proto, i32 nresults = 0) {
    Function* func = createFunction(services, L, proto);
    L->pushFunction(func);
    return L->pcall(0, nresults, 0);
}

ExecutionPolicy::InstructionCount g_nestedBudgetBefore = ExecutionPolicy::UnlimitedInstructions;
ExecutionPolicy::InstructionCount g_nestedBudgetAfter = ExecutionPolicy::UnlimitedInstructions;
i32 g_nestedCallStatus = Lua::LUA_OK;

i32 callLuaFromPolicyProbe(LuaState* L) {
    ExecutionPolicy& policy = L->getGlobalState().getExecutionPolicy();
    g_nestedBudgetBefore = policy.remainingInstructions();

    const Value target = L->at(1);
    L->pushValue(target);
    g_nestedCallStatus = L->pcall(0, 0, 0);
    g_nestedBudgetAfter = policy.remainingInstructions();

    L->pushBoolean(g_nestedCallStatus == Lua::LUA_ERRRUN);
    return 1;
}

GarbageCollector& legacyGarbageCollectorForRuntimeServicesTest() {
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

void prepareLuaCallFrame(LuaState* L, Function* func) {
    Stack& stack = L->getStack();

    stack.push(Value(func));
    usize funcIndex = stack.size() - 1;

    CallInfo& ci = L->pushCallInfo();
    ci.func = funcIndex;
    ci.base = funcIndex + 1;
    ci.top = ci.base;
    ci.savedpc = nullptr;
    ci.nresults = -1;
    ci.tailcalls = 0;

    Proto* proto = func->getProto();
    usize requiredTop = ci.base + proto->getMaxStackSize();
    if (stack.capacity() < requiredTop) {
        stack.checkSpace(requiredTop - stack.size());
    }
    while (stack.size() < requiredTop) {
        stack.push(Value());
    }

    ci.top = requiredTop;
    L->setAbsoluteTop(requiredTop);
}

} // namespace

void testRuntimeServicesExposeSingletonCompatibilityLayer(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();

    ASSERT_TRUE(suite, &services.globalState == &GlobalState::getInstance(), "global state comes from singleton layer");
    ASSERT_TRUE(suite, &services.strings == &StringPool::getInstance(), "string pool comes from singleton layer");
    ASSERT_TRUE(suite, &services.gc == &GlobalState::getInstance().getGC(), "gc comes from global state ownership");
    ASSERT_TRUE(suite, &services.gc != &legacyGarbageCollectorForRuntimeServicesTest(), "legacy GC shim is distinct");
}

void testEngineContextOwnsIsolatedRuntimeServices(TestSuite& suite) {
    EngineContext first;
    EngineContext second;

    RuntimeServices firstServices = first.services();
    RuntimeServices secondServices = second.services();

    ASSERT_TRUE(suite, &first.globalState() != &GlobalState::getInstance(),
                "engine context global state is not the singleton");
    ASSERT_TRUE(suite, &first.globalState() != &second.globalState(), "engine contexts own distinct global states");
    ASSERT_TRUE(suite, &first.strings() != &second.strings(), "engine contexts own distinct string pools");
    ASSERT_TRUE(suite, &first.gc() != &second.gc(), "engine contexts own distinct collectors");
    ASSERT_TRUE(suite, &firstServices.globalState == &first.globalState(), "services expose context global state");
    ASSERT_TRUE(suite, &firstServices.strings == &first.strings(), "services expose context string pool");
    ASSERT_TRUE(suite, &firstServices.gc == &first.gc(), "services expose context collector");
    ASSERT_TRUE(suite, &secondServices.globalState == &second.globalState(), "second services expose second context");

    GCString* firstString = first.strings().intern("isolated");
    GCString* secondString = second.strings().intern("isolated");

    ASSERT_TRUE(suite, firstString != secondString, "same text interns to distinct objects in isolated contexts");
    ASSERT_TRUE(suite, firstString->getOwnerCollector() == &first.gc(),
                "first context string belongs to first collector");
    ASSERT_TRUE(suite, secondString->getOwnerCollector() == &second.gc(),
                "second context string belongs to second collector");
}

void testLuaStateNewStateAcceptsEngineContext(TestSuite& suite) {
    EngineContext context;
    LuaState* L = LuaState::newState(context);

    ASSERT_TRUE(suite, &L->getGlobalState() == &context.globalState(), "newState(context) uses context global state");
    ASSERT_TRUE(suite, context.globalState().getMainThread() == L,
                "newState(context) registers the main thread in the context");
    ASSERT_TRUE(suite, L->getGlobalTable() != nullptr, "newState(context) creates a global table");
    ASSERT_TRUE(suite, L->getGlobalTable()->getOwnerCollector() == &context.gc(),
                "newState(context) global table belongs to context collector");

    delete L;
    context.gc().clearAll(context.strings());
}

void testLuaStateCreateReturnsOwningState(TestSuite& suite) {
    using CreateResult = decltype(LuaState::create(std::declval<EngineContext&>()));
    static_assert(std::is_same_v<CreateResult, UPtr<LuaState>>,
                  "LuaState::create should make ownership explicit for modern C++ callers");

    EngineContext context;
    UPtr<LuaState> L = LuaState::create(context);

    ASSERT_TRUE(suite, L != nullptr, "create(context) returns an owning state");
    ASSERT_TRUE(suite, &L->getGlobalState() == &context.globalState(), "create(context) uses context global state");
    ASSERT_TRUE(suite, context.globalState().getMainThread() == L.get(),
                "create(context) registers the main thread in the context");

    L.reset();
    context.gc().clearAll(context.strings());
}

void testCompilerAcceptsRuntimeServices(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();

    Proto* proto = compileChunk(services, "local x = 1 + 2\nreturn x", "=(runtime_services_codegen)");

    ASSERT_TRUE(suite, proto != nullptr, "context-aware compiler returns proto");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "context-aware compiler emits instructions");
    ASSERT_TRUE(suite, proto->getSource() != nullptr, "context-aware compiler interns source name");
}

void testLuaStateAndVmAcceptRuntimeServices(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto* proto = compileChunk(services, "return 'run' .. 'time'", "=(runtime_services_vm)");

    LuaState* L = LuaState::newState(services);
    Function* func = createFunction(services, L, proto);

    VM::execute(services, L, func);

    ASSERT_TRUE(suite, L->top().isString(), "context-aware vm leaves string result");
    ASSERT_EQ(suite, std::string("runtime"), std::string(L->top().asString()->c_str()),
              "context-aware vm executes concat");

    delete L;
}

void testNestedFunctionsUseContextStringPool(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    Proto* proto = compileChunk(services, "limit = 7\nfunction nested() return limit <= 8 end\nreturn nested()",
                                "=(runtime_services_nested)");

    LuaState* L = LuaState::newState(context);
    Function* func = createFunction(services, L, proto);

    VM::execute(services, L, func);

    ASSERT_TRUE(suite, L->top().isBoolean(), "nested function should return a boolean");
    ASSERT_TRUE(suite, L->top().asBoolean(), "nested function sees globals through context string pool");

    delete L;
    context.gc().clearAll(context.strings());
}

void testCoroutineResumeUsesContextRuntimeServices(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    Proto* proto = compileChunk(services, "return 'co' .. 'ntext'", "=(runtime_services_coroutine)");

    LuaState* L = LuaState::newState(context);
    Function* function = createFunction(services, L, proto);
    Thread* thread = Thread::create(L, function);
    L->pushValue(Value(thread));
    const usize prefixTop = L->getAbsoluteTop();

    const bool resumed = thread->resume(L, 0);

    ASSERT_TRUE(suite, resumed, "isolated-context coroutine resumes successfully");
    ASSERT_TRUE(suite, thread->isDead(), "isolated-context coroutine reaches its return");
    ASSERT_EQ(suite, prefixTop + 2, L->getAbsoluteTop(),
              "coroutine resume preserves its thread prefix and publishes two results");
    ASSERT_TRUE(suite, L->top().isString(), "isolated-context coroutine returns a string");
    ASSERT_EQ(suite, std::string("context"), std::string(L->top().asString()->c_str()),
              "isolated-context coroutine executes string operations");
    ASSERT_TRUE(suite, L->top().asString()->getOwnerCollector() == &context.gc(),
                "coroutine result belongs to its context collector");

    delete L;
    context.gc().clearAll(context.strings());
}

void testExecutionPolicyStopsInfiniteLoopAtInstructionBudget(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    UPtr<LuaState> L = LuaState::create(context);
    Proto* proto = compileChunk(services, "while true do end", "=(execution_budget)");

    constexpr ExecutionPolicy::InstructionCount kBudget = 64;
    context.executionPolicy().configure({kBudget, ExecutionPolicy::Clock::time_point::max()});
    const i32 status = runProtectedChunk(services, L.get(), proto);

    ASSERT_EQ(suite, Lua::LUA_ERRRUN, status, "finite instruction budget stops an infinite loop");
    ASSERT_EQ(suite, static_cast<u64>(0), context.executionPolicy().remainingInstructions(),
              "instruction budget is exhausted exactly once");
    ASSERT_EQ(suite, kBudget, context.executionPolicy().consumedInstructions(),
              "configured budget permits exactly N instructions");
    ASSERT_TRUE(suite, L->top().isString(), "budget stop leaves a Lua string error object");
    ASSERT_TRUE(suite,
                L->top().asString() == context.globalState().getExecutionPolicyErrorMessage(
                                           ExecutionStopReason::InstructionBudgetExceeded),
                "protected call returns the fixed instruction-budget error object");

    context.executionPolicy().reset();
}

void testExecutionPolicyUsesMonotonicDeadline(TestSuite& suite) {
    using namespace std::chrono_literals;

    EngineContext context;
    RuntimeServices services = context.services();
    UPtr<LuaState> L = LuaState::create(context);
    Proto* proto = compileChunk(services, "while true do end", "=(execution_deadline)");

    ExecutionPolicy::Limits limits;
    limits.instructionBudget = 10'000'000;
    limits.deadline = ExecutionPolicy::Clock::now() + 2ms;
    context.executionPolicy().configure(limits);
    const i32 status = runProtectedChunk(services, L.get(), proto);

    ASSERT_EQ(suite, Lua::LUA_ERRRUN, status, "steady-clock deadline stops an infinite loop");
    ASSERT_TRUE(suite, context.executionPolicy().remainingInstructions() > 0,
                "deadline fires before the fallback instruction budget");
    ASSERT_TRUE(suite,
                L->top().isString() && L->top().asString() == context.globalState().getExecutionPolicyErrorMessage(
                                                                  ExecutionStopReason::DeadlineExceeded),
                "protected call returns the fixed deadline error object");

    context.executionPolicy().reset();
}

void testExecutionPolicyAllowsAtomicExternalCancellation(TestSuite& suite) {
    using namespace std::chrono_literals;

    EngineContext context;
    RuntimeServices services = context.services();
    UPtr<LuaState> L = LuaState::create(context);
    Proto* proto = compileChunk(services, "while true do end", "=(execution_cancel)");

    ExecutionPolicy::Limits limits;
    limits.instructionBudget = 50'000'000;
    context.executionPolicy().configure(limits);
    const ExecutionCancellationHandle cancellation = context.cancellationHandle();

    std::thread canceller([cancellation] {
        std::this_thread::sleep_for(2ms);
        cancellation.requestCancellation();
    });
    const i32 status = runProtectedChunk(services, L.get(), proto);
    canceller.join();

    ASSERT_EQ(suite, Lua::LUA_ERRRUN, status, "external atomic cancellation stops an infinite loop");
    ASSERT_TRUE(suite, context.executionPolicy().isCancellationRequested(),
                "owner observes the one-way cancellation request");
    ASSERT_TRUE(suite, context.executionPolicy().remainingInstructions() > 0,
                "cancellation fires before the fallback instruction budget");
    ASSERT_TRUE(suite,
                L->top().isString() && L->top().asString() == context.globalState().getExecutionPolicyErrorMessage(
                                                                  ExecutionStopReason::Cancelled),
                "protected call returns the fixed cancellation error object");

    context.executionPolicy().reset();
}

void testRuntimeOwnerThreadRejectsForeignStateAccess(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    GlobalState& global = context.globalState();
    StringPool& strings = context.strings();
    GarbageCollector& gc = context.gc();
    UPtr<LuaState> state = LuaState::create(context);
    lua_State* publicState = reinterpret_cast<lua_State*>(state.get());
    const ExecutionCancellationHandle cancellation = context.cancellationHandle();

    bool servicesRejected = false;
    bool servicesConstructionRejected = false;
    bool vmRejected = false;
    bool publicApiRejected = false;
    bool executionPollRejected = false;
    bool debugApiRejected = false;
    int checkStackResult = -1;
    int protectedCallResult = Lua::LUA_OK;
    lua_State* child = publicState;

    std::thread foreign([&] {
        try {
            (void)context.services();
        } catch (const RuntimeOwnerThreadError& error) {
            servicesRejected = std::string(error.what()) == "Lua runtime accessed from non-owner thread";
        }

        try {
            (void)RuntimeServices(global, strings, gc);
        } catch (const RuntimeOwnerThreadError& error) {
            servicesConstructionRejected = std::string(error.what()) == "Lua runtime accessed from non-owner thread";
        }

        try {
            (void)VM::executeProto(services, state.get(), nullptr, 1);
        } catch (const RuntimeOwnerThreadError& error) {
            vmRejected = std::string(error.what()) == "Lua runtime accessed from non-owner thread";
        }

        try {
            (void)lua_gettop(publicState);
        } catch (const RuntimeOwnerThreadError& error) {
            publicApiRejected = std::string(error.what()) == "Lua runtime accessed from non-owner thread";
        }

        try {
            lua_checkexecution(publicState);
        } catch (const RuntimeOwnerThreadError& error) {
            executionPollRejected = std::string(error.what()) == "Lua runtime accessed from non-owner thread";
        }

        try {
            (void)lua_gethook(publicState);
        } catch (const RuntimeOwnerThreadError& error) {
            debugApiRejected = std::string(error.what()) == "Lua runtime accessed from non-owner thread";
        }

        checkStackResult = lua_checkstack(publicState, 1);
        protectedCallResult = lua_pcall(publicState, 0, 0, 0);
        child = lua_trynewthread(publicState);
        cancellation.requestCancellation();
    });
    foreign.join();

    ASSERT_TRUE(suite, servicesRejected, "foreign thread cannot acquire mutable EngineContext services");
    ASSERT_TRUE(suite, servicesConstructionRejected, "foreign thread cannot construct a runtime service bundle");
    ASSERT_TRUE(suite, vmRejected, "foreign thread cannot enter the VM with pre-acquired services");
    ASSERT_TRUE(suite, publicApiRejected, "may-throw C API rejects foreign thread before reading the stack");
    ASSERT_TRUE(suite, executionPollRejected, "native execution poll remains an owner-thread C API");
    ASSERT_TRUE(suite, debugApiRejected, "debug API rejects foreign thread before reading debug state");
    ASSERT_EQ(suite, 0, checkStackResult, "noexcept stack API rejects foreign thread without mutation");
    ASSERT_EQ(suite, Lua::LUA_ERRRUN, protectedCallResult, "protected API returns runtime failure on foreign thread");
    ASSERT_TRUE(suite, child == nullptr, "transactional thread creation rejects a foreign owner");
    ASSERT_TRUE(suite, context.executionPolicy().isCancellationRequested(),
                "pre-acquired atomic cancellation remains the only foreign-thread control");
    ASSERT_EQ(suite, 0, lua_gettop(publicState), "foreign-thread rejections preserve the owner stack");
    lua_pushinteger(publicState, 7);
    ASSERT_EQ(suite, 1, lua_gettop(publicState), "runtime remains usable on its owner thread");
}

void testGameServerSandboxProfileControlsLibraryExposure(TestSuite& suite) {
    EngineContext restrictedContext;
    restrictedContext.sandboxPolicy().configure(SandboxProfile::gameServer());
    RuntimeServices restrictedServices = restrictedContext.services();
    UPtr<LuaState> restrictedState = LuaState::create(restrictedContext);
    StandardLibrary::openAll(restrictedState.get());

    const SandboxPolicy& policy = restrictedContext.sandboxPolicy();
    ASSERT_TRUE(suite, policy.allowsStandardLibrary("base"), "game-server profile exposes the base library");
    ASSERT_TRUE(suite, policy.allowsStandardLibrary("package"), "game-server profile exposes preload modules");
    ASSERT_TRUE(suite, !policy.allowsStandardLibrary("io"), "game-server profile hides the IO library");
    ASSERT_TRUE(suite, !policy.allowsStandardLibrary("os"), "game-server profile hides the OS library");
    ASSERT_TRUE(suite, !policy.allowsStandardLibrary("debug"), "game-server profile hides the debug library");
    ASSERT_TRUE(suite, !policy.allows(SandboxCapability::Filesystem), "game-server profile denies filesystem access");
    ASSERT_TRUE(suite, !policy.allows(SandboxCapability::Process), "game-server profile denies process access");
    ASSERT_TRUE(suite, !policy.allows(SandboxCapability::NativeModules), "game-server profile denies native modules");

    ASSERT_TRUE(suite, restrictedState->getGlobal("math").isTable(), "allowed math library is published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("string").isTable(), "allowed string library is published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("table").isTable(), "allowed table library is published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("coroutine").isTable(), "allowed coroutine library is published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("io").isNil(), "disabled IO library is not published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("os").isNil(), "disabled OS library is not published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("debug").isNil(), "disabled debug library is not published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("loadfile").isNil(), "filesystem base helper is not published");
    ASSERT_TRUE(suite, restrictedState->getGlobal("dofile").isNil(), "filesystem executor is not published");

    Value packageValue = restrictedState->getGlobal("package");
    ASSERT_TRUE(suite, packageValue.isTable(), "preload-only package table is published");
    Table* packageTable = packageValue.isTable() ? packageValue.asTable() : nullptr;
    auto packageField = [&](StrView name) {
        if (packageTable == nullptr) {
            return Value();
        }
        return packageTable->get(Value(restrictedContext.strings().intern(name)));
    };
    ASSERT_TRUE(suite, packageField("loadlib").isNil(), "native package.loadlib is not published");
    ASSERT_TRUE(suite, packageField("path").isString() && packageField("path").asString()->view().empty(),
                "filesystem module path is empty");
    ASSERT_TRUE(suite, packageField("cpath").isString() && packageField("cpath").asString()->view().empty(),
                "native module path is empty");

    Value loadersValue = packageField("loaders");
    Table* loaders = loadersValue.isTable() ? loadersValue.asTable() : nullptr;
    ASSERT_TRUE(suite, loaders != nullptr && loaders->get(Value(1.0)).isFunction(),
                "preload searcher remains available");
    ASSERT_TRUE(suite, loaders != nullptr && loaders->get(Value(2.0)).isNil(),
                "filesystem and native searchers are absent");

    Proto* preload = compileChunk(restrictedServices, R"(
        package.preload.sandbox_fixture = function()
            return { value = 42 }
        end
        sandbox_preload_ok = require("sandbox_fixture").value == 42
    )",
                                  "=(sandbox_preload)");
    ASSERT_EQ(suite, Lua::LUA_OK, runProtectedChunk(restrictedServices, restrictedState.get(), preload),
              "preload-only modules execute without privileged capabilities");
    ASSERT_TRUE(suite, restrictedState->getGlobal("sandbox_preload_ok").isTrue(), "preloaded module result is visible");

    const i32 stackBeforeDeniedOpen = restrictedState->getTop();
    bool directOpenDenied = false;
    try {
        (void)luaopen_io(reinterpret_cast<lua_State*>(restrictedState.get()));
    } catch (const RuntimeError& error) {
        directOpenDenied = std::string(error.what()) == SandboxPolicy::libraryDeniedMessage();
    }
    ASSERT_TRUE(suite, directOpenDenied, "direct public opener cannot bypass disabled library exposure");
    ASSERT_EQ(suite, stackBeforeDeniedOpen, restrictedState->getTop(),
              "denied public opener leaves the stack unchanged");
    ASSERT_TRUE(suite, restrictedState->getGlobal("io").isNil(), "denied public opener does not publish IO state");

    EngineContext baseOnlyContext;
    SandboxProfile baseOnlyProfile = SandboxProfile::unrestricted();
    baseOnlyProfile.standardLibraries = StandardLibrarySet::Base;
    baseOnlyContext.sandboxPolicy().configure(baseOnlyProfile);
    UPtr<LuaState> baseOnlyState = LuaState::create(baseOnlyContext);
    const i32 baseOnlyTop = baseOnlyState->getTop();
    bool combinedBaseOpenDenied = false;
    try {
        (void)luaopen_base(reinterpret_cast<lua_State*>(baseOnlyState.get()));
    } catch (const RuntimeError& error) {
        combinedBaseOpenDenied = std::string(error.what()) == SandboxPolicy::libraryDeniedMessage();
    }
    ASSERT_TRUE(suite, combinedBaseOpenDenied, "luaopen_base preflights the paired coroutine library");
    ASSERT_EQ(suite, baseOnlyTop, baseOnlyState->getTop(), "failed paired base open preserves the stack");
    ASSERT_TRUE(suite, baseOnlyState->getGlobal("print").isNil(), "failed paired base open publishes no base globals");
    ASSERT_TRUE(suite, baseOnlyState->getGlobal("coroutine").isNil(),
                "failed paired base open publishes no coroutine table");

    EngineContext inspectionContext;
    SandboxProfile inspectionProfile = SandboxProfile::unrestricted();
    inspectionProfile.filesystem = false;
    inspectionProfile.process = false;
    inspectionProfile.nativeModules = false;
    inspectionContext.sandboxPolicy().configure(inspectionProfile);
    UPtr<LuaState> inspectionState = LuaState::create(inspectionContext);
    StandardLibrary::openAll(inspectionState.get());
    auto inspectionField = [&](Table* table, StrView name) {
        return table != nullptr ? table->get(Value(inspectionContext.strings().intern(name))) : Value();
    };

    ASSERT_TRUE(suite, inspectionState->getGlobal("loadfile").isNil(),
                "base filesystem helpers are absent when only the capability is denied");

    Value ioValue = inspectionState->getGlobal("io");
    Table* ioTable = ioValue.isTable() ? ioValue.asTable() : nullptr;
    ASSERT_TRUE(suite, ioTable != nullptr && inspectionField(ioTable, "close").isFunction(),
                "IO cleanup remains available without filesystem capability");
    ASSERT_TRUE(suite, ioTable != nullptr && inspectionField(ioTable, "open").isNil(),
                "IO filesystem functions are absent without filesystem capability");
    ASSERT_TRUE(suite, ioTable != nullptr && inspectionField(ioTable, "popen").isNil(),
                "IO process functions are absent without process capability");

    Value osValue = inspectionState->getGlobal("os");
    Table* osTable = osValue.isTable() ? osValue.asTable() : nullptr;
    ASSERT_TRUE(suite, osTable != nullptr && inspectionField(osTable, "time").isFunction(),
                "safe OS time helpers remain available");
    ASSERT_TRUE(suite, osTable != nullptr && inspectionField(osTable, "execute").isNil(),
                "OS process functions are absent without process capability");
    ASSERT_TRUE(suite, osTable != nullptr && inspectionField(osTable, "remove").isNil(),
                "OS filesystem functions are absent without filesystem capability");

    Value debugValue = inspectionState->getGlobal("debug");
    Table* debugTable = debugValue.isTable() ? debugValue.asTable() : nullptr;
    ASSERT_TRUE(suite, debugTable != nullptr, "debug inspection helpers can be exposed independently");
    ASSERT_TRUE(suite, debugTable != nullptr && inspectionField(debugTable, "traceback").isFunction(),
                "non-interactive debug helpers remain available");
    ASSERT_TRUE(suite, debugTable != nullptr && inspectionField(debugTable, "debug").isNil(),
                "interactive debug console is absent without process capability");

    EngineContext unrestrictedContext;
    UPtr<LuaState> unrestrictedState = LuaState::create(unrestrictedContext);
    StandardLibrary::openAll(unrestrictedState.get());

    ASSERT_TRUE(suite, unrestrictedContext.sandboxPolicy().allows(SandboxCapability::Filesystem),
                "another context retains unrestricted defaults");
    ASSERT_TRUE(suite, unrestrictedState->getGlobal("io").isTable(), "unrestricted context still publishes IO");
    ASSERT_TRUE(suite, unrestrictedState->getGlobal("os").isTable(), "unrestricted context still publishes OS");
    ASSERT_TRUE(suite, unrestrictedState->getGlobal("debug").isTable(), "unrestricted context still publishes debug");
    ASSERT_TRUE(suite, unrestrictedState->getGlobal("loadfile").isFunction(),
                "unrestricted context retains filesystem helpers");

    Value unrestrictedPackage = unrestrictedState->getGlobal("package");
    Table* unrestrictedPackageTable = unrestrictedPackage.isTable() ? unrestrictedPackage.asTable() : nullptr;
    Value unrestrictedLoaders =
        unrestrictedPackageTable != nullptr
            ? unrestrictedPackageTable->get(Value(unrestrictedContext.strings().intern("loaders")))
            : Value();
    Table* unrestrictedLoaderTable = unrestrictedLoaders.isTable() ? unrestrictedLoaders.asTable() : nullptr;
    ASSERT_TRUE(suite, unrestrictedLoaderTable != nullptr && unrestrictedLoaderTable->get(Value(4.0)).isFunction(),
                "unrestricted context retains all four package searchers");
}

void testSandboxCapabilitiesRejectCapturedPrivilegedFunctions(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    StandardLibrary::openAll(state.get());

    Proto* capture = compileChunk(services, R"(
        sandbox_loadfile = loadfile
        sandbox_dofile = dofile
        sandbox_io_open = io.open
        sandbox_io_tmpfile = io.tmpfile
        sandbox_io_popen = io.popen
        sandbox_os_execute = os.execute
        sandbox_os_getenv = os.getenv
        sandbox_os_remove = os.remove
        sandbox_os_rename = os.rename
        sandbox_os_setlocale = os.setlocale
        sandbox_os_tmpname = os.tmpname
        sandbox_package_loadlib = package.loadlib
        sandbox_file = assert(io.tmpfile())
        sandbox_file:write("x")
        sandbox_file:seek("set", 0)
        sandbox_file_read = sandbox_file.read
        package.preload.sandbox_after_restrict = function()
            return { ok = true }
        end
    )",
                                  "=(sandbox_capture)");
    ASSERT_EQ(suite, Lua::LUA_OK, runProtectedChunk(services, state.get(), capture),
              "unrestricted setup captures privileged functions");

    SandboxProfile restricted = SandboxProfile::unrestricted();
    restricted.filesystem = false;
    restricted.process = false;
    restricted.nativeModules = false;
    context.sandboxPolicy().configure(restricted);

    Proto* probe = compileChunk(services, R"(
        local function denied(expected, fn, ...)
            local ok, message = pcall(fn, ...)
            return not ok and message == expected
        end

        local filesystem = "sandbox: filesystem access denied"
        local process = "sandbox: process access denied"
        local native = "sandbox: native module access denied"

        sandbox_denied_loadfile = denied(filesystem, sandbox_loadfile, "sandbox_missing.lua")
        sandbox_denied_dofile = denied(filesystem, sandbox_dofile, "sandbox_missing.lua")
        sandbox_denied_io_open = denied(filesystem, sandbox_io_open, "sandbox_missing.txt", "r")
        sandbox_denied_io_tmpfile = denied(filesystem, sandbox_io_tmpfile)
        sandbox_denied_file_read = denied(filesystem, sandbox_file_read, sandbox_file, 1)
        sandbox_denied_os_remove = denied(filesystem, sandbox_os_remove, "sandbox_missing.txt")
        sandbox_denied_os_rename = denied(filesystem, sandbox_os_rename,
                                           "sandbox_missing.txt", "sandbox_renamed.txt")
        sandbox_denied_os_tmpname = denied(filesystem, sandbox_os_tmpname)
        sandbox_denied_io_popen = denied(process, sandbox_io_popen)
        sandbox_denied_os_execute = denied(process, sandbox_os_execute)
        sandbox_denied_os_getenv = denied(process, sandbox_os_getenv, "PATH")
        sandbox_denied_os_setlocale = denied(process, sandbox_os_setlocale, nil)
        sandbox_denied_loadlib = denied(native, sandbox_package_loadlib, "sandbox_missing", "*")

        sandbox_preload_after_restrict = require("sandbox_after_restrict").ok
        local require_ok, require_error = pcall(require, "sandbox_missing_module")
        sandbox_require_file_denied = not require_ok and require_error == filesystem
        sandbox_file:close()
    )",
                                "=(sandbox_probe)");
    ASSERT_EQ(suite, Lua::LUA_OK, runProtectedChunk(services, state.get(), probe),
              "captured privileged functions fail inside protected Lua calls");

    static constexpr std::array<StrView, 15> expectedTrue = {
        "sandbox_denied_loadfile",   "sandbox_denied_dofile",          "sandbox_denied_io_open",
        "sandbox_denied_io_tmpfile", "sandbox_denied_file_read",       "sandbox_denied_os_remove",
        "sandbox_denied_os_rename",  "sandbox_denied_os_tmpname",      "sandbox_denied_io_popen",
        "sandbox_denied_os_execute", "sandbox_denied_os_getenv",       "sandbox_denied_os_setlocale",
        "sandbox_denied_loadlib",    "sandbox_preload_after_restrict", "sandbox_require_file_denied",
    };
    for (StrView name : expectedTrue) {
        ASSERT_TRUE(suite, state->getGlobal(Str(name)).isTrue(), Str(name) + " is enforced by the active sandbox");
    }

    auto nativeLoad = context.nativeModules().load("sandbox_missing_native_module");
    ASSERT_TRUE(suite, !nativeLoad.has_value(), "context registry rejects native loading before OS access");
    ASSERT_TRUE(suite,
                !nativeLoad.has_value() &&
                    nativeLoad.error() == SandboxPolicy::deniedMessage(SandboxCapability::NativeModules),
                "native registry reports the stable sandbox denial");
    ASSERT_EQ(suite, static_cast<usize>(0), context.nativeModules().loadedCount(),
              "denied native load acquires no module lease");
}

void testExecutionPolicySurvivesCToLuaReentry(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    UPtr<LuaState> L = LuaState::create(context);

    Function* probe = services.gc.create<Function>(callLuaFromPolicyProbe);
    L->setGlobal("policy_reentry", Value(probe));
    Proto* proto =
        compileChunk(services, "return policy_reentry(function() while true do end end)", "=(execution_reentry)");

    g_nestedBudgetBefore = ExecutionPolicy::UnlimitedInstructions;
    g_nestedBudgetAfter = ExecutionPolicy::UnlimitedInstructions;
    g_nestedCallStatus = Lua::LUA_OK;
    constexpr ExecutionPolicy::InstructionCount kBudget = 48;
    context.executionPolicy().configure({kBudget, ExecutionPolicy::Clock::time_point::max()});
    const i32 status = runProtectedChunk(services, L.get(), proto);

    ASSERT_EQ(suite, Lua::LUA_ERRRUN, g_nestedCallStatus, "nested protected Lua call observes the shared budget stop");
    ASSERT_TRUE(suite, g_nestedBudgetBefore < kBudget && g_nestedBudgetBefore > 0,
                "outer Lua execution consumes budget before entering C");
    ASSERT_EQ(suite, static_cast<u64>(0), g_nestedBudgetAfter,
              "C-to-Lua re-entry consumes the outer execution budget without reset");
    ASSERT_EQ(suite, Lua::LUA_ERRRUN, status, "outer protected boundary reports the exhausted shared budget");
    ASSERT_TRUE(suite,
                L->top().isString() && L->top().asString() == context.globalState().getExecutionPolicyErrorMessage(
                                                                  ExecutionStopReason::InstructionBudgetExceeded),
                "nested exhaustion preserves the fixed budget error at the outer boundary");

    context.executionPolicy().reset();
}

void testExecutionPolicyPersistsAcrossCoroutineYieldAndResume(TestSuite& suite) {
    EngineContext context;
    RuntimeServices services = context.services();
    UPtr<LuaState> L = LuaState::create(context);
    StandardLibrary::openAll(L.get());

    Proto* setup = compileChunk(services, R"(
        policy_coroutine = coroutine.create(function()
            local value = 1
            coroutine.yield(value)
            while true do value = value + 1 end
        end)
    )",
                                "=(execution_coroutine_setup)");
    ASSERT_EQ(suite, Lua::LUA_OK, runProtectedChunk(services, L.get(), setup),
              "coroutine policy fixture initializes without limits");

    const Value coroutineValue = L->getGlobal("policy_coroutine");
    ASSERT_TRUE(suite, coroutineValue.isThread(), "policy fixture publishes a coroutine");
    Thread* coroutine = coroutineValue.isThread() ? coroutineValue.asThread() : nullptr;

    constexpr ExecutionPolicy::InstructionCount kBudget = 256;
    context.executionPolicy().configure({kBudget, ExecutionPolicy::Clock::time_point::max()});
    const usize callerBase = L->getAbsoluteTop();
    const bool firstResume = coroutine != nullptr && coroutine->resume(L.get(), 0);
    const ExecutionPolicy::InstructionCount afterYield = context.executionPolicy().remainingInstructions();

    ASSERT_TRUE(suite, firstResume, "first coroutine resume reaches yield");
    ASSERT_TRUE(suite, afterYield > 0 && afterYield < kBudget, "instructions before yield consume the shared budget");

    L->setAbsoluteTop(callerBase);
    const bool secondResume = coroutine != nullptr && coroutine->resume(L.get(), 0);

    ASSERT_TRUE(suite, !secondResume, "resumed infinite coroutine stops when inherited budget is exhausted");
    ASSERT_EQ(suite, static_cast<u64>(0), context.executionPolicy().remainingInstructions(),
              "resume continues the pre-yield budget instead of resetting it");
    ASSERT_TRUE(suite,
                L->top().isString() && L->top().asString() == context.globalState().getExecutionPolicyErrorMessage(
                                                                  ExecutionStopReason::InstructionBudgetExceeded),
                "coroutine resume publishes the fixed shared-budget error object");

    context.executionPolicy().reset();
}

void testVmTryExecuteProtoReturnsExpectedType(TestSuite& suite) {
    using TryResult = decltype(VM::tryExecuteProto(std::declval<RuntimeServices&>(), std::declval<LuaState*>(),
                                                   std::declval<Proto*>(), 1));
    bool hasExpectedSignature = std::is_same_v<TryResult, std::expected<ExecResult, RuntimeError>>;
    ASSERT_TRUE(suite, hasExpectedSignature, "tryExecuteProto returns expected exec result or runtime error");
}

void testVmTryExecuteProtoReturnsExecResultOnSuccess(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto* proto = compileChunk(services, "return 'vm' .. 'expected'", "=(vm_expected_success)");

    LuaState* L = LuaState::newState(services);
    Function* func = createFunction(services, L, proto);
    prepareLuaCallFrame(L, func);

    auto executed = VM::tryExecuteProto(services, L, proto, 1);

    ASSERT_TRUE(suite, executed.has_value(), "valid proto execution should return an ExecResult");
    if (executed) {
        ASSERT_EQ(suite, static_cast<int>(ExecResult::Returned), static_cast<int>(*executed),
                  "valid proto execution should return Returned");
    }

    L->popCallInfo();

    ASSERT_TRUE(suite, L->top().isString(), "tryExecuteProto leaves string result");
    ASSERT_EQ(suite, std::string("vmexpected"), std::string(L->top().asString()->c_str()),
              "tryExecuteProto executes concat");

    delete L;
}

void testVmTryExecuteProtoReturnsRuntimeErrorOnFailure(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    auto executed = VM::tryExecuteProto(services, L, nullptr, 1);

    ASSERT_TRUE(suite, !executed.has_value(), "null proto should return a RuntimeError");
    if (!executed) {
        std::string message = executed.error().what();
        ASSERT_TRUE(suite, message.find("null proto") != std::string::npos,
                    "RuntimeError should preserve null proto message");
    }

    delete L;
}

void testRuntimeErrorCaptureHelperMapsExpectedBoundary(TestSuite& suite) {
    auto successful = VM::detail::captureRuntimeErrors<i32>([] { return 42; });
    ASSERT_TRUE(suite, successful.has_value(), "runtime capture should return successful values");
    if (successful) {
        ASSERT_EQ(suite, 42, *successful, "runtime capture should preserve successful values");
    }

    auto runtimeFailure =
        VM::detail::captureRuntimeErrors<i32>([]() -> i32 { throw RuntimeError("runtime boundary"); });
    ASSERT_TRUE(suite, !runtimeFailure.has_value(), "runtime capture should map RuntimeError to unexpected");
    if (!runtimeFailure) {
        ASSERT_TRUE(suite, std::string(runtimeFailure.error().what()).find("runtime boundary") != std::string::npos,
                    "runtime capture should preserve RuntimeError messages");
    }

    auto luaFailure = VM::detail::captureRuntimeErrors<i32>([]() -> i32 { throw LuaError("lua boundary"); });
    ASSERT_TRUE(suite, !luaFailure.has_value(), "runtime capture should map LuaError to RuntimeError");
    if (!luaFailure) {
        ASSERT_TRUE(suite, std::string(luaFailure.error().what()).find("lua boundary") != std::string::npos,
                    "runtime capture should preserve LuaError messages");
    }

    auto stdFailure =
        VM::detail::captureRuntimeErrors<i32>([]() -> i32 { throw std::logic_error("standard boundary"); });
    ASSERT_TRUE(suite, !stdFailure.has_value(), "runtime capture should map std::exception to RuntimeError");
    if (!stdFailure) {
        ASSERT_TRUE(suite, std::string(stdFailure.error().what()).find("standard boundary") != std::string::npos,
                    "runtime capture should preserve std::exception messages");
    }

    bool rethrewBadAlloc = false;
    try {
        [[maybe_unused]] auto result = VM::detail::captureRuntimeErrors<i32>([]() -> i32 { throw std::bad_alloc(); });
    } catch (const std::bad_alloc&) {
        rethrewBadAlloc = true;
    }
    ASSERT_TRUE(suite, rethrewBadAlloc, "runtime capture should rethrow bad_alloc");
}

void testVmExecuteProtoKeepsThrowingForCompatibility(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    bool threwRuntimeError = false;
    try {
        (void)VM::executeProto(services, L, nullptr, 1);
    } catch (const RuntimeError& error) {
        threwRuntimeError = std::string(error.what()).find("null proto") != std::string::npos;
    }

    ASSERT_TRUE(suite, threwRuntimeError, "legacy executeProto should still throw on runtime failure");

    delete L;
}

void registerRuntimeServicesTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Expose Singleton Compatibility Layer",
                          testRuntimeServicesExposeSingletonCompatibilityLayer);
    registry.registerTest(kSuiteName, "EngineContext owns isolated runtime services",
                          testEngineContextOwnsIsolatedRuntimeServices);
    registry.registerTest(kSuiteName, "LuaState newState accepts EngineContext",
                          testLuaStateNewStateAcceptsEngineContext);
    registry.registerTest(kSuiteName, "LuaState create returns owning state", testLuaStateCreateReturnsOwningState);
    registry.registerTest(kSuiteName, "Compiler Accepts Runtime Services", testCompilerAcceptsRuntimeServices);
    registry.registerTest(kSuiteName, "LuaState And VM Accept Runtime Services",
                          testLuaStateAndVmAcceptRuntimeServices);
    registry.registerTest(kSuiteName, "Nested functions use context string pool",
                          testNestedFunctionsUseContextStringPool);
    registry.registerTest(kSuiteName, "Coroutine resume uses context runtime services",
                          testCoroutineResumeUsesContextRuntimeServices);
    registry.registerTest(kSuiteName, "Execution policy stops infinite loop at instruction budget",
                          testExecutionPolicyStopsInfiniteLoopAtInstructionBudget);
    registry.registerTest(kSuiteName, "Execution policy uses monotonic deadline",
                          testExecutionPolicyUsesMonotonicDeadline);
    registry.registerTest(kSuiteName, "Execution policy allows atomic external cancellation",
                          testExecutionPolicyAllowsAtomicExternalCancellation);
    registry.registerTest(kSuiteName, "Runtime owner thread rejects foreign state access",
                          testRuntimeOwnerThreadRejectsForeignStateAccess);
    registry.registerTest(kSuiteName, "Game-server sandbox profile controls library exposure",
                          testGameServerSandboxProfileControlsLibraryExposure);
    registry.registerTest(kSuiteName, "Sandbox capabilities reject captured privileged functions",
                          testSandboxCapabilitiesRejectCapturedPrivilegedFunctions);
    registry.registerTest(kSuiteName, "Execution policy survives C to Lua reentry",
                          testExecutionPolicySurvivesCToLuaReentry);
    registry.registerTest(kSuiteName, "Execution policy persists across coroutine yield and resume",
                          testExecutionPolicyPersistsAcrossCoroutineYieldAndResume);
    registry.registerTest(kSuiteName, "tryExecuteProto returns expected type",
                          testVmTryExecuteProtoReturnsExpectedType);
    registry.registerTest(kSuiteName, "tryExecuteProto returns exec result on success",
                          testVmTryExecuteProtoReturnsExecResultOnSuccess);
    registry.registerTest(kSuiteName, "tryExecuteProto returns RuntimeError on failure",
                          testVmTryExecuteProtoReturnsRuntimeErrorOnFailure);
    registry.registerTest(kSuiteName, "Runtime error capture helper maps expected boundary",
                          testRuntimeErrorCaptureHelperMapsExpectedBoundary);
    registry.registerTest(kSuiteName, "executeProto keeps throwing for compatibility",
                          testVmExecuteProtoKeepsThrowingForCompatibility);
}
