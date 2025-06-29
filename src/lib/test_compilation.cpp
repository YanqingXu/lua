#include "modern_lib_framework.hpp"
#include <iostream>

// Simple compilation test without using incomplete types
int main() {
    try {
        // Test framework compilation
        Lua::Lib::FunctionRegistry registry;

        // Test function metadata
        Lua::Lib::FunctionMetadata meta("test");
        meta.withDescription("Test function").withArgs(1, 2);

        // Test library context
        auto context = std::make_shared<Lua::Lib::LibraryContext>();
        context->setConfig("debug", true);

        std::cout << "Basic framework compilation test successful!" << std::endl;
        std::cout << "Function registry size: " << registry.size() << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
