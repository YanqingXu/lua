#pragma once

#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include "vm/value.hpp"
#include "lib/core/lib_module.hpp"
#include <fstream>
#include <memory>

namespace Lua {

/**
 * @brief IO library implementation
 *
 * Provides Lua file I/O functions:
 * - open: Open file with specified mode
 * - close: Close file handle
 * - read: Read from file or stdin
 * - write: Write to file or stdout
 * - flush: Flush output buffer
 * - lines: Iterator over file lines
 * - input: Set default input file
 * - output: Set default output file
 * - type: Get file handle type
 *
 * This implementation follows the simplified framework design
 * for better performance and maintainability.
 */
class IOLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "io"; }

    /**
     * @brief Register IO library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(LuaState* state) override;

    /**
     * @brief Initialize the IO library
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(LuaState* state) override;

    // File operation function declarations with proper documentation
    /**
     * @brief Open file function
     * @param state Lua state containing filename and mode
     * @param nargs Number of arguments (1 or 2)
     * @return File handle or nil on error
     * @throws std::invalid_argument if state is null
     */
    static Value open(LuaState* state, i32 nargs);

    /**
     * @brief Close file function
     * @param state Lua state containing file handle
     * @param nargs Number of arguments (0 or 1)
     * @return True on success, nil on error
     * @throws std::invalid_argument if state is null
     */
    static Value close(LuaState* state, i32 nargs);

    /**
     * @brief Read from file or stdin
     * @param state Lua state containing file handle and format
     * @param nargs Number of arguments (0 to 2)
     * @return String data or nil on EOF/error
     * @throws std::invalid_argument if state is null
     */
    static Value read(LuaState* state, i32 nargs);

    /**
     * @brief Write to file or stdout
     * @param state Lua state containing file handle and data
     * @param nargs Number of arguments (1 or more)
     * @return File handle or nil on error
     * @throws std::invalid_argument if state is null
     */
    static Value write(LuaState* state, i32 nargs);

    /**
     * @brief Flush output buffer
     * @param state Lua state containing optional file handle
     * @param nargs Number of arguments (0 or 1)
     * @return True on success, nil on error
     * @throws std::invalid_argument if state is null
     */
    static Value flush(LuaState* state, i32 nargs);

    /**
     * @brief Get file lines iterator
     * @param state Lua state containing filename or file handle
     * @param nargs Number of arguments (0 or 1)
     * @return Iterator function
     * @throws std::invalid_argument if state is null
     */
    static Value lines(LuaState* state, i32 nargs);

    /**
     * @brief Set default input file
     * @param state Lua state containing filename or file handle
     * @param nargs Number of arguments (0 or 1)
     * @return Previous input file or nil
     * @throws std::invalid_argument if state is null
     */
    static Value input(LuaState* state, i32 nargs);

    /**
     * @brief Set default output file
     * @param state Lua state containing filename or file handle
     * @param nargs Number of arguments (0 or 1)
     * @return Previous output file or nil
     * @throws std::invalid_argument if state is null
     */
    static Value output(LuaState* state, i32 nargs);

    /**
     * @brief Get file handle type
     * @param state Lua state containing file handle
     * @param nargs Number of arguments (should be 1)
     * @return String describing file type or nil
     * @throws std::invalid_argument if state is null
     */
    static Value type(LuaState* state, i32 nargs);

private:
    /**
     * @brief File handle wrapper for Lua
     */
    struct FileHandle {
        std::unique_ptr<std::fstream> file;
        Str filename;
        Str mode;
        bool isStdio;
        
        FileHandle(const Str& fname, const Str& fmode);
        FileHandle(bool stdio); // For stdin/stdout/stderr
        ~FileHandle() = default;
        
        bool isOpen() const;
        void close();
    };

    /**
     * @brief Validate file handle argument
     * @param state Lua state
     * @param argIndex Argument index (1-based)
     * @return Pointer to FileHandle or nullptr if invalid
     */
    static FileHandle* validateFileHandle(LuaState* state, i32 argIndex);

    /**
     * @brief Create file handle userdata
     * @param state Lua state
     * @param filename File name
     * @param mode File mode
     * @return Value containing file handle userdata
     */
    static Value createFileHandle(LuaState* state, const Str& filename, const Str& mode);

    /**
     * @brief Parse file mode string
     * @param mode Mode string (e.g., "r", "w", "a", "r+", etc.)
     * @return std::ios_base::openmode flags
     */
    static std::ios_base::openmode parseMode(const Str& mode);

    /**
     * @brief Read line from file
     * @param file File stream
     * @return Line string or empty if EOF
     */
    static Str readLine(std::fstream& file);

    /**
     * @brief Read all content from file
     * @param file File stream
     * @return All content as string
     */
    static Str readAll(std::fstream& file);

    /**
     * @brief Read specified number of characters
     * @param file File stream
     * @param count Number of characters to read
     * @return String with read characters
     */
    static Str readChars(std::fstream& file, i32 count);

    // Default file handles
    static FileHandle* defaultInput;
    static FileHandle* defaultOutput;
};

/**
 * @brief Convenient IOLib initialization function
 * @param state Lua state
 */
void initializeIOLib(LuaState* state);

} // namespace Lua
