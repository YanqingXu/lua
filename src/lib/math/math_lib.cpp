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

    void MathLib::registerFunctions(Lib::LibFuncRegistry& registry, const Lib::LibContext& context) {
        // Basic mathematical functions with comprehensive metadata
        {
            FunctionMetadata meta("abs");
            meta.withDescription("Absolute value")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::absFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("floor");
            meta.withDescription("Floor function (largest integer <= x)")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::floorFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("ceil");
            meta.withDescription("Ceiling function (smallest integer >= x)")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::ceilFunc(s, n); });
        }
        
        // Trigonometric functions
        {
            FunctionMetadata meta("sin");
            meta.withDescription("Sine function")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::sinFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("cos");
            meta.withDescription("Cosine function")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::cosFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("tan");
            meta.withDescription("Tangent function")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::tanFunc(s, n); });
        }
        
        // Power and logarithmic functions
        {
            FunctionMetadata meta("sqrt");
            meta.withDescription("Square root")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::sqrtFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("pow");
            meta.withDescription("Power function (x^y)")
                .withArgs(2, 2);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::powFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("exp");
            meta.withDescription("Exponential function (e^x)")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::expFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("log");
            meta.withDescription("Natural logarithm")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::logFunc(s, n); });
        }
        
        // Angle conversion functions
        {
            FunctionMetadata meta("deg");
            meta.withDescription("Convert radians to degrees")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::degFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("rad");
            meta.withDescription("Convert degrees to radians")
                .withArgs(1, 1);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::radFunc(s, n); });
        }
        
        // Min/max functions
        {
            FunctionMetadata meta("min");
            meta.withDescription("Minimum value")
                .withArgs(1, -1)
                .withVariadic();
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::minFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("max");
            meta.withDescription("Maximum value")
                .withArgs(1, -1)
                .withVariadic();
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::maxFunc(s, n); });
        }
        
        {
            FunctionMetadata meta("fmod");
            meta.withDescription("Floating-point remainder")
                .withArgs(2, 2);
            registry.registerFunction(meta, [](State* s, i32 n) { return MathLib::fmodFunc(s, n); });
        }
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
