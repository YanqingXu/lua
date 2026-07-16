/**
 * @file vm_ops.cpp
 * @brief VM table, arithmetic, comparison, unary, and concat helpers.
 */

#include "vm/vm_internal.hpp"

#include "common/lua_error.hpp"
#include "common/config.hpp"
#include "common/number_conversion.hpp"
#include "core/gc_string.hpp"
#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_constants.hpp"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace Lua {
namespace {

bool tryToNumber(const Value& val, f64& result) {
    if (val.isNumber()) {
        result = val.asNumber();
        return true;
    }
    if (val.isString()) {
        GCString* str = val.asString();
        return luaStringToNumber(str->view(), result);
    }
    return false;
}

struct ConcatOperandText {
    std::array<char, 64> numberBuffer{};
    StrView view;
};

bool concatOperandText(const Value& value, ConcatOperandText& text) {
    if (value.isString()) {
        text.view = value.asString()->view();
        return true;
    }
    if (!value.isNumber()) {
        return false;
    }

    text.view = luaNumberToView(value.asNumber(), text.numberBuffer);
    return true;
}

f64 luaModulo(f64 left, f64 right) {
    return left - std::floor(left / right) * right;
}

void checkStringConcatLength(usize left, usize right) {
    if (left > LUA_MAX_STRING_LENGTH || right > LUA_MAX_STRING_LENGTH || left > LUA_MAX_STRING_LENGTH - right) {
        throw RuntimeError("string length overflow");
    }
}

int luaStringCompare(const GCString* left, const GCString* right) {
    const char* l = left->c_str();
    const char* r = right->c_str();
    usize ll = left->getLength();
    usize lr = right->getLength();

    for (;;) {
        int cmp = std::strcoll(l, r);
        if (cmp != 0) {
            return cmp;
        }

        usize len = std::strlen(l);
        if (len == lr) {
            return (len == ll) ? 0 : 1;
        }
        if (len == ll) {
            return -1;
        }

        len++;
        l += len;
        r += len;
        ll -= len;
        lr -= len;
    }
}

} // namespace

namespace VM::detail {

void gettable(LuaState* L, Value t, const Value& key, Value& result) {
    for (i32 loop = 0; loop < MAXTAGLOOP; loop++) {
        if (t.isTable()) {
            Table* h = t.asTable();
            Value res = h->get(key);
            if (!res.isNil()) {
                result = res;
                return;
            }

            Value tm = getMetamethodByObject(L, t, TMS::TM_INDEX);
            if (tm.isNil()) {
                result = Value();
                return;
            }
            if (tm.isFunction()) {
                callTMWithResult(L, result, tm, t, key);
                return;
            }
            t = tm;
        } else {
            Value tm = getMetamethodByObject(L, t, TMS::TM_INDEX);
            if (tm.isNil()) {
                throw RuntimeError("VM: attempt to index a non-table value");
            }
            if (tm.isFunction()) {
                callTMWithResult(L, result, tm, t, key);
                return;
            }
            t = tm;
        }
    }
    throw RuntimeError("VM: loop in gettable");
}

void settable(LuaState* L, Value t, const Value& key, const Value& val) {
    for (i32 loop = 0; loop < MAXTAGLOOP; loop++) {
        if (t.isTable()) {
            Table* h = t.asTable();
            Value oldval = h->get(key);
            if (!oldval.isNil()) {
                h->set(key, val);
                return;
            }

            Value tm = getMetamethodByObject(L, t, TMS::TM_NEWINDEX);
            if (tm.isNil()) {
                h->set(key, val);
                return;
            }
            if (tm.isFunction()) {
                callTM(L, tm, t, key, val);
                return;
            }
            t = tm;
        } else {
            Value tm = getMetamethodByObject(L, t, TMS::TM_NEWINDEX);
            if (tm.isNil()) {
                throw RuntimeError("VM: attempt to index a non-table value");
            }
            if (tm.isFunction()) {
                callTM(L, tm, t, key, val);
                return;
            }
            t = tm;
        }
    }
    throw RuntimeError("VM: loop in settable");
}

void arith(LuaState* L, Value& result, const Value& left, const Value& right, OpCode op) {
    f64 lval, rval;
    if (tryToNumber(left, lval) && tryToNumber(right, rval)) {
        f64 res = 0.0;
        switch (op) {
        case OpCode::ADD:
            res = lval + rval;
            break;
        case OpCode::SUB:
            res = lval - rval;
            break;
        case OpCode::MUL:
            res = lval * rval;
            break;
        case OpCode::DIV:
            res = lval / rval;
            break;
        case OpCode::MOD:
            res = luaModulo(lval, rval);
            break;
        case OpCode::POW:
            res = std::pow(lval, rval);
            break;
        default:
            throw RuntimeError("VM::arith: invalid opcode");
        }
        result = Value(res);
        return;
    }

    TMS tmEvent;
    switch (op) {
    case OpCode::ADD:
        tmEvent = TMS::TM_ADD;
        break;
    case OpCode::SUB:
        tmEvent = TMS::TM_SUB;
        break;
    case OpCode::MUL:
        tmEvent = TMS::TM_MUL;
        break;
    case OpCode::DIV:
        tmEvent = TMS::TM_DIV;
        break;
    case OpCode::MOD:
        tmEvent = TMS::TM_MOD;
        break;
    case OpCode::POW:
        tmEvent = TMS::TM_POW;
        break;
    default:
        throw RuntimeError("VM::arith: invalid opcode for metamethod");
    }

    Value tmResult;
    if (callBinaryTM(L, left, right, tmResult, tmEvent)) {
        result = tmResult;
        return;
    }
    throw RuntimeError("VM: attempt to perform arithmetic on non-number values");
}

bool equal(LuaState* L, const Value& left, const Value& right) {
    if (left.getType() != right.getType())
        return false;
    if (left.isNil())
        return true;
    if (left.isNumber())
        return left.asNumber() == right.asNumber();
    if (left.isBoolean())
        return left.asBoolean() == right.asBoolean();
    if (left.isString())
        return left.asString()->getData() == right.asString()->getData();

    if (left.isTable()) {
        if (left.asTable() == right.asTable())
            return true;
        Table* mt1 = left.asTable()->getMetatable();
        Table* mt2 = right.asTable()->getMetatable();
        Value tm = getComparisonTM(L, mt1, mt2, TMS::TM_EQ);
        if (!tm.isNil()) {
            Value r;
            callTMWithResult(L, r, tm, left, right);
            return !(r.isNil() || (r.isBoolean() && !r.asBoolean()));
        }
        return false;
    }
    if (left.isUserdata()) {
        if (left.asUserdata() == right.asUserdata())
            return true;
        Table* mt1 = left.asUserdata()->getMetatable();
        Table* mt2 = right.asUserdata()->getMetatable();
        Value tm = getComparisonTM(L, mt1, mt2, TMS::TM_EQ);
        if (!tm.isNil()) {
            Value r;
            callTMWithResult(L, r, tm, left, right);
            return !(r.isNil() || (r.isBoolean() && !r.asBoolean()));
        }
        return false;
    }
    return left == right;
}

bool lessThan(LuaState* L, const Value& left, const Value& right) {
    if (left.getType() != right.getType()) {
        throw RuntimeError("VM: attempt to compare two different types");
    }
    if (left.isNumber())
        return left.asNumber() < right.asNumber();
    if (left.isString())
        return luaStringCompare(left.asString(), right.asString()) < 0;

    i32 tmResult = callOrderTM(L, left, right, TMS::TM_LT);
    if (tmResult == -1) {
        throw RuntimeError("VM: attempt to compare without __lt metamethod");
    }
    return tmResult != 0;
}

bool lessEqual(LuaState* L, const Value& left, const Value& right) {
    if (left.getType() != right.getType()) {
        throw RuntimeError("VM: attempt to compare two different types");
    }
    if (left.isNumber())
        return left.asNumber() <= right.asNumber();
    if (left.isString())
        return luaStringCompare(left.asString(), right.asString()) <= 0;

    i32 tmResult = callOrderTM(L, left, right, TMS::TM_LE);
    if (tmResult != -1)
        return tmResult != 0;

    tmResult = callOrderTM(L, right, left, TMS::TM_LT);
    if (tmResult == -1) {
        throw RuntimeError("VM: attempt to compare without __le or __lt metamethod");
    }
    return tmResult == 0;
}

void unaryMinus(LuaState* L, Value& result, const Value& val) {
    f64 num;
    if (tryToNumber(val, num)) {
        result = Value(-num);
        return;
    }

    Value tmResult;
    if (callBinaryTM(L, val, Value(), tmResult, TMS::TM_UNM)) {
        result = tmResult;
        return;
    }
    throw RuntimeError("VM: attempt to perform arithmetic on a non-number value");
}

void length(LuaState* L, Value& result, const Value& val) {
    if (val.isString()) {
        result = Value(static_cast<f64>(val.asString()->getLength()));
        return;
    }
    if (val.isTable()) {
        result = Value(static_cast<f64>(val.asTable()->length()));
        return;
    }

    Value tm = getMetamethodByObject(L, val, TMS::TM_LEN);
    if (!tm.isNil() && tm.isFunction()) {
        Value r;
        callTMWithResult(L, r, tm, val, Value());
        if (r.isNumber()) {
            result = r;
            return;
        }
        throw RuntimeError("VM: __len metamethod must return a number");
    }
    throw RuntimeError("VM: attempt to get length of a value without __len metamethod");
}

void concat(RuntimeServices& services, LuaState* L, Value* base, i32 a, i32 b, i32 c) {
    Stack& stack = L->getStack();
    const usize baseIndex = static_cast<usize>(base - &stack[0]);
    i32 total = c - b + 1;
    i32 last = c;
    StringPool& pool = services.strings;

    while (total > 1) {
        base = &stack[baseIndex];
        Value& top1 = base[last];
        Value& top2 = base[last - 1];

        ConcatOperandText text1;
        ConcatOperandText text2;
        const bool canConcat = concatOperandText(top2, text2) && concatOperandText(top1, text1);

        if (!canConcat) {
            Value result;
            if (!callBinaryTM(L, top2, top1, result, TMS::TM_CONCAT)) {
                throw RuntimeError("VM: attempt to concatenate non-string/number values");
            }
            base = &stack[baseIndex];
            base[last - 1] = result;
            total--;
            last--;
            continue;
        }

        checkStringConcatLength(text2.view.size(), text1.view.size());
        LuaReallocVector<char> result(services.globalState.getAllocator());
        result.resize(text2.view.size() + text1.view.size());
        std::memcpy(result.data(), text2.view.data(), text2.view.size());
        std::memcpy(result.data() + text2.view.size(), text1.view.data(), text1.view.size());
        const StrView resultView = result.empty() ? StrView("") : StrView(result.data(), result.size());
        base[last - 1] = Value(pool.intern(resultView));
        [[maybe_unused]] const usize collected = services.gc.maybeCollectAutomatic(L);
        total--;
        last--;
    }
    base = &stack[baseIndex];
    base[a] = base[b];
}

} // namespace VM::detail
} // namespace Lua
