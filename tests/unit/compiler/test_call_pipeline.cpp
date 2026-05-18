/**
 * @file test_call_pipeline.cpp
 * @brief PR-5 Call/Vararg/MultiRet pipeline tests.
 *
 * Verifies that emitCallExpr / emitVarargExpr produce correct bytecode
 * and that multret propagation works for: return f(), return ...,
 * return (f()), g(f()), {f()}, local a,b = f(), a,b = f(), (f()),
 * and print((f()))-style parenthesized call consumption.
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_manager.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Call Pipeline (PR-5)";

bool runLua(LuaState* L, const char* code) {
    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test_call_pipeline");
        if (proto == nullptr) {
            return false;
        }

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
        delete proto;
        return true;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Call pipeline chunk exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cout << "  [ERROR] Call pipeline chunk unknown exception" << std::endl;
        return false;
    }
}

LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

bool protoContainsOp(const Proto* proto, OpCode op) {
    if (proto == nullptr) {
        return false;
    }

    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op) {
            return true;
        }
    }

    for (usize i = 0; i < proto->getSubProtoCount(); i++) {
        if (protoContainsOp(proto->getSubProto(i), op)) {
            return true;
        }
    }

    return false;
}

i32 reportCallStackDepth(LuaState* L) {
    L->pushNumber(static_cast<f64>(L->getCallStackSize()));
    return 1;
}

} // namespace

// -- return f() multret propagation --
void testReturnCallMultret(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pack3() return 10, 20, 30 end
        local function relay() return pack3() end
        local a, b, c = relay()
        assert(a == 10 and b == 20 and c == 30,
               "return f() should propagate all return values")
    )lua");
    ASSERT_TRUE(suite, ok, "return f() propagates multret");
    delete L;
}

// -- return ... multret propagation --
void testReturnVarargMultret(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function relay(...)
            return ...
        end
        local a, b, c = relay(10, 20, 30)
        assert(a == 10 and b == 20 and c == 30,
               "return ... should propagate all varargs")
    )lua");
    ASSERT_TRUE(suite, ok, "return ... propagates multret");
    delete L;
}

// -- g(f()) last-arg multret --
void testCallLastArgMultret(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local out = {}
        local function capture(a, b, c, d)
            out[1], out[2], out[3], out[4] = a, b, c, d
        end
        local function pack3() return 10, 20, 30 end
        capture(pack3())
        assert(out[1] == 10 and out[2] == 20 and out[3] == 30 and out[4] == nil,
               "g(f()) should pass all of f's returns to g")
    )lua");
    ASSERT_TRUE(suite, ok, "g(f()) last-arg multret");
    delete L;
}

// -- g(1, f()) last-arg multret with leading fixed args --
void testCallLeadingFixedPlusMultret(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local out = {}
        local function capture(a, b, c, d)
            out[1], out[2], out[3], out[4] = a, b, c, d
        end
        local function pack2() return 20, 30 end
        capture(10, pack2())
        assert(out[1] == 10 and out[2] == 20 and out[3] == 30 and out[4] == nil,
               "g(fixed, f()) should combine fixed + multret")
    )lua");
    ASSERT_TRUE(suite, ok, "g(fixed, f()) leading fixed + multret");
    delete L;
}

// -- g(f(), 1) non-last-arg collapsed --
void testCallNonLastArgCollapsed(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local out = {}
        local function capture(a, b, c, d)
            out[1], out[2], out[3], out[4] = a, b, c, d
        end
        local function pack3() return 10, 20, 30 end
        capture(pack3(), 99)
        assert(out[1] == 10 and out[2] == 99 and out[3] == nil and out[4] == nil,
               "g(f(), x) should collapse f to single value")
    )lua");
    ASSERT_TRUE(suite, ok, "g(f(), x) non-last collapses multret");
    delete L;
}

// -- (f()) collapse --
void testParenCallCollapse(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pack3() return 10, 20, 30 end
        local a, b = (pack3())
        assert(a == 10 and b == nil,
               "(f()) should collapse to single value")
    )lua");
    ASSERT_TRUE(suite, ok, "(f()) collapses to single value");
    delete L;
}

// -- return (f()) collapse --
void testReturnParenCallCollapse(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pack3() return 10, 20, 30 end
        local function relay() return (pack3()) end
        local a, b, c = relay()
        assert(a == 10 and b == nil and c == nil,
               "return (f()) should collapse to a single value")
    )lua");
    ASSERT_TRUE(suite, ok, "return (f()) collapses to single value");
    delete L;
}

// -- print((f())) style consumption --
void testPrintParenCallCollapse(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local seen = {}
        local function print(...)
            seen.n = select('#', ...)
            seen[1], seen[2], seen[3] = ...
        end
        local function pack3() return 10, 20, 30 end
        print((pack3()))
        assert(seen.n == 1 and seen[1] == 10 and seen[2] == nil,
               "print((f())) should consume exactly one value")
    )lua");
    ASSERT_TRUE(suite, ok, "print((f())) collapses to single argument");
    delete L;
}

// -- local a,b,c = f() multiple locals --
void testLocalMultiAssignFromCall(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pack3() return 10, 20, 30 end
        local a, b, c = pack3()
        assert(a == 10 and b == 20 and c == 30,
               "local a,b,c = f() should unpack all returns")
    )lua");
    ASSERT_TRUE(suite, ok, "local a,b,c = f() unpacks returns");
    delete L;
}

// -- a,b,c = f() assignment from call --
void testAssignMultiFromCall(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pack3() return 10, 20, 30 end
        local a, b, c
        a, b, c = pack3()
        assert(a == 10 and b == 20 and c == 30,
               "a,b,c = f() should unpack all returns")
    )lua");
    ASSERT_TRUE(suite, ok, "a,b,c = f() unpacks returns");
    delete L;
}

// -- {f()} table constructor multret --
void testTableConstructorCallMultret(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pack3() return 10, 20, 30 end
        local t = {pack3()}
        assert(t[1] == 10 and t[2] == 20 and t[3] == 30,
               "{f()} should expand all returned values")
        assert(t[4] == nil, "expanded table should stop after returns")
    )lua");
    ASSERT_TRUE(suite, ok, "{f()} expands multret into table");
    delete L;
}

// -- CallStmt discards returns --
void testCallStmtDiscardsReturns(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local x = 0
        local function sideEffect()
            x = x + 1
            return 10, 20, 30
        end
        sideEffect()
        assert(x == 1, "call statement should execute but discard returns")
    )lua");
    ASSERT_TRUE(suite, ok, "call statement discards returns");
    delete L;
}

// -- method call o:m() multret propagation --
void testMethodCallMultret(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local obj = {}
        function obj:multi()
            return 10, 20, 30
        end
        local a, b, c = obj:multi()
        assert(a == 10 and b == 20 and c == 30,
               "method call should propagate multret")
    )lua");
    ASSERT_TRUE(suite, ok, "method call multret propagation");
    delete L;
}

// -- nested return f(g()) --
void testNestedReturnCallChain(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function double(x) return x * 2 end
        local function getVal() return 5 end
        local function chain() return double(getVal()) end
        local v = chain()
        assert(v == 10, "nested call chain should work")
    )lua");
    ASSERT_TRUE(suite, ok, "nested return f(g()) chain");
    delete L;
}

void testTailReturnCallEmitsTailcall(TestSuite& suite) {
    try {
        Parser parser(R"lua(
            local function target()
                return 42
            end

            local function relay()
                return target()
            end

            return relay()
        )lua");
        Chunk chunk = parser.parse();
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test_tailcall_codegen");

        ASSERT_TRUE(suite, proto != nullptr, "tail call proto generated");
        ASSERT_TRUE(suite, protoContainsOp(proto, OpCode::TAILCALL),
                    "return f() emits TAILCALL");

        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "tail call codegen should not throw");
    }
}

void testTailRecursiveLuaCallReusesFrame(TestSuite& suite) {
    LuaState* L = createFullState();
    Function* depthProbe = new Function(reportCallStackDepth);
    L->getGlobalState().getGC().registerObject(depthProbe);
    L->setGlobal("report_call_depth", Value(depthProbe));

    bool ok = runLua(L, R"lua(
        function loop(n)
            if n == 0 then
                return report_call_depth()
            end
            return loop(n - 1)
        end

        depth_after_tail_recursion = loop(200)
    )lua");

    ASSERT_TRUE(suite, ok, "tail recursive chunk runs");
    Value depth = L->getGlobal("depth_after_tail_recursion");
    ASSERT_TRUE(suite, depth.isNumber(), "tail recursion records call depth");
    if (depth.isNumber()) {
        ASSERT_TRUE(suite, depth.asNumber() <= 5.0,
                    "tail recursion reuses Lua call frames");
    }

    delete L;
}

void registerCallPipelineTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "return f() multret", testReturnCallMultret);
    registry.registerTest(kSuiteName, "return ... multret", testReturnVarargMultret);
    registry.registerTest(kSuiteName, "g(f()) last-arg multret", testCallLastArgMultret);
    registry.registerTest(kSuiteName, "g(fixed, f()) leading + multret", testCallLeadingFixedPlusMultret);
    registry.registerTest(kSuiteName, "g(f(), x) non-last collapse", testCallNonLastArgCollapsed);
    registry.registerTest(kSuiteName, "(f()) collapse", testParenCallCollapse);
    registry.registerTest(kSuiteName, "return (f()) collapse", testReturnParenCallCollapse);
    registry.registerTest(kSuiteName, "print((f())) collapse", testPrintParenCallCollapse);
    registry.registerTest(kSuiteName, "local a,b,c = f()", testLocalMultiAssignFromCall);
    registry.registerTest(kSuiteName, "a,b,c = f()", testAssignMultiFromCall);
    registry.registerTest(kSuiteName, "{f()} table multret", testTableConstructorCallMultret);
    registry.registerTest(kSuiteName, "call stmt discards returns", testCallStmtDiscardsReturns);
    registry.registerTest(kSuiteName, "method call multret", testMethodCallMultret);
    registry.registerTest(kSuiteName, "nested return f(g())", testNestedReturnCallChain);
    registry.registerTest(kSuiteName, "return f() emits TAILCALL", testTailReturnCallEmitsTailcall);
    registry.registerTest(kSuiteName, "tail recursion reuses frames", testTailRecursiveLuaCallReusesFrame);
}
