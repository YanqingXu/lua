#pragma once

#include <iostream>
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../lib/base/base_lib.hpp"
#include "../../lib/lib_base_utils.hpp"

namespace Lua {
namespace Tests {

class StateTest {
public:
    static void runAllTests();
    
private:
    static void testState();
    static void testExecute();
};

} // namespace Tests
} // namespace Lua