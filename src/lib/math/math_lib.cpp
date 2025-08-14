#include "math_lib.hpp"
#include "../core/multi_return_helper.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_string.hpp"
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

void MathLib::registerFunctions(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Create math library table
    Value mathTable = LibRegistry::createLibTable(state, "math");

    // Register basic math functions (legacy)
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "abs", abs);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "floor", floor);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "ceil", ceil);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "sqrt", sqrt);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "pow", pow);

    // Register trigonometric functions (legacy)
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "sin", sin);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "cos", cos);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "tan", tan);

    // Register logarithmic and exponential functions (legacy)
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "log", log);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "exp", exp);

    // Register min/max functions (legacy)
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "min", min);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "max", max);

    // Register multi-return functions using new mechanism
    LibRegistry::registerTableFunction(state, mathTable, "modf", modf);
    LibRegistry::registerTableFunction(state, mathTable, "frexp", frexp);

    // Register legacy single-return functions
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "fmod", fmod);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "ldexp", ldexp);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "deg", deg);
    LibRegistry::registerTableFunctionLegacy(state, mathTable, "rad", rad);
}

void MathLib::initialize(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Set mathematical constants
    auto mathStr = GCString::create("math");
    Value mathTable = state->getGlobal(mathStr);
    if (mathTable.isTable()) {
        auto table = mathTable.asTable();
        auto piStr = GCString::create("pi");
        auto hugeStr = GCString::create("huge");
        table->set(Value(piStr), Value(M_PI));
        table->set(Value(hugeStr), Value(HUGE_VAL));
    }

    // Math library initialized
}

// ===================================================================
// Basic Math Function Implementations
// ===================================================================

Value MathLib::abs(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;
    Value val = state->get(stackIdx);

    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::abs(num));
}

Value MathLib::floor(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;
    Value val = state->get(stackIdx);

    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::floor(num));
}

Value MathLib::ceil(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;
    Value val = state->get(stackIdx);

    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::ceil(num));
}

Value MathLib::sqrt(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(-nargs);  // First argument is at -nargs from top
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    if (num < 0) return Value(); // nil for negative numbers

    return Value(std::sqrt(num));
}

Value MathLib::pow(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    Value baseVal = state->get(-nargs);      // First argument (base)
    Value expVal = state->get(-nargs + 1);   // Second argument (exponent)

    if (!baseVal.isNumber() || !expVal.isNumber()) return Value();

    f64 base = baseVal.asNumber();
    f64 exp = expVal.asNumber();

    return Value(std::pow(base, exp));
}

// ===================================================================
// Trigonometric Function Implementations
// ===================================================================

Value MathLib::sin(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(-nargs);  // First argument is at -nargs from top
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::sin(num));
}

Value MathLib::cos(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(-nargs);  // First argument is at -nargs from top
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::cos(num));
}

Value MathLib::tan(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(-nargs);  // First argument is at -nargs from top
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::tan(num));
}

Value MathLib::asin(LuaState* state, i32 nargs) {
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

Value MathLib::acos(LuaState* state, i32 nargs) {
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

Value MathLib::atan(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 num = val.asNumber();
    return Value(std::atan(num));
}

Value MathLib::atan2(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    Value yVal = state->get(-nargs);      // First argument (y)
    Value xVal = state->get(-nargs + 1);  // Second argument (x)

    if (!yVal.isNumber() || !xVal.isNumber()) return Value();

    f64 y = yVal.asNumber();
    f64 x = xVal.asNumber();

    return Value(std::atan2(y, x));
}

// ===================================================================
// Logarithmic and Exponential Function Implementations
// ===================================================================

Value MathLib::log(LuaState* state, i32 nargs) {
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

Value MathLib::log10(LuaState* state, i32 nargs) {
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

Value MathLib::exp(LuaState* state, i32 nargs) {
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

Value MathLib::min(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    f64 minVal = HUGE_VAL;
    bool hasValidNumber = false;

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;

    // Iterate through all arguments
    for (i32 i = 0; i < nargs; ++i) {
        Value val = state->get(stackIdx + i);
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

Value MathLib::max(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    f64 maxVal = -HUGE_VAL;
    bool hasValidNumber = false;

    // Legacy function: arguments are directly on stack without module parameter
    int stackIdx = state->getTop() - nargs;

    // Iterate through all arguments
    for (i32 i = 0; i < nargs; ++i) {
        Value val = state->get(stackIdx + i);
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

Value MathLib::fmod(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 2) return Value();

    Value xVal = state->get(-nargs);      // First argument (x)
    Value yVal = state->get(-nargs + 1);  // Second argument (y)

    if (!xVal.isNumber() || !yVal.isNumber()) return Value();

    f64 x = xVal.asNumber();
    f64 y = yVal.asNumber();

    if (y == 0) return Value(); // nil for division by zero

    return Value(std::fmod(x, y));
}

// ===================================================================
// Random Function Implementations
// ===================================================================

Value MathLib::random(LuaState* state, i32 nargs) {
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
        Value nVal = state->get(-nargs);  // First argument
        if (!nVal.isNumber()) return Value();

        i32 n = static_cast<i32>(nVal.asNumber());
        if (n <= 0) return Value();

        return Value(static_cast<f64>(std::rand() % n + 1));
    } else if (nargs >= 2) {
        // Return random integer between m and n
        Value mVal = state->get(-nargs);      // First argument (m)
        Value nVal = state->get(-nargs + 1);  // Second argument (n)

        if (!mVal.isNumber() || !nVal.isNumber()) return Value();

        i32 m = static_cast<i32>(mVal.asNumber());
        i32 n = static_cast<i32>(nVal.asNumber());

        if (m > n) return Value();

        return Value(static_cast<f64>(std::rand() % (n - m + 1) + m));
    }

    return Value();
}

Value MathLib::randomseed(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value seedVal = state->get(-nargs);  // First argument
    if (!seedVal.isNumber()) return Value();

    unsigned int seed = static_cast<unsigned int>(seedVal.asNumber());
    std::srand(seed);

    return Value(); // nil
}

// ===================================================================
// Additional Math Utility Functions
// ===================================================================

// New Lua 5.1 standard modf implementation (multi-return)
i32 MathLib::modf(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 1) {
        throw std::invalid_argument("math.modf: expected 1 argument (number)");
    }

    // Get the first argument (now in clean stack environment)
    Value val = state->get(0);  // First argument is at index 0
    if (!val.isNumber()) {
        throw std::invalid_argument("math.modf: argument must be a number");
    }

    f64 num = val.asNumber();
    f64 intpart;
    f64 fracpart = std::modf(num, &intpart);

    // Clear arguments and push results
    state->clearStack();
    state->push(Value(intpart));   // Integer part first
    state->push(Value(fracpart));  // Fractional part second

    // Return 2 values
    return 2;
}

// New Lua 5.1 standard frexp implementation (multi-return)
i32 MathLib::frexp(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 1) {
        throw std::invalid_argument("math.frexp: expected 1 argument (number)");
    }

    // Get the first argument (now in clean stack environment)
    Value val = state->get(0);  // First argument is at index 0
    if (!val.isNumber()) {
        throw std::invalid_argument("math.frexp: argument must be a number");
    }

    f64 num = val.asNumber();
    int exp;
    f64 mantissa = std::frexp(num, &exp);

    // Clear arguments and push results
    state->clearStack();
    state->push(Value(mantissa));                    // Mantissa first
    state->push(Value(static_cast<f64>(exp)));       // Exponent second (as number)

    // Return 2 values
    return 2;
}

Value MathLib::ldexp(LuaState* state, i32 nargs) {
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

Value MathLib::deg(LuaState* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State pointer cannot be null");
    }
    if (nargs < 1) return Value();

    Value val = state->get(1);
    if (!val.isNumber()) return Value();

    f64 radians = val.asNumber();
    return Value(radians * 180.0 / M_PI);
}

Value MathLib::rad(LuaState* state, i32 nargs) {
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

void initializeMathLib(LuaState* state) {
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
