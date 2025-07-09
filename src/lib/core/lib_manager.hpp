#pragma once

#include "lib_module.hpp"

namespace Lua {

/**
 * @brief Standard library manager
 *
 * This class provides static methods for initializing the standard
 * libraries in the Lua interpreter. It coordinates the registration
 * and initialization of all library modules.
 *
 * The manager follows a simple approach:
 * 1. Initialize individual libraries through their convenience functions
 * 2. Provide a unified interface for initializing all libraries
 * 3. Handle error cases gracefully with proper logging
 */
class StandardLibrary {
public:
    /**
     * @brief Initialize all standard libraries
     * @param state Lua state to initialize libraries for
     * 
     * This method initializes all available standard libraries:
     * - Base library (print, type, tostring, etc.)
     * - String library (string.len, string.sub, etc.)
     * - Math library (math.abs, math.sin, etc.)
     * 
     * @throws std::invalid_argument if state is null
     */
    static void initializeAll(State* state);
    
    /**
     * @brief Initialize base library only
     * @param state Lua state to initialize base library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeBase(State* state);
    
    /**
     * @brief Initialize string library only
     * @param state Lua state to initialize string library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeString(State* state);
    
    /**
     * @brief Initialize math library only
     * @param state Lua state to initialize math library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeMath(State* state);

    /**
     * @brief Initialize table library only
     * @param state Lua state to initialize table library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeTable(State* state);

    /**
     * @brief Initialize IO library only
     * @param state Lua state to initialize IO library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeIO(State* state);

    /**
     * @brief Initialize OS library only
     * @param state Lua state to initialize OS library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeOS(State* state);

    /**
     * @brief Initialize debug library only
     * @param state Lua state to initialize debug library for
     * @throws std::invalid_argument if state is null
     */
    static void initializeDebug(State* state);
;

private:
    // Private constructor - this is a utility class with only static methods
    StandardLibrary() = delete;
    StandardLibrary(const StandardLibrary&) = delete;
    StandardLibrary& operator=(const StandardLibrary&) = delete;
};

} // namespace Lua
