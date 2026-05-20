/**
 * @file test_runtime_services.cpp
 * @brief RuntimeServices compatibility and explicit-context entry tests.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen.hpp"
#include "compiler/parser.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"

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

}  // namespace

void testRuntimeServicesExposeSingletonCompatibilityLayer(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();

    ASSERT_TRUE(suite, &services.globalState == &GlobalState::getInstance(), "global state comes from singleton layer");
    ASSERT_TRUE(suite, &services.strings == &StringPool::getInstance(), "string pool comes from singleton layer");
    ASSERT_TRUE(suite, &services.gc == &GarbageCollector::getInstance(), "gc comes from singleton layer");
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
    Function* func = new Function(proto);
    func->setEnv(L->getGlobalTable());
    services.gc.registerObject(func);

    VM::execute(services, L, func);

    ASSERT_TRUE(suite, L->top().isString(), "context-aware vm leaves string result");
    ASSERT_EQ(suite, std::string("runtime"), std::string(L->top().asString()->c_str()), "context-aware vm executes concat");

    delete L;
}

void registerRuntimeServicesTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Expose Singleton Compatibility Layer",
                          testRuntimeServicesExposeSingletonCompatibilityLayer);
    registry.registerTest(kSuiteName, "Compiler Accepts Runtime Services", testCompilerAcceptsRuntimeServices);
    registry.registerTest(kSuiteName, "LuaState And VM Accept Runtime Services", testLuaStateAndVmAcceptRuntimeServices);
}
