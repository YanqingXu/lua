/**
 * @file vm_loop.cpp
 * @brief VM loop helper operations.
 */

#include "vm/vm_internal.hpp"

#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "vm/call_info.hpp"
#include "vm/lua_state.hpp"
#include "vm/stack.hpp"
#include "vm/vm.hpp"

#include <string>

namespace Lua {
namespace {

Value* refreshBase(LuaState* L) {
    return &L->getStack()[L->getCurrentCallInfo().base];
}

}  // namespace

namespace VM::detail {

void tforLoop(LuaState* L, Value*& base, Proto* proto, usize& pc, i32 a, i32 c) {
    i32 cb = a + 3;
    CallInfo& ci = L->getCurrentCallInfo();
    Stack& stack = L->getStack();
    usize requiredSize = ci.base + cb + 3 + c;
    while (stack.size() < requiredSize) stack.push(Value());
    base = &stack[ci.base];

    base[cb + 2] = base[a + 2];
    base[cb + 1] = base[a + 1];
    base[cb]     = base[a];

    const auto code = proto->getInstructionSpan();
    if (pc < code.size()) {
        ci.savedpc = code.data() + pc;
    }

    usize callTop = ci.base + static_cast<usize>(cb + 3);
    stack.setTop(callTop);
    L->setAbsoluteTop(callTop);

    VM::call(L, 2, c);

    CallInfo& callerCI = L->getCurrentCallInfo();
    stack.setTop(callerCI.top);
    L->setAbsoluteTop(callerCI.top);
    base = refreshBase(L);

    cb = a + 3;
    if (!base[cb].isNil()) {
        base[a + 2] = base[cb];
        if (pc < code.size()) {
            pc += GETARG_sBx(code[pc]) + 1;
        }
    } else {
        pc++;
    }
}

}  // namespace VM::detail
}  // namespace Lua
