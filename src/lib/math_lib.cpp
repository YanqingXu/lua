#include "math_lib.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lua {

    StrView MathLib::getName() const noexcept {
        return "math";
    }

    void MathLib::registerFunctions(FunctionRegistry& registry, const LibraryContext& context) {
        // Basic math functions - simplified versions
        LUA_REGISTER_FUNCTION(registry, abs, absFunc);
        LUA_REGISTER_FUNCTION(registry, floor, floorFunc);
        LUA_REGISTER_FUNCTION(registry, ceil, ceilFunc);
        LUA_REGISTER_FUNCTION(registry, sin, sinFunc);
        LUA_REGISTER_FUNCTION(registry, cos, cosFunc);
        LUA_REGISTER_FUNCTION(registry, tan, tanFunc);
        LUA_REGISTER_FUNCTION(registry, sqrt, sqrtFunc);
        LUA_REGISTER_FUNCTION(registry, pow, powFunc);
        LUA_REGISTER_FUNCTION(registry, exp, expFunc);
        LUA_REGISTER_FUNCTION(registry, log, logFunc);
        LUA_REGISTER_FUNCTION(registry, deg, degFunc);
        LUA_REGISTER_FUNCTION(registry, rad, radFunc);
        LUA_REGISTER_FUNCTION(registry, min, minFunc);
        LUA_REGISTER_FUNCTION(registry, max, maxFunc);
        LUA_REGISTER_FUNCTION(registry, fmod, fmodFunc);
    }

    // Helper function to get number argument
    f64 getNumberArg(State* state, i32 index, f64 defaultValue = 0.0) {
        if (index > state->getTop()) return defaultValue;
        Value val = state->get(index);
        return val.isNumber() ? val.asNumber() : defaultValue;
    }

    // Simplified function implementations
    Value MathLib::absFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::abs(x));
    }

    Value MathLib::floorFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::floor(x));
    }

    Value MathLib::ceilFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::ceil(x));
    }

    Value MathLib::sinFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::sin(x));
    }

    Value MathLib::cosFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::cos(x));
    }

    Value MathLib::tanFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::tan(x));
    }

    Value MathLib::sqrtFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        if (x < 0) return Value(); // Return nil for negative input
        return Value(std::sqrt(x));
    }

    Value MathLib::powFunc(State* state, i32 nargs) {
        if (nargs < 2) return Value();
        f64 x = getNumberArg(state, 1);
        f64 y = getNumberArg(state, 2);
        return Value(std::pow(x, y));
    }

    Value MathLib::expFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(std::exp(x));
    }

    Value MathLib::logFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        if (x <= 0) return Value(); // Return nil for non-positive input
        return Value(std::log(x));
    }

    Value MathLib::degFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(x * 180.0 / M_PI);
    }

    Value MathLib::radFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 x = getNumberArg(state, 1);
        return Value(x * M_PI / 180.0);
    }

    Value MathLib::minFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 result = getNumberArg(state, 1);
        for (i32 i = 2; i <= nargs; ++i) {
            f64 value = getNumberArg(state, i);
            result = std::min(result, value);
        }
        return Value(result);
    }

    Value MathLib::maxFunc(State* state, i32 nargs) {
        if (nargs < 1) return Value();
        f64 result = getNumberArg(state, 1);
        for (i32 i = 2; i <= nargs; ++i) {
            f64 value = getNumberArg(state, i);
            result = std::max(result, value);
        }
        return Value(result);
    }

    Value MathLib::fmodFunc(State* state, i32 nargs) {
        if (nargs < 2) return Value();
        f64 x = getNumberArg(state, 1);
        f64 y = getNumberArg(state, 2);
        if (y == 0) return Value(); // Return nil for division by zero
        return Value(std::fmod(x, y));
    }

} // namespace Lua
