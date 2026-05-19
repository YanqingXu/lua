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

#include <stdexcept>
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

    if (!base[cb].isFunction()) {
        throw std::runtime_error("VM: TFORLOOP requires function at R("
                                 + std::to_string(cb) + ")");
    }

    Function* func = base[cb].asFunction();

    if (func->isCFunction()) {
        ci.savedpc = &proto->getCode()[pc];

        bool isLua = precall(L, cb, 2, c);
        if (isLua) {
            throw std::runtime_error("VM: TFORLOOP Lua iterators not supported yet");
        }

        stack.setTop(ci.top);
        L->setAbsoluteTop(ci.top);
        base = refreshBase(L);
    } else {
        throw std::runtime_error("VM: TFORLOOP Lua iterators not supported yet");
    }

    cb = a + 3;
    if (!base[cb].isNil()) {
        base[a + 2] = base[cb];
        if (pc < proto->getCode().size()) {
            pc += GETARG_sBx(proto->getCode()[pc]) + 1;
        }
    } else {
        pc++;
    }
}

}  // namespace VM::detail
}  // namespace Lua
