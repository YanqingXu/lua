#pragma once

#include "../core/lib_define.hpp"
#include "../core/lib_func_registry.hpp"
#include "../core/lib_context.hpp"
#include "../core/lib_module.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"

namespace Lua {

// Use library framework types
using namespace Lib;

/**
 * Unified Base Library implementation
 * Contains all Lua 5.1 standard base functions
 * 
 * Design principles:
 * 1. Single implementation path - each function has only one implementation
 * 2. Unified interface specification - uses standard parameter checking and error handling
 * 3. Clear dependency relationships - depends only on necessary modules
 * 4. High cohesion, low coupling - related functionality centralized, reduced external dependencies
 */
class BaseLib : public LibModule {
public:
    // === LibModule Interface Implementation ===
    
    /**
     * Get module name
     */
    StrView getName() const noexcept override { return "base"; }
    
    /**
     * Get module version
     */
    StrView getVersion() const noexcept override { return "1.0.0"; }
    
    /**
     * Register functions to registry
     */
    void registerFunctions(LibFuncRegistry& registry, const LibContext& context) override;
    
    /**
     * Initialize module
     */
    void initialize(State* state, const LibContext& context) override;
    
    /**
     * Cleanup module resources
     */
    void cleanup(State* state, const LibContext& context) override;

public:
    // === Core Base Functions ===
    
    /**
     * print(...) - Output parameters to standard output
     * @param args Variable argument list
     * @return nil
     */
    LUA_FUNCTION(print);
    
    /**
     * type(v) - Get the type of a value
     * @param v Any value
     * @return Type name string
     */
    LUA_FUNCTION(type);
    
    /**
     * tostring(v) - Convert value to string
     * @param v Any value
     * @return String representation
     */
    LUA_FUNCTION(tostring);
    
    /**
     * tonumber(e [, base]) - Convert value to number
     * @param e Value to convert
     * @param base Base for conversion (optional, default 10)
     * @return Number or nil (if conversion fails)
     */
    LUA_FUNCTION(tonumber);
    
    /**
     * error(message [, level]) - Throw an error
     * @param message Error message
     * @param level Error level (optional)
     * @return Does not return (throws exception)
     */
    LUA_FUNCTION(error);
    
    /**
     * assert(v [, message]) - Assert check
     * @param v Value to check
     * @param message Error message (optional)
     * @return v (if true)
     */
    LUA_FUNCTION(assert_func);

    // === Table Operation Functions ===
    
    /**
     * pairs(t) - Return iterator for table traversal
     * @param t Table to traverse
     * @return Iterator function, table, initial key
     */
    LUA_FUNCTION(pairs);
    
    /**
     * ipairs(t) - Return iterator for array traversal
     * @param t Array to traverse
     * @return Iterator function, table, initial index
     */
    LUA_FUNCTION(ipairs);
    
    /**
     * next(table [, index]) - Get next element in table
     * @param table Table to traverse
     * @param index Current index (optional)
     * @return Next key, next value
     */
    LUA_FUNCTION(next);
    
    /**
     * getmetatable(object) - Get metatable of object
     * @param object Object to get metatable from
     * @return Metatable or nil
     */
    LUA_FUNCTION(getmetatable);
    
    /**
     * setmetatable(table, metatable) - Set metatable for table
     * @param table Table to set metatable for
     * @param metatable Metatable to set
     * @return The table
     */
    LUA_FUNCTION(setmetatable);

    // === Raw Operation Functions ===
    
    /**
     * rawget(table, index) - Raw get operation
     * @param table Table to access
     * @param index Index to get
     * @return Value at index
     */
    LUA_FUNCTION(rawget);
    
    /**
     * rawset(table, index, value) - Raw set operation
     * @param table Table to modify
     * @param index Index to set
     * @param value Value to set
     * @return The table
     */
    LUA_FUNCTION(rawset);
    
    /**
     * rawlen(v) - Raw length operation
     * @param v Object to get length of
     * @return Length of object
     */
    LUA_FUNCTION(rawlen);
    
    /**
     * rawequal(v1, v2) - Raw equality comparison
     * @param v1 First value
     * @param v2 Second value
     * @return true if equal, false otherwise
     */
    LUA_FUNCTION(rawequal);

    // === Control Flow Functions ===
    
    /**
     * pcall(f [, arg1, ...]) - Protected call
     * @param f Function to call
     * @param args Arguments to pass
     * @return success flag, result or error message
     */
    LUA_FUNCTION(pcall);
    
    /**
     * xpcall(f, err [, arg1, ...]) - Extended protected call
     * @param f Function to call
     * @param err Error handler function
     * @param args Arguments to pass
     * @return success flag, result or error message
     */
    LUA_FUNCTION(xpcall);
    
    /**
     * select(index, ...) - Select arguments
     * @param index Index to select from (or "#" for count)
     * @param args Variable arguments
     * @return Selected arguments
     */
    LUA_FUNCTION(select);
    
    /**
     * unpack(list [, i [, j]]) - Unpack table
     * @param list Table to unpack
     * @param i Start index (optional, default 1)
     * @param j End index (optional, default #list)
     * @return Unpacked values
     */
    LUA_FUNCTION(unpack);

    // === Loading Functions ===
    
    /**
     * load(func [, chunkname]) - Load code chunk
     * @param func Function or string containing code
     * @param chunkname Name of chunk (optional)
     * @return Compiled function or nil, error message
     */
    LUA_FUNCTION(load);
    
    /**
     * loadstring(string [, chunkname]) - Load string code
     * @param string String containing code
     * @param chunkname Name of chunk (optional)
     * @return Compiled function or nil, error message
     */
    LUA_FUNCTION(loadstring);
    
    /**
     * dofile([filename]) - Execute file
     * @param filename File to execute (optional, default stdin)
     * @return Return values from file
     */
    LUA_FUNCTION(dofile);
    
    /**
     * loadfile([filename]) - Load file
     * @param filename File to load (optional, default stdin)
     * @return Compiled function or nil, error message
     */
    LUA_FUNCTION(loadfile);

private:
    // Internal state
    bool initialized_ = false;
};

/**
 * Factory function to create BaseLib instance
 * @return Unique pointer to BaseLib instance
 */
std::unique_ptr<LibModule> createBaseLib();

} // namespace Lua
