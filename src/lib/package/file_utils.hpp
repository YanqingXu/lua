#pragma once

#include "../../common/types.hpp"
#include <string>
#include <vector>

namespace Lua {

/**
 * @brief File system utilities for package library
 *
 * Provides cross-platform file operations needed for module loading:
 * - File existence checking
 * - File content reading
 * - Path manipulation and joining
 * - Module search path expansion
 * - Directory and filename extraction
 *
 * This implementation aims for cross-platform compatibility while
 * following Lua 5.1 module search conventions.
 */
class FileUtils {
public:
    // ===================================================================
    // File Operations
    // ===================================================================

    /**
     * @brief Check if file exists and is readable
     * @param path File path to check
     * @return true if file exists and is readable
     */
    static bool fileExists(const Str& path);

    /**
     * @brief Read entire file content as string
     * @param path File path to read
     * @return File content as string
     * @throws std::runtime_error if file cannot be read
     */
    static Str readFile(const Str& path);

    /**
     * @brief Check if path is a directory
     * @param path Path to check
     * @return true if path is a directory
     */
    static bool isDirectory(const Str& path);

    // ===================================================================
    // Path Manipulation
    // ===================================================================

    /**
     * @brief Join two path components
     * @param dir Directory path
     * @param file File or subdirectory name
     * @return Joined path with appropriate separator
     */
    static Str joinPath(const Str& dir, const Str& file);

    /**
     * @brief Get directory part of path
     * @param path Full file path
     * @return Directory part (without trailing separator)
     */
    static Str getDirectory(const Str& path);

    /**
     * @brief Get filename part of path
     * @param path Full file path
     * @return Filename part (without directory)
     */
    static Str getFilename(const Str& path);

    /**
     * @brief Get file extension
     * @param path File path
     * @return File extension including dot (e.g., ".lua")
     */
    static Str getExtension(const Str& path);

    /**
     * @brief Remove file extension from path
     * @param path File path
     * @return Path without extension
     */
    static Str removeExtension(const Str& path);

    /**
     * @brief Normalize path separators for current platform
     * @param path Path to normalize
     * @return Normalized path
     */
    static Str normalizePath(const Str& path);

    // ===================================================================
    // Module Search Operations
    // ===================================================================

    /**
     * @brief Expand search path pattern with module name
     * @param pattern Search path pattern (e.g., "./?.lua")
     * @param modname Module name (e.g., "mymodule")
     * @return Expanded file path
     */
    static Str expandSearchPattern(const Str& pattern, const Str& modname);

    /**
     * @brief Find module file using search path
     * @param modname Module name
     * @param searchPath Semicolon-separated search paths
     * @param sep Module name separator (default: ".")
     * @param rep Path separator replacement (default: "/")
     * @return First existing file path, or empty string if not found
     */
    static Str findModuleFile(const Str& modname, const Str& searchPath,
                             const Str& sep = ".", const Str& rep = "/");

    /**
     * @brief Find module file using search path and collect attempted paths
     * @param modname Module name
     * @param searchPath Semicolon-separated search paths
     * @param sep Module name separator (default: ".")
     * @param rep Path separator replacement (default: "/")
     * @param attemptedPaths Output vector to collect attempted file paths
     * @return First existing file path, or empty string if not found
     */
    static Str findModuleFileWithPaths(const Str& modname, const Str& searchPath,
                                      const Str& sep, const Str& rep,
                                      Vec<Str>& attemptedPaths);

    /**
     * @brief Split search path string into individual patterns
     * @param searchPath Semicolon-separated search paths
     * @return Vector of individual search patterns
     */
    static Vec<Str> splitSearchPath(const Str& searchPath);

    /**
     * @brief Convert module name to file path
     * @param modname Module name with dots (e.g., "foo.bar")
     * @param sep Module name separator (default: ".")
     * @param rep Path separator replacement (default: "/")
     * @return File path (e.g., "foo/bar")
     */
    static Str moduleNameToPath(const Str& modname, const Str& sep = ".", const Str& rep = "/");

    // ===================================================================
    // Platform-Specific Utilities
    // ===================================================================

    /**
     * @brief Get platform-specific path separator
     * @return Path separator character ('/' on Unix, '\\' on Windows)
     */
    static char getPathSeparator();

    /**
     * @brief Get platform-specific search path separator
     * @return Search path separator character (';' on Windows, ':' on Unix)
     */
    static char getSearchPathSeparator();

    /**
     * @brief Check if path is absolute
     * @param path Path to check
     * @return true if path is absolute
     */
    static bool isAbsolutePath(const Str& path);

    /**
     * @brief Get current working directory
     * @return Current working directory path
     * @throws std::runtime_error if cannot get current directory
     */
    static Str getCurrentDirectory();

    // ===================================================================
    // Error Handling
    // ===================================================================

    /**
     * @brief Get last file system error message
     * @return Error message string
     */
    static Str getLastError();

private:
    // ===================================================================
    // Internal Helper Functions
    // ===================================================================

    /**
     * @brief Replace all occurrences of substring in string
     * @param str String to modify
     * @param from Substring to replace
     * @param to Replacement substring
     * @return Modified string
     */
    static Str replaceAll(const Str& str, const Str& from, const Str& to);

    /**
     * @brief Split string by delimiter
     * @param str String to split
     * @param delimiter Delimiter character
     * @return Vector of split parts
     */
    static Vec<Str> split(const Str& str, char delimiter);

    /**
     * @brief Trim whitespace from string
     * @param str String to trim
     * @return Trimmed string
     */
    static Str trim(const Str& str);
};

} // namespace Lua
