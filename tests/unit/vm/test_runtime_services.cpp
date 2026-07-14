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
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_internal.hpp"
#include "vm/vm.hpp"

#include <expected>
#include <new>
#include <stdexcept>
#include <string>
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
