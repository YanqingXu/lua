#include "math_lib.hpp"
#include "../../vm/table.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lua {

// ===================================================================
// MathLib Implementation
// ===================================================================

void MathLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create math library table
    Value mathTable = LibRegistry::createLibTable(state, "math");

    // Register basic math functions
    REGISTER_TABLE_FUNCTION(state, mathTable, abs, abs);
    REGISTER_TABLE_FUNCTION(state, mathTable, floor, floor);
    REGISTER_TABLE_FUNCTION(state, mathTable, ceil, ceil);
    REGISTER_TABLE_FUNCTION(state, mathTable, sqrt, sqrt);
    REGISTER_TABLE_FUNCTION(state, mathTable, pow, pow);

    // Register trigonometric functions
    REGISTER_TABLE_FUNCTION(state, mathTable, sin, sin);
    REGISTER_TABLE_FUNCTION(state, mathTable, cos, cos);
    REGISTER_TABLE_FUNCTION(state, mathTable, tan, tan);

    // Register logarithmic and exponential functions
    REGISTER_TABLE_FUNCTION(state, mathTable, log, log);
    REGISTER_TABLE_FUNCTION(state, mathTable, exp, exp);

    // Register min/max functions
    REGISTER_TABLE_FUNCTION(state, mathTable, min, min);
    REGISTER_TABLE_FUNCTION(state, mathTable, max, max);

    // Register other utility functions
    REGISTER_TABLE_FUNCTION(state, mathTable, fmod, fmod);
    REGISTER_TABLE_FUNCTION(state, mathTable, deg, deg);
    REGISTER_TABLE_FUNCTION(state, mathTable, rad, rad);
}

void MathLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Set mathematical constants
    Value mathTable = state->getGlobal("math");
    if (mathTable.isTable()) {
        auto table = mathTable.asTable();
        table->set(Value("pi"), Value(M_PI));
        table->set(Value("huge"), Value(HUGE_VAL));
    }

    std::cout << "[MathLib] Initialized successfully!" << std::endl;
}

// ===================================================================
// Basic Math Function Implementations
// ===================================================================

Value MathLib::abs(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::abs(num));
}

Value MathLib::floor(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::floor(num));
}

Value MathLib::ceil(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::ceil(num));
}

Value MathLib::sqrt(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num < 0) return Value(); // nil for negative numbers

    return Value(std::sqrt(num));
}

Value MathLib::pow(State* state, i32 nargs) {
    if (nargs < 2) return Value();

    Value baseVal = state->get(1);
    Value expVal = state->get(2);

    if (!baseVal.isNumber() || !expVal.isNumber()) return Value();

    f64 base = baseVal.asNumber();
    f64 exp = expVal.asNumber();

    return Value(std::pow(base, exp));
}

// ===================================================================
// Trigonometric Function Implementations
// ===================================================================

Value MathLib::sin(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::sin(num));
}

Value MathLib::cos(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::cos(num));
}

Value MathLib::tan(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::tan(num));
}

// ===================================================================
// Logarithmic and Exponential Function Implementations
// ===================================================================

Value MathLib::log(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num <= 0) return Value(); // nil for non-positive numbers

    return Value(std::log(num));
}

Value MathLib::exp(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::exp(num));
}

// ===================================================================
// Min/Max Function Implementations
// ===================================================================

Value MathLib::min(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    f64 minVal = HUGE_VAL;
    bool hasValidNumber = false;

    for (i32 i = 1; i <= nargs; ++i) {
        Value val = state->get(i);
        if (val.isNumber()) {
            f64 num = val.asNumber();
            if (!hasValidNumber || num < minVal) {
                minVal = num;
                hasValidNumber = true;
            }
        }
    }

    return hasValidNumber ? Value(minVal) : Value();
}

Value MathLib::max(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    f64 maxVal = -HUGE_VAL;
    bool hasValidNumber = false;

    for (i32 i = 1; i <= nargs; ++i) {
        Value val = state->get(i);
        if (val.isNumber()) {
            f64 num = val.asNumber();
            if (!hasValidNumber || num > maxVal) {
                maxVal = num;
                hasValidNumber = true;
            }
        }
    }

    return hasValidNumber ? Value(maxVal) : Value();
}

// ===================================================================
// Other Utility Function Implementations
// ===================================================================

Value MathLib::fmod(State* state, i32 nargs) {
    if (nargs < 2) return Value();

    Value xVal = state->get(1);
    Value yVal = state->get(2);

    if (!xVal.isNumber() || !yVal.isNumber()) return Value();

    f64 x = xVal.asNumber();
    f64 y = yVal.asNumber();

    if (y == 0) return Value(); // nil for division by zero

    return Value(std::fmod(x, y));
}

Value MathLib::deg(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 radians = val.asNumber();
    return Value(radians * 180.0 / M_PI);
}

Value MathLib::rad(State* state, i32 nargs) {
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 degrees = val.asNumber();
    return Value(degrees * M_PI / 180.0);
}

// ===================================================================
// Convenient Initialization Functions
// ===================================================================

void initializeMathLib(State* state) {
    MathLib mathLib;
    mathLib.registerFunctions(state);
    mathLib.initialize(state);
}

// For backward compatibility, provide old function names
namespace Lua {
    std::unique_ptr<LibModule> createMathLib() {
        return std::make_unique<MathLib>();
    }
}

} // namespace Lua
