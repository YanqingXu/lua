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
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Call Pipeline (PR-5)";

bool runLua(LuaState* L, const char* code) {
    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
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

void testCompatArgTableForOldStyleVararg(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        _G.arg = nil

        local function oldstyle(a, ...)
            assert(type(arg) == "table")
            if a == nil then
                assert(arg.n == 0)
                return nil
            end
            assert(arg.n == 3)
            assert(arg[1] == "x" and arg[2] == nil and arg[3] == "z")
            if type(a) == "table" then
                assert(a[1] == 10)
                return a[1]
            end
            return a
        end

        local function newstyle(...)
            assert(arg == nil)
            local t = {...}
            return t[1], t[2]
        end

        assert(oldstyle() == nil)
        assert(oldstyle(10, "x", nil, "z") == 10)
        assert(oldstyle({10}, "x", nil, "z") == 10)

        local function compare(a, ...)
            assert(type(arg) == "table")
            for i = 1, arg.n do
                assert(a[i] == arg[i])
            end
            return arg.n
        end

        assert(compare() == 0)
        assert(compare({1, 2, 3}, 1, 2, 3) == 3)

        local a, b = newstyle(20, 30)
        assert(a == 20 and b == 30)
    )lua");
    ASSERT_TRUE(suite, ok, "old-style vararg creates local arg table without breaking ...");
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

void testTableConstructorCollapsesNonLastCalls(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        function unroll(t, i)
            i = i or 1
            if i <= table.getn(t) then
                return t[i], unroll(t, i + 1)
            end
        end

        function ret2(a, b) return a, b end
        local values = ret2{ unroll{1,2,3}, unroll{3,2,1}, unroll{"a", "b"}}
        assert(values[1] == 1 and values[2] == 3 and values[3] == "a" and values[4] == "b",
               "non-last calls inside a table argument should collapse to one value")
    )lua");
    ASSERT_TRUE(suite, ok, "table constructor collapses non-last calls");
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

void testBinaryComparisonKeepsLeftCallResult(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        a = {i = 10}
        self = 20
        function a:x (x) return x+self.i end
        function a.y (x) return x+self end
        assert(a:x(1)+10 == a.y(1),
               "right-hand call arguments must not overwrite the left expression result")
    )lua");
    ASSERT_TRUE(suite, ok, "binary comparison keeps left call-derived result");
    delete L;
}

void testImmediatelyInvokedFunctionExpression(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local a = 0
        (function (x) a = x end)(23)
        assert(a == 23, "parenthesized function expression calls should use the closure as callee")
    )lua");
    ASSERT_TRUE(suite, ok, "immediately invoked function expression");
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

void testGenericForExplicitLuaIteratorTriple(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function range(limit, control)
            local next_value = control + 1
            if next_value <= limit then
                return next_value, next_value * 10
            end
        end

        local sum_i = 0
        local sum_v = 0
        for i, v in range, 4, 0 do
            sum_i = sum_i + i
            sum_v = sum_v + v
        end

        assert(sum_i == 10, "generic for should update control from Lua iterator")
        assert(sum_v == 100, "generic for should copy all requested iterator results")
    )lua");
    ASSERT_TRUE(suite, ok, "generic for explicit Lua iterator triple");
    delete L;
}

void testGenericForCallReturningLuaIterator(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function make_range(limit)
            local current = 0
            return function()
                current = current + 1
                if current <= limit then
                    return current, current * 2
                end
            end
        end

        local seen = {}
        local total = 0
        for i, v in make_range(4) do
            seen[i] = v
            total = total + v
        end

        assert(seen[1] == 2 and seen[2] == 4 and seen[3] == 6 and seen[4] == 8,
               "generic for should accept a Lua iterator returned by a call")
        assert(total == 20, "generic for should continue until iterator returns nil")
    )lua");
    ASSERT_TRUE(suite, ok, "generic for call returning Lua iterator");
    delete L;
}

void testGenericForLoopVarSurvivesLocalCallInitializer(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local joined = ""
        for _, n in pairs{"A", "B"} do
            local replaced, count = string.gsub("x", "x", n)
            joined = joined .. replaced .. count
        end

        assert(joined == "A1B1",
               "generic for loop variables should remain readable in local call initializers")
    )lua");
    ASSERT_TRUE(suite, ok, "generic for loop var survives local call initializer");
    delete L;
}

void testGenericForLocalCallInitializerCanReadShadowedOuter(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local joined = ""
        local prog = "x"
        for _, n in pairs{"A", "B"} do
            local prog, count = string.gsub(prog, "x", n)
            joined = joined .. prog .. count
        end

        assert(joined == "A1B1",
               "local call initializer should read the outer variable before the new local is active")
    )lua");
    ASSERT_TRUE(suite, ok, "generic for local call initializer reads shadowed outer");
    delete L;
}

void testGenericForAfterTemporaryHeavyAssignments(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local sentinel = true
        leak1 = "one"
        leak2 = "two"
        leak3 = "three"
        prog = "x"

        joined = ""
        for _, n in pairs{"A", "B"} do
            local prog, count = string.gsub(prog, "x", n)
            joined = joined .. prog .. count
        end

        assert(sentinel == true and joined == "A1B1",
               "generic for registers should start after active locals, not leaked temporaries")
    )lua");
    ASSERT_TRUE(suite, ok, "generic for after temporary-heavy assignments");
    delete L;
}

void testOfficialLineEndingRewriteLoop(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function dostring (x) return assert(loadstring(x))() end

        prog = [[
a = 1        -- a comment
b = 2


x = [=[
hi
]=]
y = "\
hello\r\n\
"
return debug.getinfo(1).currentline
]]

        for _, n in pairs{"\n", "\r", "\n\r", "\r\n"} do
            local prog, nn = string.gsub(prog, "\n", n)
            assert(dostring(prog) == nn)
            assert(_G.x == "hi\n" and _G.y == "\nhello\r\n\n")
        end
    )lua");
    ASSERT_TRUE(suite, ok, "official literals line-ending rewrite loop");
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
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test_tailcall_codegen");

        ASSERT_TRUE(suite, proto != nullptr, "tail call proto generated");
        ASSERT_TRUE(suite, protoContainsOp(proto, OpCode::TAILCALL),
                    "return f() emits TAILCALL");

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

        depth_after_tail_recursion = loop(20000)
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

void testDeepTailCallErrorDiagnosticsAreBounded(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local function loop(n)
            if n == 0 then
                return missing_tail_target()
            end
            return loop(n - 1)
        end

        local ok, msg = pcall(loop, 20000)
        deep_tail_error_named =
            (not ok and string.find(msg, "global 'missing_tail_target'", 1, true) ~= nil)
    )lua");

    ASSERT_TRUE(suite, ok, "deep tail call error diagnostic chunk runs");
    ASSERT_TRUE(suite, L->getGlobal("deep_tail_error_named").asBoolean(),
                "deep tail call error diagnostic remains bounded and names global");

    delete L;
}

void testLargeTableConstructorUsesExtendedSetList(TestSuite& suite) {
    LuaState* L = createFullState();

    std::string code;
    code.reserve(180000);
    code += "local t = {\n";
    for (i32 i = 1; i <= 25551; ++i) {
        code += std::to_string(i);
        code += ",\n";
    }
    code += R"lua(
}
assert(t[1] == 1)
assert(t[25550] == 25550)
assert(t[25551] == 25551)
)lua";

    bool ok = runLua(L, code.c_str());
    ASSERT_TRUE(suite, ok, "large table constructor crosses SETLIST extended block boundary");
    delete L;
}

void registerCallPipelineTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "return f() multret", testReturnCallMultret);
    registry.registerTest(kSuiteName, "return ... multret", testReturnVarargMultret);
    registry.registerTest(kSuiteName, "compat arg table for old-style vararg", testCompatArgTableForOldStyleVararg);
    registry.registerTest(kSuiteName, "g(f()) last-arg multret", testCallLastArgMultret);
    registry.registerTest(kSuiteName, "g(fixed, f()) leading + multret", testCallLeadingFixedPlusMultret);
    registry.registerTest(kSuiteName, "g(f(), x) non-last collapse", testCallNonLastArgCollapsed);
    registry.registerTest(kSuiteName, "(f()) collapse", testParenCallCollapse);
    registry.registerTest(kSuiteName, "return (f()) collapse", testReturnParenCallCollapse);
    registry.registerTest(kSuiteName, "print((f())) collapse", testPrintParenCallCollapse);
    registry.registerTest(kSuiteName, "local a,b,c = f()", testLocalMultiAssignFromCall);
    registry.registerTest(kSuiteName, "a,b,c = f()", testAssignMultiFromCall);
    registry.registerTest(kSuiteName, "{f()} table multret", testTableConstructorCallMultret);
    registry.registerTest(kSuiteName, "table constructor collapses non-last calls",
                          testTableConstructorCollapsesNonLastCalls);
    registry.registerTest(kSuiteName, "call stmt discards returns", testCallStmtDiscardsReturns);
    registry.registerTest(kSuiteName, "method call multret", testMethodCallMultret);
    registry.registerTest(kSuiteName, "binary comparison keeps left call result",
                          testBinaryComparisonKeepsLeftCallResult);
    registry.registerTest(kSuiteName, "immediately invoked function expression",
                          testImmediatelyInvokedFunctionExpression);
    registry.registerTest(kSuiteName, "nested return f(g())", testNestedReturnCallChain);
    registry.registerTest(kSuiteName, "generic for explicit Lua iterator triple", testGenericForExplicitLuaIteratorTriple);
    registry.registerTest(kSuiteName, "generic for call returning Lua iterator", testGenericForCallReturningLuaIterator);
    registry.registerTest(kSuiteName, "generic for loop var survives local call initializer",
                          testGenericForLoopVarSurvivesLocalCallInitializer);
    registry.registerTest(kSuiteName, "generic for local call initializer reads shadowed outer",
                          testGenericForLocalCallInitializerCanReadShadowedOuter);
    registry.registerTest(kSuiteName, "generic for after temporary-heavy assignments",
                          testGenericForAfterTemporaryHeavyAssignments);
    registry.registerTest(kSuiteName, "official literals line-ending rewrite loop",
                          testOfficialLineEndingRewriteLoop);
    registry.registerTest(kSuiteName, "return f() emits TAILCALL", testTailReturnCallEmitsTailcall);
    registry.registerTest(kSuiteName, "tail recursion reuses frames", testTailRecursiveLuaCallReusesFrame);
    registry.registerTest(kSuiteName, "deep tail call error diagnostics are bounded",
                          testDeepTailCallErrorDiagnosticsAreBounded);
    registry.registerTest(kSuiteName, "large table constructor uses extended SETLIST",
                          testLargeTableConstructorUsesExtendedSetList);
}

