#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../parser/parser.hpp"
#include "../parser/visitor.hpp"

namespace Lua {
namespace Tests {

void testParser();
void testStatements();
void testWhileLoop();
void testASTVisitor();

} // namespace Tests
} // namespace Lua