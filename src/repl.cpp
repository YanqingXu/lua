#include <iostream>
#include <string>
#include <sstream>

#include "common/types.hpp"
#include "vm/state.hpp"
#include "lib/core/lib_manager.hpp"

// This function will be called from main.cpp
void run_repl() {
    Lua::State state;
    Lua::StandardLibrary::initializeAll(&state);

    std::cout << "Lua REPL. Type 'exit' to quit." << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line) || line == "exit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        try {
            state.doString(line);
        } catch (const Lua::LuaException& e) {
            // Catch specific Lua exceptions
            std::cerr << "Error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            // Catch other standard exceptions
            std::cerr << "System Error: " << e.what() << std::endl;
        }
    }
}