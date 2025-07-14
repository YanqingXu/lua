#pragma once

#include "../common/types.hpp"
#include "value.hpp"

namespace Lua {
    /**
     * @brief Structure to handle multiple return values from function calls
     * 
     * This structure encapsulates the result of a function call that may
     * return multiple values, which is a core feature of Lua.
     */
    struct CallResult {
        Vec<Value> values;  ///< The return values
        size_t count;       ///< Number of return values
        
        /**
         * @brief Default constructor - no return values
         */
        CallResult() : count(0) {}
        
        /**
         * @brief Constructor for single return value
         * @param singleValue The single return value
         */
        explicit CallResult(const Value& singleValue) : count(1) {
            values.push_back(singleValue);
        }
        
        /**
         * @brief Constructor for multiple return values
         * @param multipleValues Vector of return values
         */
        CallResult(const Vec<Value>& multipleValues) : values(multipleValues), count(multipleValues.size()) {}
        
        /**
         * @brief Get the first return value (for backward compatibility)
         * @return The first return value, or nil if no values
         */
        Value getFirst() const {
            return count > 0 ? values[0] : Value();
        }
        
        /**
         * @brief Check if there are any return values
         * @return true if there are return values, false otherwise
         */
        bool hasValues() const {
            return count > 0;
        }
        
        /**
         * @brief Get a specific return value by index
         * @param index The index of the return value (0-based)
         * @return The return value at the specified index, or nil if out of bounds
         */
        Value getValue(size_t index) const {
            return index < count ? values[index] : Value();
        }
    };
}
