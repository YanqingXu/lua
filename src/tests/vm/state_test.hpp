#pragma once

#include <iostream>
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../lib/base_lib.hpp"

namespace Lua {
namespace Tests {

void testState();
void testExecute();

} // namespace Tests
} // namespace Lua