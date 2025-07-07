#pragma once

#include "../core/lib_module.hpp"
#include "../core/lib_registry.hpp"

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
    void registerFunctions(State* state) override;

    /**
     * @brief Initialize the math library with constants
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(State* state) override;

    // Basic math function declarations with proper documentation
    /**
     * @brief Absolute value function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Absolute value of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value abs(State* state, i32 nargs);

    /**
     * @brief Floor function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Floor of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value floor(State* state, i32 nargs);

    /**
     * @brief Ceiling function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Ceiling of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value ceil(State* state, i32 nargs);

    /**
     * @brief Square root function
     * @param state Lua state containing number argument
     * @param nargs Number of arguments (should be 1)
     * @return Square root of the argument
     * @throws std::invalid_argument if state is null
     */
    static Value sqrt(State* state, i32 nargs);

    /**
     * @brief Power function
     * @param state Lua state containing base and exponent
     * @param nargs Number of arguments (should be 2)
     * @return Base raised to the power of exponent
     * @throws std::invalid_argument if state is null
     */
    static Value pow(State* state, i32 nargs);

    // Trigonometric function declarations
    static Value sin(State* state, i32 nargs);
    static Value cos(State* state, i32 nargs);
    static Value tan(State* state, i32 nargs);

    // Logarithmic and exponential functions
    static Value log(State* state, i32 nargs);
    static Value exp(State* state, i32 nargs);

    // Min/max functions
    static Value min(State* state, i32 nargs);
    static Value max(State* state, i32 nargs);

    // Other utility functions
    static Value fmod(State* state, i32 nargs);
    static Value deg(State* state, i32 nargs);
    static Value rad(State* state, i32 nargs);
};

/**
 * @brief Convenient MathLib initialization function
 * @param state Lua state to initialize math library for
 * @throws std::invalid_argument if state is null
 */
void initializeMathLib(State* state);

} // namespace Lua
