#pragma once

#include "../../common/types.hpp"
#include "../../vm/call_result.hpp"
#include "../../vm/value.hpp"

namespace Lua {
    class State;

    /**
     * @brief Multi-return value helper utilities for standard library functions
     * 
     * Provides convenient functions for creating and managing multiple return values
     * in standard library implementations, following Lua 5.1 conventions.
     * 
     * This helper class simplifies the implementation of standard library functions
     * that need to return multiple values, such as pcall, modf, frexp, etc.
     */
    class MultiReturnHelper {
    public:
        /**
         * @brief Create a CallResult with two values (commonly used pattern)
         * @param first The first return value
         * @param second The second return value
         * @return CallResult containing both values
         */
        static CallResult createTwoValues(const Value& first, const Value& second);

        /**
         * @brief Create a CallResult with three values
         * @param first The first return value
         * @param second The second return value
         * @param third The third return value
         * @return CallResult containing all three values
         */
        static CallResult createThreeValues(const Value& first, const Value& second, const Value& third);

        /**
         * @brief Create a CallResult with multiple values from a vector
         * @param values Vector of values to return
         * @return CallResult containing all values
         */
        static CallResult createMultipleValues(const Vec<Value>& values);

        /**
         * @brief Create a success result for pcall-style functions
         * @param result The successful result value
         * @return CallResult with (true, result)
         */
        static CallResult createPCallSuccess(const Value& result);

        /**
         * @brief Create a success result for pcall-style functions with multiple results
         * @param results Vector of successful result values
         * @return CallResult with (true, result1, result2, ...)
         */
        static CallResult createPCallSuccessMultiple(const Vec<Value>& results);

        /**
         * @brief Create an error result for pcall-style functions
         * @param errorMessage The error message
         * @return CallResult with (false, error_message)
         */
        static CallResult createPCallError(const Str& errorMessage);

        /**
         * @brief Create a loadfile-style error result
         * @param errorMessage The error message
         * @return CallResult with (nil, error_message)
         */
        static CallResult createLoadError(const Str& errorMessage);

        /**
         * @brief Create a loadfile-style success result
         * @param loadedFunction The successfully loaded function
         * @return CallResult with (function)
         */
        static CallResult createLoadSuccess(const Value& loadedFunction);

        /**
         * @brief Push multiple values to the state stack (for C function returns)
         * @param state The Lua state
         * @param values The values to push
         * @return Number of values pushed
         */
        static i32 pushMultipleValues(State* state, const Vec<Value>& values);

        /**
         * @brief Convert CallResult to stack-based return for C functions
         * @param state The Lua state
         * @param result The CallResult to convert
         * @return Number of return values (for C function return)
         */
        static i32 returnCallResult(State* state, const CallResult& result);

        /**
         * @brief Helper for math functions that return two values (like modf, frexp)
         * @param first The first return value (usually the main result)
         * @param second The second return value (usually the secondary result)
         * @return CallResult with both values
         */
        static CallResult createMathTwoValues(f64 first, f64 second);

        /**
         * @brief Helper for math functions that return two values (like modf, frexp)
         * @param first The first return value (usually the main result)
         * @param second The second return value (usually an integer)
         * @return CallResult with both values
         */
        static CallResult createMathTwoValues(f64 first, i32 second);

        /**
         * @brief Create a nil result (single nil value)
         * @return CallResult with single nil value
         */
        static CallResult createNilResult();

        /**
         * @brief Create a single value result
         * @param value The single value to return
         * @return CallResult with single value
         */
        static CallResult createSingleValue(const Value& value);

    private:
        // Private constructor - this is a utility class with only static methods
        MultiReturnHelper() = delete;
        MultiReturnHelper(const MultiReturnHelper&) = delete;
        MultiReturnHelper& operator=(const MultiReturnHelper&) = delete;
    };

} // namespace Lua
