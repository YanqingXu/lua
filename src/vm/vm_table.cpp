/**
 * @file vm_table.cpp
 * @brief VM table-construction helpers.
 */

#include "vm/vm_internal.hpp"

#include "core/table.hpp"
#include "vm/call_info.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm_constants.hpp"

#include <stdexcept>

namespace Lua::VM::detail {

void setList(LuaState* L, Value* base, i32 a, i32 b, i32 c) {
    if (!base[a].isTable()) {
        throw std::runtime_error("VM: SETLIST requires table");
    }

    Table* table = base[a].asTable();
    i32 n = b;

    if (n == 0) {
        CallInfo& ci = L->getCurrentCallInfo();
        usize ra = ci.base + static_cast<usize>(a);
        n = static_cast<i32>(L->getAbsoluteTop() - ra) - 1;
        L->setAbsoluteTop(ci.top);
    }

    i32 baseIndex = (c - 1) * FIELDS_PER_FLUSH;
    for (i32 i = 1; i <= n; i++) {
        table->setArray(baseIndex + i, base[a + i]);
    }
}

}  // namespace Lua::VM::detail
