#include "../framework/test_framework.hpp"
#include "common/lua_error.hpp"
#include "lib/mathlib.hpp"
#include "vm/state/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <vector>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Math Library";

i32 callMathFunc(LuaState* L, const char* funcName, const std::function<void(LuaState*)>& pushArgs) {
    Value mathTable = L->getGlobal("math");
    if (!mathTable.isTable()) {
        return -1;
    }

    Table* table = mathTable.asTable();
    GCString* key = L->getGlobalState().getStringPool().intern(funcName);
    Value func = table->get(Value(key));
    if (!func.isFunction()) {
        return -1;
    }

    L->getStack().clear();
    L->setAbsoluteTop(0);

    if (pushArgs) {
        pushArgs(L);
    }

    Function* f = func.asFunction();
    if (f->isCFunction()) {
        return f->getCFunction()(L);
    }

    return -1;
}

bool nearlyEqual(f64 left, f64 right, f64 epsilon = 1e-12) {
    return std::fabs(left - right) <= epsilon;
}

} // namespace

void testMathConstants(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();

    Value mathTable = L->getGlobal("math");
    ASSERT_TRUE(suite, mathTable.isTable(), "math table exists");
    if (!mathTable.isTable()) {
        return;
    }

    Table* table = mathTable.asTable();
    auto& pool = L->getGlobalState().getStringPool();
    Value pi = table->get(Value(pool.intern("pi")));
    Value huge = table->get(Value(pool.intern("huge")));

    ASSERT_TRUE(suite, pi.isNumber(), "math.pi is number");
    ASSERT_TRUE(suite, huge.isNumber(), "math.huge is number");
    if (pi.isNumber()) {
        ASSERT_TRUE(suite, nearlyEqual(pi.asNumber(), std::acos(-1.0)), "math.pi has expected value");
    }
}

void testMathHyperbolicFunctions(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();

    i32 ret = callMathFunc(L, "sinh", [](LuaState* s) {
        s->pushNumber(0.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.sinh returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.sinh returns number");
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 0.0), "math.sinh(0) == 0");

    ret = callMathFunc(L, "cosh", [](LuaState* s) {
        s->pushNumber(0.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.cosh returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.cosh returns number");
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 1.0), "math.cosh(0) == 1");

    ret = callMathFunc(L, "tanh", [](LuaState* s) {
        s->pushNumber(0.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.tanh returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.tanh returns number");
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 0.0), "math.tanh(0) == 0");

    ret = callMathFunc(L, "sinh", [](LuaState* s) {
        s->pushNumber(1.0);
    });
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), std::sinh(1.0)), "math.sinh(1) matches std::sinh");

    ret = callMathFunc(L, "cosh", [](LuaState* s) {
        s->pushNumber(1.0);
    });
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), std::cosh(1.0)), "math.cosh(1) matches std::cosh");

    ret = callMathFunc(L, "tanh", [](LuaState* s) {
        s->pushNumber(1.0);
    });
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), std::tanh(1.0)), "math.tanh(1) matches std::tanh");
}

void testMathModAlias(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();

    i32 ret = callMathFunc(L, "mod", [](LuaState* s) {
        s->pushNumber(7.0);
        s->pushNumber(3.0);
    });

    ASSERT_EQ(suite, ret, 1, "math.mod returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.mod returns number");
    if (L->top().isNumber()) {
        ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 1.0), "math.mod returns Lua remainder");
    }

    ret = callMathFunc(L, "mod", [](LuaState* s) {
        s->pushNumber(-4.0);
        s->pushNumber(3.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.mod negative dividend returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber() && nearlyEqual(L->top().asNumber(), 2.0),
                "math.mod should use Lua floor remainder for negative dividends");

    ret = callMathFunc(L, "mod", [](LuaState* s) {
        s->pushNumber(4.0);
        s->pushNumber(-3.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.mod negative divisor returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber() && nearlyEqual(L->top().asNumber(), -2.0),
                "math.mod should use Lua floor remainder for negative divisors");
}

void testMathArgumentErrorUsesFunctionName(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    try {
        (void)callMathFunc(L, "sin", [&](LuaState* s) {
            s->pushString(pool.intern("a"));
        });
        ASSERT_TRUE(suite, false, "math.sin rejects non-number argument");
    } catch (const LuaError& error) {
        std::string message = error.what();
        ASSERT_TRUE(suite, message.find("to 'sin'") != std::string::npos,
                    "math argument error names bare function");
        ASSERT_TRUE(suite, message.find("math.sin") == std::string::npos,
                    "math argument error omits table prefix");
    }
}

void testMathFunctionsAcceptNumericStrings(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    i32 ret = callMathFunc(L, "sin", [&](LuaState* s) {
        s->pushString(pool.intern(" 1.5707963267948966 "));
    });

    ASSERT_EQ(suite, ret, 1, "math.sin accepts numeric strings");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.sin numeric string result is number");
    if (L->top().isNumber()) {
        ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 1.0), "math.sin converts numeric string argument");
    }
}

void testMathRandomIsIsolatedAndDeterministicPerContext(TestSuite& suite) {
    EngineContext firstContext;
    EngineContext secondContext;
    UPtr<LuaState> first = LuaState::create(firstContext);
    UPtr<LuaState> second = LuaState::create(secondContext);
    openMathLib(first.get());
    openMathLib(second.get());

    const auto seed = [](LuaState* state, i32 value) {
        return callMathFunc(state, "randomseed", [=](LuaState* s) { s->pushNumber(value); });
    };
    const auto next = [](LuaState* state) {
        (void)callMathFunc(state, "random", [](LuaState* s) {
            s->pushNumber(-1000000);
            s->pushNumber(1000000);
        });
        return state->top().asNumber();
    };

    ASSERT_EQ(suite, 0, seed(first.get(), 12345), "first context accepts an explicit seed");
    ASSERT_EQ(suite, 0, seed(second.get(), 12345), "second context accepts the same explicit seed");
    for (i32 i = 0; i < 8; ++i) {
        ASSERT_EQ(suite, next(first.get()), next(second.get()), "equal per-context seeds reproduce the same sequence");
    }

    (void)seed(first.get(), 777);
    const RuntimeRandom::State saved = secondContext.random().state();
    const LuaNumber expected = next(second.get());
    secondContext.random().restore(saved);
    ASSERT_EQ(suite, expected, next(second.get()), "context RNG state can be snapshotted and restored");
}

void testMathIntegerArgumentsRejectUndefinedConversions(TestSuite& suite) {
    const auto rejectsRandomBound = [](LuaNumber value) {
        LuaStdLibTestContext ctx(openMathLib);
        try {
            (void)callMathFunc(ctx.getState(), "random", [=](LuaState* state) { state->pushNumber(value); });
        } catch (const LuaError&) {
            return true;
        }
        return false;
    };

    ASSERT_TRUE(suite, rejectsRandomBound(std::numeric_limits<LuaNumber>::quiet_NaN()),
                "math.random rejects NaN before integer conversion");
    ASSERT_TRUE(suite, rejectsRandomBound(std::numeric_limits<LuaNumber>::infinity()),
                "math.random rejects positive infinity before integer conversion");
    ASSERT_TRUE(suite, rejectsRandomBound(-std::numeric_limits<LuaNumber>::infinity()),
                "math.random rejects negative infinity before integer conversion");
    ASSERT_TRUE(suite, rejectsRandomBound(std::numeric_limits<LuaNumber>::max()),
                "math.random rejects out-of-range values before integer conversion");
}

void registerMathLibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "constants", testMathConstants);
    registry.registerTest(kSuiteName, "hyperbolic functions", testMathHyperbolicFunctions);
    registry.registerTest(kSuiteName, "mod alias", testMathModAlias);
    registry.registerTest(kSuiteName, "argument error names", testMathArgumentErrorUsesFunctionName);
    registry.registerTest(kSuiteName, "numeric string arguments", testMathFunctionsAcceptNumericStrings);
    registry.registerTest(kSuiteName, "per-context deterministic random", testMathRandomIsIsolatedAndDeterministicPerContext);
    registry.registerTest(kSuiteName, "integer arguments reject undefined conversions",
                          testMathIntegerArgumentsRejectUndefinedConversions);
}
