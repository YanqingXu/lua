#include "lib_manager.hpp"
#include "lib/base/base_lib.hpp"
#include "lib/string/string_lib.hpp"
#include "lib/math/math_lib.hpp"
#include "lib/table/table_lib.hpp"
#include "lib/io/io_lib.hpp"
#include "lib/os/os_lib.hpp"
#include "lib/debug/debug_lib.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// StandardLibrary Implementation
// ===================================================================

void StandardLibrary::initializeAll(State* state) {
    std::cout << "[DEBUG] StandardLibrary::initializeAll called" << std::endl;

    if (!state) {
        std::cerr << "[ERROR] StandardLibrary::initializeAll: State is null!" << std::endl;
        return;
    }

    std::cout << "[StandardLibrary] Initializing all standard libraries..." << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing Base library" << std::endl;
    initializeBase(state);
    std::cout << "[DEBUG] StandardLibrary: Base library initialized" << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing String library" << std::endl;
    initializeString(state);
    std::cout << "[DEBUG] StandardLibrary: String library initialized" << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing Math library" << std::endl;
    initializeMath(state);
    std::cout << "[DEBUG] StandardLibrary: Math library initialized" << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing Table library" << std::endl;
    initializeTable(state);
    std::cout << "[DEBUG] StandardLibrary: Table library initialized" << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing IO library" << std::endl;
    initializeIO(state);
    std::cout << "[DEBUG] StandardLibrary: IO library initialized" << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing OS library" << std::endl;
    initializeOS(state);
    std::cout << "[DEBUG] StandardLibrary: OS library initialized" << std::endl;

    std::cout << "[DEBUG] StandardLibrary: Initializing Debug library" << std::endl;
    initializeDebug(state);
    std::cout << "[DEBUG] StandardLibrary: Debug library initialized" << std::endl;

    std::cout << "[StandardLibrary] All standard libraries initialized successfully!" << std::endl;
}

void StandardLibrary::initializeBase(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeBase" << std::endl;
        return;
    }

    initializeBaseLib(state);
}

void StandardLibrary::initializeString(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeString" << std::endl;
        return;
    }

    initializeStringLib(state);
}

void StandardLibrary::initializeMath(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeMath" << std::endl;
        return;
    }

    initializeMathLib(state);
}

void StandardLibrary::initializeTable(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeTable" << std::endl;
        return;
    }

    initializeTableLib(state);
}

void StandardLibrary::initializeIO(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeIO" << std::endl;
        return;
    }

    initializeIOLib(state);
}

void StandardLibrary::initializeOS(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeOS" << std::endl;
        return;
    }

    initializeOSLib(state);
}

void StandardLibrary::initializeDebug(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeDebug" << std::endl;
        return;
    }

    initializeDebugLib(state);
}

} // namespace Lua