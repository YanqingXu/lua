#pragma once

#include "../core/lib_module.hpp"
#include "../core/lib_registry.hpp"
#include "../../vm/call_result.hpp"

namespace Lua {

/**
 * @brief Math library implementation
 *
 * Provides Lua mathematical functions:
 * - abs: Absolute value
 * - floor: Floor function
 * - ceil: Ceiling function
 * - sqrt: Square root
 * - pow: Power operation
 * - sin, cos, tan: Trigonometric functions
 * - min, max: Min/max functions
 *
 * This implementation follows the simplified framework design
 * for better performance and maintainability.
 */
class MathLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "math"; }

    /**
     * @brief Register math library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(LuaState* state) override;

    /**
     * @brief Initialize the math library with constants
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(LuaState* state) override;

    // Basic math function declarations with proper documentation
    /**
     * @brief Absolute value function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Absolute value of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value abs(LuaState* state, i32 nargs);

    /**
     * @brief Floor function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Floor of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value floor(LuaState* state, i32 nargs);

    /**
     * @brief Ceiling function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Ceiling of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value ceil(LuaState* state, i32 nargs);

    /**
     * @brief Square root function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Square root of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value sqrt(LuaState* state, i32 nargs);

    /**
     * @brief Power function
     * @param state Lua state containing base and exponent
     * @param nargs Number of arguments (should be 2)
     * @return Base raised to the power of exponent
     * @throws std::invalid_argument if state is null
     */
    static Value pow(LuaState* state, i32 nargs);

    // Trigonometric function declarations
    static Value sin(LuaState* state, i32 nargs);
    static Value cos(LuaState* state, i32 nargs);
    static Value tan(LuaState* state, i32 nargs);
    static Value asin(LuaState* state, i32 nargs);
    static Value acos(LuaState* state, i32 nargs);
    static Value atan(LuaState* state, i32 nargs);
    static Value atan2(LuaState* state, i32 nargs);

    // Logarithmic and exponential functions
    static Value log(LuaState* state, i32 nargs);
    static Value log10(LuaState* state, i32 nargs);
    static Value exp(LuaState* state, i32 nargs);

    // Min/max functions
    static Value min(LuaState* state, i32 nargs);
    static Value max(LuaState* state, i32 nargs);

    // Random functions
    static Value random(LuaState* state, i32 nargs);
    static Value randomseed(LuaState* state, i32 nargs);

    // Other utility functions
    static Value fmod(LuaState* state, i32 nargs);

    // Multi-return value functions (Lua 5.1 standard)
    static i32 modf(LuaState* state);
    static i32 frexp(LuaState* state);

    static Value ldexp(LuaState* state, i32 nargs);
    static Value deg(LuaState* state, i32 nargs);
    static Value rad(LuaState* state, i32 nargs);
};

/**
 * @brief Convenient MathLib initialization function
 * @param state Lua state to initialize math library for
 * @throws std::invalid_argument if state is null
 */
void initializeMathLib(LuaState* state);

} // namespace Lua
