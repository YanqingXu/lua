#include "lib_manager.hpp"
#include "../base/base_lib.hpp"
//#include "../string/string_lib.hpp"
//#include "../math/math_lib.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// StandardLibrary Implementation
// ===================================================================

void StandardLibrary::initializeAll(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeAll" << std::endl;
        return;
    }

    std::cout << "[StandardLibrary] Initializing all standard libraries..." << std::endl;

    initializeBase(state);
    initializeString(state);
    initializeMath(state);

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

    //initializeStringLib(state);
}

void StandardLibrary::initializeMath(State* state) {
    if (!state) {
        std::cerr << "Error: State is null in initializeMath" << std::endl;
        return;
    }

    //initializeMathLib(state);
}

} // namespace Lua