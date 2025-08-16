/**
 * @file repl.cpp
 * @brief Lua 5.1 compatible REPL implementation
 *
 * This file implements a REPL (Read-Eval-Print Loop) that closely follows
 * the official Lua 5.1 implementation, providing maximum compatibility
 * while leveraging modern C++ features for better maintainability.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <csignal>
#include <cstdlib>
#include <cstring>

#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include "vm/global_state.hpp"
#include "compiler/compiler.hpp"
#include "lib/core/lib_manager.hpp"
#include "gc/core/gc_string.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

namespace Lua {

// Lua 5.1 compatible constants
constexpr const char* LUA_PROMPT = "> ";
constexpr const char* LUA_PROMPT2 = ">> ";
constexpr i32 LUA_MAXINPUT = 512;

// Global state for signal handling (Lua 5.1 compatible)
static LuaState* globalL = nullptr;
static volatile bool g_interrupted = false;

/**
 * Signal handler for SIGINT (Ctrl+C) following Lua 5.1 behavior
 */
static void laction(i32 signal) {
    std::signal(signal, SIG_DFL); // Reset to default if another SIGINT occurs
    if (globalL) {
        // In a full implementation, this would set a debug hook
        // For now, we'll use a simple flag
        g_interrupted = true;
    }
}

/**
 * REPL-specific signal handler
 */
static void signalHandler(i32 signal) {
    if (signal == SIGINT) {
        g_interrupted = true;
        std::cout << "\n";
    }
}

/**
 * REPL exit function for compatibility
 */
static Value replExit(LuaState* state, i32 nargs) {
    i32 exitCode = 0;
    if (nargs > 0) {
        Value arg = state->get(-nargs);
        if (arg.isNumber()) {
            exitCode = static_cast<i32>(arg.asNumber());
        }
    }
    std::exit(exitCode);
    return Value(); // never reached
}

/**
 * Get prompt string from Lua state or use default
 * Following Lua 5.1 get_prompt implementation
 */
static const char* get_prompt(LuaState* L, bool firstline) {
    try {
        auto promptName = GCString::create(firstline ? "_PROMPT" : "_PROMPT2");
        Value promptVal = L->getGlobal(promptName);
        if (promptVal.isString()) {
            // Note: In a full implementation, we'd need to manage string lifetime
            static Str cachedPrompt;
            cachedPrompt = promptVal.toString();
            return cachedPrompt.c_str();
        }
    } catch (...) {
        // Ignore errors when getting prompts
    }
    return firstline ? LUA_PROMPT : LUA_PROMPT2;
}

/**
 * Check if parsing error indicates incomplete input (Lua 5.1 method)
 * This replaces the complex custom detector with the official approach
 */
static bool incomplete(LuaState* L, const Str& code) {
    try {
        Parser parser(code);
        auto statements = parser.parse();

        if (parser.hasError()) {
            Str errorMsg = parser.getFormattedErrors();
            // Check if error message contains "<eof>" indicating incomplete input
            return errorMsg.find("<eof>") != Str::npos;
        }
        return false; // No error, input is complete
    } catch (const std::exception& e) {
        Str errorMsg = e.what();
        // Check for EOF-related error messages
        return errorMsg.find("<eof>") != Str::npos ||
               errorMsg.find("unexpected end") != Str::npos ||
               errorMsg.find("unfinished") != Str::npos;
    }
}

/**
 * Read a line with proper prompt handling (Lua 5.1 pushline equivalent)
 */
static bool pushline(LuaState* L, Str& buffer, bool firstline) {
    char input[LUA_MAXINPUT];
    const char* prompt = get_prompt(L, firstline);

    std::cout << prompt;
    std::cout.flush();

    if (!std::cin.getline(input, LUA_MAXINPUT)) {
        return false; // EOF or error
    }

    // Handle =expression syntax sugar (Lua 5.1 feature)
    if (firstline && input[0] == '=') {
        buffer = "return " + Str(input + 1);
    } else {
        buffer = input;
    }

    return true;
}

/**
 * Load a complete line of input (Lua 5.1 loadline equivalent)
 */
static bool loadline(LuaState* L, Str& code) {
    Str line;
    if (!pushline(L, line, true)) {
        return false; // EOF
    }

    code = line;

    // Keep reading lines until input is complete
    while (incomplete(L, code)) {
        if (!pushline(L, line, false)) {
            return false; // EOF during continuation
        }
        code += "\n" + line;
    }

    return true;
}
/**
 * Execute code with proper error handling and result display (Lua 5.1 docall equivalent)
 */
static i32 docall(LuaState* L, const Str& code) {
    try {
        // Set up signal handling during execution
        std::signal(SIGINT, laction);

        bool success = L->doString(code);

        // Restore signal handling
        std::signal(SIGINT, SIG_DFL);

        return success ? 0 : 1;
    } catch (const std::exception& e) {
        std::signal(SIGINT, SIG_DFL);
        std::cerr << "lua: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * Print any results left on the stack (Lua 5.1 behavior)
 */
static void print_results(LuaState* L) {
    // In Lua 5.1, if there are results on the stack after execution,
    // they are printed using the global print function
    // This is a simplified implementation
    try {
        // Check if there are values on the stack
        // In a full implementation, we'd check lua_gettop(L) > 0
        // For now, we'll skip this as our VM doesn't expose stack top directly
    } catch (...) {
        // Ignore errors in result printing
    }
}

/**
 * Main REPL loop following Lua 5.1 dotty implementation
 */
static void dotty(LuaState* L) {
    globalL = L; // Set global state for signal handling

    Str code;
    while (loadline(L, code)) {
        if (g_interrupted) {
            g_interrupted = false;
            std::cout << "\n";
            continue;
        }

        i32 status = docall(L, code);
        if (status == 0) {
            print_results(L);
        }

        // Reset interrupted flag
        g_interrupted = false;
    }

    std::cout << "\n";
    std::cout.flush();
    globalL = nullptr;
}

/**
 * Initialize REPL state with Lua 5.1 compatible settings
 */
static void initializeREPL(LuaState& state) {
    // Set default prompts (Lua 5.1 compatible)
    auto promptStr = GCString::create("_PROMPT");
    auto prompt2Str = GCString::create("_PROMPT2");
    auto versionStr = GCString::create("_VERSION");
    auto exitStr = GCString::create("exit");

    state.setGlobal(promptStr, Value(GCString::create(LUA_PROMPT)));
    state.setGlobal(prompt2Str, Value(GCString::create(LUA_PROMPT2)));

    // Set version info
    state.setGlobal(versionStr, Value(GCString::create("Lua 5.1.5")));

    // Add REPL specific global functions
    auto exitFunc = Function::createNativeLegacy(replExit);
    state.setGlobal(exitStr, Value(exitFunc));

    // Set signal handling
    std::signal(SIGINT, signalHandler);
}

} // namespace Lua

/**
 * Main REPL entry point called from main.cpp
 * This function is designed to be called when no script is provided
 * and implements the Lua 5.1 interactive mode
 */
void run_repl() {
    using namespace Lua;

    // Create Lua state with RAII
    auto globalState = std::make_unique<GlobalState>();
    auto state = std::make_unique<LuaState>(globalState.get());

    if (!state) {
        std::cerr << "lua: cannot create state: not enough memory" << std::endl;
        return;
    }

    // Initialize standard libraries
    StandardLibrary::initializeAll(state.get());
    initializeREPL(*state);

    // Print version banner (Lua 5.1 compatible)
    std::cout << "Lua 5.1.5  Copyright (C) 1994-2012 Lua.org, PUC-Rio" << std::endl;

    // Enter the main REPL loop
    dotty(state.get());
}

