/**
 * @file test_debug_cli.cpp
 * @brief Scripted internal CLI end-to-end debugger test.
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "debugger/debug_cli.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Internal CLI";

Str readFixture(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

} // namespace

void testDebugCliScriptedEndToEnd(TestSuite& suite) {
    const std::filesystem::path fixture = "tests/lua/debugger/basic_breakpoint.lua";
    const Str source = readFixture(fixture);
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    IDebugRuntime& runtime = controller;
    DebugCli cli(runtime);
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);

    Parser parser(source, services);
    auto parsed = parser.parse();
    CodeGenerator codegen(services);
    Proto* proto = codegen.generate(*parsed, "@tests/lua/debugger/basic_breakpoint.lua");
    Function* function = new Function(proto);
    function->setEnv(state->getGlobalTable());
    services.gc.registerObject(function);

    auto attached = runtime.attachSession();
    DebugSession session = std::move(*attached);
    const auto breakpoint = cli.execute("break tests/lua/debugger/basic_breakpoint.lua:4");
    const auto run = cli.execute("run");
    std::atomic<bool> executionDone = false;
    bool timedOut = false;
    DebugResult<Str> backtrace = std::unexpected(DebugError{});
    DebugResult<Str> locals = std::unexpected(DebugError{});
    DebugResult<Str> continued = std::unexpected(DebugError{});

    std::thread control([&]() {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!executionDone.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
            if (runtime.snapshot().state == DebugSessionState::Suspended) {
                backtrace = cli.execute("backtrace");
                locals = cli.execute("locals");
                continued = cli.execute("continue");
                return;
            }
            std::this_thread::yield();
        }
        timedOut = true;
        (void)runtime.terminateExecution();
    });

    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    executionDone.store(true, std::memory_order_release);
    control.join();

    ASSERT_TRUE(suite, breakpoint && *breakpoint == "breakpoint pending line 4",
                "CLI break command installs a pending source breakpoint");
    ASSERT_TRUE(suite, run && *run == "running", "CLI run command starts the protocol-independent runtime");
    ASSERT_FALSE(suite, timedOut, "Scripted CLI session reaches the breakpoint within timeout");
    ASSERT_TRUE(suite, backtrace && backtrace->find("line=5") != Str::npos,
                "CLI backtrace reports the resolved executable line");
    ASSERT_TRUE(suite, locals && locals->find("greeting = \"hello\" : string") != Str::npos &&
                           locals->find("count = 2 : number") != Str::npos,
                "CLI locals command renders stable local names and values");
    ASSERT_TRUE(suite, continued && *continued == "continued", "CLI continue resumes the suspended VM");
    ASSERT_TRUE(suite, executed && state->top().isString() && state->top().asString()->view() == "hello 2",
                "CLI-driven end-to-end run preserves program behavior");
    ASSERT_TRUE(suite, !cli.execute("unknown"), "Unknown CLI command returns a structured error");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void registerDebuggerCliTests() {
    TestRegistry::getInstance().registerTest(kSuiteName, "Scripted End To End", testDebugCliScriptedEndToEnd);
}
