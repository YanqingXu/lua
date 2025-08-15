#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <memory>

#ifdef _WIN32
#include <io.h>
#include <stdio.h>
#else
#include <unistd.h>
#endif

#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include "vm/global_state.hpp"
#include "vm/value.hpp"
#include "vm/table.hpp"
#include "vm/function.hpp"
#include "gc/core/gc_string.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "parser/visitor.hpp"
#include "compiler/symbol_table.hpp"
#include "common/defines.hpp"
#include "lib/core/lib_manager.hpp"

using namespace Lua;

// Forward declaration for the REPL function from repl.cpp
void run_repl();

// Lua 5.1 compatible constants
constexpr const char* LUA_PROGNAME = "lua";
constexpr const char* LUA_VERSION_STR = "Lua 5.1.5";
constexpr const char* LUA_COPYRIGHT_STR = "Copyright (C) 1994-2012 Lua.org, PUC-Rio";
constexpr const char* LUA_INIT_ENV = "LUA_INIT";

// Global program name for error reporting
static const char* progname = LUA_PROGNAME;

/**
 * Print usage information following Lua 5.1 format
 */
static void print_usage() {
    std::fprintf(stderr,
        "usage: %s [options] [script [args]].\n"
        "Available options are:\n"
        "  -e stat  execute string 'stat'\n"
        "  -l name  require library 'name'\n"
        "  -i       enter interactive mode after executing 'script'\n"
        "  -v       show version information\n"
        "  --       stop handling options\n"
        "  -        execute stdin and stop handling options\n",
        progname);
    std::fflush(stderr);
}

/**
 * Print version information following Lua 5.1 format
 */
static void print_version() {
    std::printf("%s  %s\n", LUA_VERSION_STR, LUA_COPYRIGHT_STR);
}

/**
 * Print error message with program name
 */
static void l_message(const char* pname, const char* msg) {
    if (pname) {
        std::fprintf(stderr, "%s: ", pname);
    }
    std::fprintf(stderr, "%s\n", msg);
    std::fflush(stderr);
}

/**
 * Read file content with proper error handling
 */
static Str readFile(const Str& path) {
    std::ifstream file(path);
    if (!file) {
        throw LuaException("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Execute string code with error reporting
 */
static i32 dostring(LuaState* L, const char* s, const char* name) {
    try {
        bool success = L->doString(s);
        return success ? 0 : 1;
    } catch (const std::exception& e) {
        l_message(progname, e.what());
        return 1;
    }
}

/**
 * Execute file with error reporting
 */
static i32 dofile(LuaState* L, const char* name) {
    try {
        Str source;
        if (name == nullptr || std::strcmp(name, "-") == 0) {
            // Read from stdin
            std::string line;
            std::stringstream buffer;
            while (std::getline(std::cin, line)) {
                buffer << line << "\n";
            }
            source = buffer.str();
        } else {
            source = readFile(name);
        }

        bool success = L->doString(source);
        return success ? 0 : 1;
    } catch (const std::exception& e) {
        l_message(progname, e.what());
        return 1;
    }
}

/**
 * Require library with error reporting
 */
static i32 dolibrary(LuaState* L, const char* name) {
    try {
        // Simple implementation: try to load as file first
        Str libPath = Str(name) + ".lua";
        return dofile(L, libPath.c_str());
    } catch (const std::exception& e) {
        l_message(progname, e.what());
        return 1;
    }
}

/**
 * Check TTY status (simplified implementation)
 */
static bool lua_stdin_is_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(0) != 0;
#endif
}

/**
 * Handle LUA_INIT environment variable
 */
static i32 handle_luainit(LuaState* L) {
#ifdef _WIN32
    // Use safer _dupenv_s on Windows
    char* init = nullptr;
    size_t len = 0;
    if (_dupenv_s(&init, &len, LUA_INIT_ENV) != 0 || init == nullptr) {
        return 0; // No LUA_INIT or error, success
    }

    i32 result;
    if (init[0] == '@') {
        // Execute file
        result = dofile(L, init + 1);
    } else {
        // Execute string
        result = dostring(L, init, "=LUA_INIT");
    }

    std::free(init); // Free the allocated memory
    return result;
#else
    // Use standard getenv on other platforms
    const char* init = std::getenv(LUA_INIT_ENV);
    if (init == nullptr) {
        return 0; // No LUA_INIT, success
    }

    if (init[0] == '@') {
        // Execute file
        return dofile(L, init + 1);
    } else {
        // Execute string
        return dostring(L, init, "=LUA_INIT");
    }
#endif
}

/**
 * Setup arg global table for script arguments
 */
static void setup_arg_table(LuaState* L, char** argv, i32 argc, i32 script_index) {
    // Create arg table
    auto argTable = make_gc_table();

    // Add all arguments with proper indexing
    for (i32 i = 0; i < argc; ++i) {
        i32 index = i - script_index;
        argTable->set(Value(static_cast<f64>(index)), Value(argv[i]));
    }

    // Set global arg
    auto argStr = GCString::create("arg");
    L->setGlobal(argStr, Value(argTable));
}

/**
 * Parse command line arguments following Lua 5.1 specification
 */
struct CommandLineArgs {
    bool has_i = false;      // Interactive mode
    bool has_v = false;      // Version info
    bool has_e = false;      // Execute string
    i32 script_index = 0;    // Index of script file
    Vec<Str> execute_strings; // Strings to execute with -e
    Vec<Str> libraries;      // Libraries to require with -l
};

/**
 * Check if argument has extra characters (Lua 5.1 compatible)
 */
static bool notail(const char* arg) {
    return arg[2] == '\0';
}

/**
 * Collect and parse command line arguments
 */
static i32 collectargs(char** argv, CommandLineArgs& args) {
    i32 i;
    for (i = 1; argv[i] != nullptr; ++i) {
        if (argv[i][0] != '-') {
            // Not an option, this is the script
            return i;
        }

        switch (argv[i][1]) {
            case '-':
                if (!notail(argv[i])) return -1;
                return (argv[i+1] != nullptr ? i+1 : 0);

            case '\0':
                return i; // Single dash means stdin

            case 'i':
                if (!notail(argv[i])) return -1;
                args.has_i = true;
                break;

            case 'v':
                if (!notail(argv[i])) return -1;
                args.has_v = true;
                break;

            case 'e':
                args.has_e = true;
                if (argv[i][2] == '\0') {
                    ++i;
                    if (argv[i] == nullptr) return -1;
                    args.execute_strings.push_back(argv[i]);
                } else {
                    args.execute_strings.push_back(argv[i] + 2);
                }
                break;

            case 'l':
                if (argv[i][2] == '\0') {
                    ++i;
                    if (argv[i] == nullptr) return -1;
                    args.libraries.push_back(argv[i]);
                } else {
                    args.libraries.push_back(argv[i] + 2);
                }
                break;

            default:
                return -1; // Invalid option
        }
    }
    return 0;
}

/**
 * Main function with Lua 5.1 compatible argument processing
 */
i32 main(i32 argc, char** argv) {
    try {
        // Set program name
        if (argv[0] && argv[0][0]) {
            progname = argv[0];
        }

        // Parse command line arguments
        CommandLineArgs args;
        i32 script = collectargs(argv, args);

        if (script < 0) {
            // Invalid arguments
            print_usage();
            return EXIT_FAILURE;
        }

        // Show version if requested
        if (args.has_v) {
            print_version();
        }

        // Create Lua state with RAII
        auto globalState = std::make_unique<GlobalState>();
        auto luaState = std::make_unique<LuaState>(globalState.get());

        if (!luaState) {
            l_message(progname, "cannot create state: not enough memory");
            return EXIT_FAILURE;
        }

        // Initialize standard libraries
        StandardLibrary::initializeAll(luaState.get());

        // Handle LUA_INIT
        if (handle_luainit(luaState.get()) != 0) {
            return EXIT_FAILURE;
        }

        // Execute -l libraries
        for (const auto& lib : args.libraries) {
            if (dolibrary(luaState.get(), lib.c_str()) != 0) {
                return EXIT_FAILURE;
            }
        }

        // Execute -e strings
        for (const auto& code : args.execute_strings) {
            if (dostring(luaState.get(), code.c_str(), "=(command line)") != 0) {
                return EXIT_FAILURE;
            }
        }

        // Handle script execution
        if (script > 0) {
            // Setup arg table
            setup_arg_table(luaState.get(), argv, argc, script);

            // Execute script
            const char* filename = argv[script];
            if (dofile(luaState.get(), filename) != 0) {
                return EXIT_FAILURE;
            }
        }

        // Enter interactive mode if requested or no script provided
        if (args.has_i || (script == 0 && !args.has_e && !args.has_v)) {
            if (script == 0 && !args.has_e && !args.has_v && lua_stdin_is_tty()) {
                print_version();
            }
            run_repl();
        }

        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        l_message(progname, e.what());
        return EXIT_FAILURE;
    }
}
