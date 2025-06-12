#pragma once

#include <iostream>
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../common/types.hpp"

namespace Lua {
namespace Tests {

class ValueTest {
public:
    static void runAllTests();
    
private:
    static void testValues();
};

} // namespace Tests
} // namespace Lua