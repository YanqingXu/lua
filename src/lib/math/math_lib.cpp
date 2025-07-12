#include "math_lib.hpp"
#include "../../vm/table.hpp"
#include "../../common/defines.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <ctime>

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

    // Math library initialized
}

// ===================================================================
// Basic Math Function Implementations
// ===================================================================

Value MathLib::abs(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::abs(num));
}

Value MathLib::floor(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::floor(num));
}

Value MathLib::ceil(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::ceil(num));
}

Value MathLib::sqrt(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num < 0) return Value(); // nil for negative numbers

    return Value(std::sqrt(num));
}

Value MathLib::pow(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
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
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::sin(num));
}

Value MathLib::cos(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::cos(num));
}

Value MathLib::tan(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::tan(num));
}

Value MathLib::asin(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num < -1.0 || num > 1.0) return Value(); // nil for out of domain
    return Value(std::asin(num));
}

Value MathLib::acos(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num < -1.0 || num > 1.0) return Value(); // nil for out of domain
    return Value(std::acos(num));
}

Value MathLib::atan(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::atan(num));
}

Value MathLib::atan2(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    Value yVal = state->get(1);
    Value xVal = state->get(2);

    if (!yVal.isNumber() || !xVal.isNumber()) return Value();

    f64 y = yVal.asNumber();
    f64 x = xVal.asNumber();

    return Value(std::atan2(y, x));
}

// ===================================================================
// Logarithmic and Exponential Function Implementations
// ===================================================================

Value MathLib::log(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num <= 0) return Value(); // nil for non-positive numbers

    return Value(std::log(num));
}

Value MathLib::log10(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num <= 0) return Value(); // nil for non-positive numbers

    return Value(std::log10(num));
}

Value MathLib::exp(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
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
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
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
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
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
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    Value xVal = state->get(1);
    Value yVal = state->get(2);

    if (!xVal.isNumber() || !yVal.isNumber()) return Value();

    f64 x = xVal.asNumber();
    f64 y = yVal.asNumber();

    if (y == 0) return Value(); // nil for division by zero

    return Value(std::fmod(x, y));
}

// ===================================================================
// Random Function Implementations
// ===================================================================

Value MathLib::random(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    if (nargs == 0) {
        // Return random float between 0 and 1
        return Value(static_cast<f64>(std::rand()) / RAND_MAX);
    } else if (nargs == 1) {
        // Return random integer between 1 and n
        Value nVal = state->get(1);
        if (!nVal.isNumber()) return Value();

        i32 n = static_cast<i32>(nVal.asNumber());
        if (n <= 0) return Value();

        return Value(static_cast<f64>(std::rand() % n + 1));
    } else if (nargs >= 2) {
        // Return random integer between m and n
        Value mVal = state->get(1);
        Value nVal = state->get(2);

        if (!mVal.isNumber() || !nVal.isNumber()) return Value();

        i32 m = static_cast<i32>(mVal.asNumber());
        i32 n = static_cast<i32>(nVal.asNumber());

        if (m > n) return Value();

        return Value(static_cast<f64>(std::rand() % (n - m + 1) + m));
    }

    return Value();
}

Value MathLib::randomseed(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value seedVal = state->get(1);
    if (!seedVal.isNumber()) return Value();

    unsigned int seed = static_cast<unsigned int>(seedVal.asNumber());
    std::srand(seed);

    return Value(); // nil
}

// ===================================================================
// Additional Math Utility Functions
// ===================================================================

Value MathLib::modf(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    f64 intpart;
    f64 fracpart = std::modf(num, &intpart);

    // In Lua, modf returns two values: integer part and fractional part
    // For now, just return the fractional part
    // TODO: Implement proper multiple return values
    return Value(fracpart);
}

Value MathLib::frexp(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    int exp;
    f64 mantissa = std::frexp(num, &exp);

    // In Lua, frexp returns two values: mantissa and exponent
    // For now, just return the mantissa
    // TODO: Implement proper multiple return values
    return Value(mantissa);
}

Value MathLib::ldexp(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    Value mantissaVal = state->get(1);
    Value expVal = state->get(2);

    if (!mantissaVal.isNumber() || !expVal.isNumber()) return Value();

    f64 mantissa = mantissaVal.asNumber();
    int exp = static_cast<int>(expVal.asNumber());

    return Value(std::ldexp(mantissa, exp));
}

Value MathLib::deg(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 radians = val.asNumber();
    return Value(radians * 180.0 / M_PI);
}

Value MathLib::rad(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
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
