/**
 * @file test_runtime_services.cpp
 * @brief RuntimeServices compatibility and explicit-context entry tests.
 */

#include "../framework/test_framework.hpp"
#include "common/lua_error.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
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

}  // namespace

void testRuntimeServicesExposeSingletonCompatibilityLayer(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();

    ASSERT_TRUE(suite, &services.globalState == &GlobalState::getInstance(), "global state comes from singleton layer");
    ASSERT_TRUE(suite, &services.strings == &StringPool::getInstance(), "string pool comes from singleton layer");
    ASSERT_TRUE(suite, &services.gc == &GlobalState::getInstance().getGC(), "gc comes from global state ownership");
    ASSERT_TRUE(suite, &services.gc != &GarbageCollector::getInstance(), "legacy GC shim is distinct");
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
    ASSERT_EQ(suite, std::string("runtime"), std::string(L->top().asString()->c_str()), "context-aware vm executes concat");

    delete L;
}

void testVmTryExecuteProtoReturnsExpectedType(TestSuite& suite) {
    using TryResult = decltype(VM::tryExecuteProto(
        std::declval<RuntimeServices&>(),
        std::declval<LuaState*>(),
        std::declval<Proto*>(),
        1));
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
    auto successful = VM::detail::captureRuntimeErrors<i32>([] {
        return 42;
    });
    ASSERT_TRUE(suite, successful.has_value(), "runtime capture should return successful values");
    if (successful) {
        ASSERT_EQ(suite, 42, *successful, "runtime capture should preserve successful values");
    }

    auto runtimeFailure = VM::detail::captureRuntimeErrors<i32>([]() -> i32 {
        throw RuntimeError("runtime boundary");
    });
    ASSERT_TRUE(suite, !runtimeFailure.has_value(), "runtime capture should map RuntimeError to unexpected");
    if (!runtimeFailure) {
        ASSERT_TRUE(suite, std::string(runtimeFailure.error().what()).find("runtime boundary") != std::string::npos,
                    "runtime capture should preserve RuntimeError messages");
    }

    auto luaFailure = VM::detail::captureRuntimeErrors<i32>([]() -> i32 {
        throw LuaError("lua boundary");
    });
    ASSERT_TRUE(suite, !luaFailure.has_value(), "runtime capture should map LuaError to RuntimeError");
    if (!luaFailure) {
        ASSERT_TRUE(suite, std::string(luaFailure.error().what()).find("lua boundary") != std::string::npos,
                    "runtime capture should preserve LuaError messages");
    }

    auto stdFailure = VM::detail::captureRuntimeErrors<i32>([]() -> i32 {
        throw std::logic_error("standard boundary");
    });
    ASSERT_TRUE(suite, !stdFailure.has_value(), "runtime capture should map std::exception to RuntimeError");
    if (!stdFailure) {
        ASSERT_TRUE(suite, std::string(stdFailure.error().what()).find("standard boundary") != std::string::npos,
                    "runtime capture should preserve std::exception messages");
    }

    bool rethrewBadAlloc = false;
    try {
        (void)VM::detail::captureRuntimeErrors<i32>([]() -> i32 {
            throw std::bad_alloc();
        });
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
    registry.registerTest(kSuiteName, "Compiler Accepts Runtime Services", testCompilerAcceptsRuntimeServices);
    registry.registerTest(kSuiteName, "LuaState And VM Accept Runtime Services", testLuaStateAndVmAcceptRuntimeServices);
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
