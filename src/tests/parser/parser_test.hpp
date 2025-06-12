#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../../parser/parser.hpp"
#include "../../parser/visitor.hpp"

namespace Lua {
namespace Tests {

class ParserTest {
public:
    static void runAllTests();
    
private:
    static void testParser();
    static void testStatements();
    static void testWhileLoop();
    static void testASTVisitor();
};

} // namespace Tests
} // namespace Lua