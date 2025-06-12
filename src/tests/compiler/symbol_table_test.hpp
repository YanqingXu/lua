#pragma once

#include <iostream>
#include "../../compiler/symbol_table.hpp"

namespace Lua {
namespace Tests {

class SymbolTableTest {
public:
    static void runAllTests();
    
private:
    static void testSymbolTable();
};

} // namespace Tests
} // namespace Lua