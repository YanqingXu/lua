#pragma once

#include <iostream>
#include <string>
#include "../../lexer/lexer.hpp"

namespace Lua {
namespace Tests {

class LexerTest {
public:
    static void runAllTests();
    
private:
    static void testLexer(const std::string& source);
};

} // namespace Tests
} // namespace Lua