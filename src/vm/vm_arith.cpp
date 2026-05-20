/**
 * @file vm_arith.cpp
 * @brief VM arithmetic opcode execution helpers.
 */

#include "vm/vm_internal.hpp"

#include "core/function.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm_constants.hpp"

namespace Lua {
namespace {

Value* refreshBase(LuaState* L) {
    return &L->getStack()[L->getCurrentCallInfo().base];
}

Value getRK(Proto* proto, Value* base, i32 rk) {
    if (ISK(rk)) {
        return proto->getConstant(INDEXK(rk));
    }
    return base[rk];
}

}  // namespace

namespace VM::detail {

void execArithmetic(LuaState* L, Proto* proto, Value*& base, i32 a, i32 b, i32 c, OpCode op) {
    Value left = getRK(proto, base, b);
    Value right = getRK(proto, base, c);
    Value result;
    arith(L, result, left, right, op);
    base = refreshBase(L);
    base[a] = result;
}

}  // namespace VM::detail
}  // namespace Lua
