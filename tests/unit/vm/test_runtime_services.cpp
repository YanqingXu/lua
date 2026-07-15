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
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_internal.hpp"
#include "vm/vm.hpp"

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
