/**
 * @file vm.cpp
 * @brief Lua虚拟机执行引擎实现 — 纯自由函数风格
 *
 * 设计说明：
 * 与 Lua 5.1 C 实现的 lvm.c 完全对齐：
 * - luaV_execute(L, nexeccalls) 是自由函数
 * - pc, base, cl, k 全部作为局部变量存在于函数调用栈中
 * - 辅助操作（gettable, settable, arith, concat 等）是独立的自由函数，
 *   仅接收 LuaState* 和具体的值参数
 * - 无任何类或对象——所有执行状态显式传递
 *
 * 参考：lua_c_analysis/src/lvm.c luaV_execute()
 */

#include "vm/vm.hpp"
#include "vm/vm_constants.hpp"
#include "vm/lua_state.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/metatable.hpp"
#include "vm/global_state.hpp"
#include "compiler/opcode.hpp"
#include "debug/trace_sink.hpp"
#include "debug/trace_types.hpp"
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace Lua {

// =====================================================================
// Trace 全局状态
// =====================================================================

static ITraceSink* g_traceSink = nullptr;
static u64         g_traceSeq  = 0;

namespace VM {

void setTraceSink(ITraceSink* sink) {
    g_traceSink = sink;
    g_traceSeq  = 0;
}

ITraceSink* getTraceSink() {
    return g_traceSink;
}

} // namespace VM

// =====================================================================
// 匿名命名空间：内部辅助自由函数
// =====================================================================

namespace {

// -----------------------------------------------------------------
// 基础工具函数
// -----------------------------------------------------------------

/**
 * @brief 刷新 base 指针（栈可能因扩展而重新分配）
 * 对应 Lua C: base = L->base;
 */
static inline Value* refreshBase(LuaState* L) {
    return &L->getStack()[L->getCurrentCallInfo().base];
}

/**
 * @brief 获取 RK 值（寄存器或常量）
 * 对应 Lua C:  ISK(x) ? k[INDEXK(x)] : base[x]
 */
static inline Value getRK(Proto* proto, Value* base, i32 rk) {
    if (ISK(rk)) {
        return proto->getConstant(INDEXK(rk));
    }
    return base[rk];
}

/**
 * @brief 尝试将 Value 转换为数字
 * 对应 Lua C: luaV_tonumber()
 */
static bool tryToNumber(const Value& val, f64& result) {
    if (val.isNumber()) {
        result = val.asNumber();
        return true;
    }
    if (val.isString()) {
        GCString* str = val.asString();
        const char* s = str->c_str();
        char* endptr;
        f64 num = std::strtod(s, &endptr);
        if (endptr != s && *endptr == '\0') {
            result = num;
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------
// 表操作（可能触发元方法 → 栈重分配）
// -----------------------------------------------------------------

/**
 * @brief 表读取，支持 __index 元方法链
 * 对应 Lua C: luaV_gettable()
 *
 * @note 调用方需在返回后 refreshBase
 */
static void vmGettable(LuaState* L, Value t, const Value& key, Value& result) {
    for (i32 loop = 0; loop < MAXTAGLOOP; loop++) {
        if (t.isTable()) {
            Table* h = t.asTable();
            Value res = h->get(key);
            if (!res.isNil()) { result = res; return; }

            Value tm = getMetamethodByObject(L, t, TMS::TM_INDEX);
            if (tm.isNil()) { result = Value(); return; }
            if (tm.isFunction()) {
                callTMWithResult(L, result, tm, t, key);
                return;
            }
            t = tm; // __index 是表，继续链式查找
        } else {
            Value tm = getMetamethodByObject(L, t, TMS::TM_INDEX);
            if (tm.isNil())
                throw std::runtime_error("VM: attempt to index a non-table value");
            if (tm.isFunction()) {
                callTMWithResult(L, result, tm, t, key);
                return;
            }
            t = tm;
        }
    }
    throw std::runtime_error("VM: loop in gettable");
}

/**
 * @brief 表写入，支持 __newindex 元方法链
 * 对应 Lua C: luaV_settable()
 */
static void vmSettable(LuaState* L, Value t, const Value& key, const Value& val) {
    for (i32 loop = 0; loop < MAXTAGLOOP; loop++) {
        if (t.isTable()) {
            Table* h = t.asTable();
            Value oldval = h->get(key);
            if (!oldval.isNil()) { h->set(key, val); return; }

            Value tm = getMetamethodByObject(L, t, TMS::TM_NEWINDEX);
            if (tm.isNil()) { h->set(key, val); return; }
            if (tm.isFunction()) { callTM(L, tm, t, key, val); return; }
            t = tm;
        } else {
            Value tm = getMetamethodByObject(L, t, TMS::TM_NEWINDEX);
            if (tm.isNil())
                throw std::runtime_error("VM: attempt to index a non-table value");
            if (tm.isFunction()) { callTM(L, tm, t, key, val); return; }
            t = tm;
        }
    }
    throw std::runtime_error("VM: loop in settable");
}

// -----------------------------------------------------------------
// 算术运算（可能触发元方法）
// -----------------------------------------------------------------

/**
 * @brief 执行二元算术运算，支持元方法
 * 对应 Lua C: Arith() 宏 + luaV_execute 中的算术分支
 */
static void vmArith(LuaState* L, Value& result,
                    const Value& left, const Value& right, OpCode op) {
    f64 lval, rval;
    if (tryToNumber(left, lval) && tryToNumber(right, rval)) {
        f64 res = 0.0;
        switch (op) {
            case OpCode::ADD: res = lval + rval; break;
            case OpCode::SUB: res = lval - rval; break;
            case OpCode::MUL: res = lval * rval; break;
            case OpCode::DIV:
                if (rval == 0.0) throw std::runtime_error("VM: division by zero");
                res = lval / rval; break;
            case OpCode::MOD: res = std::fmod(lval, rval); break;
            case OpCode::POW: res = std::pow(lval, rval); break;
            default: throw std::runtime_error("VM::arith: invalid opcode");
        }
        result = Value(res);
        return;
    }

    // 数字转换失败 → 尝试元方法
    TMS tmEvent;
    switch (op) {
        case OpCode::ADD: tmEvent = TMS::TM_ADD; break;
        case OpCode::SUB: tmEvent = TMS::TM_SUB; break;
        case OpCode::MUL: tmEvent = TMS::TM_MUL; break;
        case OpCode::DIV: tmEvent = TMS::TM_DIV; break;
        case OpCode::MOD: tmEvent = TMS::TM_MOD; break;
        case OpCode::POW: tmEvent = TMS::TM_POW; break;
        default: throw std::runtime_error("VM::arith: invalid opcode for metamethod");
    }

    Value tmResult;
    if (callBinaryTM(L, left, right, tmResult, tmEvent)) {
        result = tmResult;
        return;
    }
    throw std::runtime_error("VM: attempt to perform arithmetic on non-number values");
}

// -----------------------------------------------------------------
// 比较运算（可能触发元方法，返回 bool）
// -----------------------------------------------------------------

/** 对应 Lua C: luaV_equalobj() */
static bool vmEqual(LuaState* L, const Value& left, const Value& right) {
    if (left.getType() != right.getType()) return false;
    if (left.isNil()) return true;
    if (left.isNumber()) return left.asNumber() == right.asNumber();
    if (left.isBoolean()) return left.asBoolean() == right.asBoolean();
    if (left.isString()) return left.asString()->getData() == right.asString()->getData();

    if (left.isTable()) {
        if (left.asTable() == right.asTable()) return true;
        Table* mt1 = left.asTable()->getMetatable();
        Table* mt2 = right.asTable()->getMetatable();
        Value tm = getComparisonTM(L, mt1, mt2, TMS::TM_EQ);
        if (!tm.isNil()) {
            Value r; callTMWithResult(L, r, tm, left, right);
            return !(r.isNil() || (r.isBoolean() && !r.asBoolean()));
        }
        return false;
    }
    if (left.isUserdata()) {
        if (left.asUserdata() == right.asUserdata()) return true;
        Table* mt1 = left.asUserdata()->getMetatable();
        Table* mt2 = right.asUserdata()->getMetatable();
        Value tm = getComparisonTM(L, mt1, mt2, TMS::TM_EQ);
        if (!tm.isNil()) {
            Value r; callTMWithResult(L, r, tm, left, right);
            return !(r.isNil() || (r.isBoolean() && !r.asBoolean()));
        }
        return false;
    }
    return left == right;
}

/** 对应 Lua C: luaV_lessthan() */
static bool vmLessThan(LuaState* L, const Value& left, const Value& right) {
    if (left.getType() != right.getType())
        throw std::runtime_error("VM: attempt to compare two different types");
    if (left.isNumber()) return left.asNumber() < right.asNumber();
    if (left.isString()) return left.asString()->getData() < right.asString()->getData();

    i32 tmResult = callOrderTM(L, left, right, TMS::TM_LT);
    if (tmResult == -1)
        throw std::runtime_error("VM: attempt to compare without __lt metamethod");
    return tmResult != 0;
}

/** 对应 Lua C: lessequal() */
static bool vmLessEqual(LuaState* L, const Value& left, const Value& right) {
    if (left.getType() != right.getType())
        throw std::runtime_error("VM: attempt to compare two different types");
    if (left.isNumber()) return left.asNumber() <= right.asNumber();
    if (left.isString()) return left.asString()->getData() <= right.asString()->getData();

    i32 tmResult = callOrderTM(L, left, right, TMS::TM_LE);
    if (tmResult != -1) return tmResult != 0;

    // 回退到 __lt: a <= b ⟺ !(b < a)
    tmResult = callOrderTM(L, right, left, TMS::TM_LT);
    if (tmResult == -1)
        throw std::runtime_error("VM: attempt to compare without __le or __lt metamethod");
    return tmResult == 0; // 取反
}

// -----------------------------------------------------------------
// 一元运算
// -----------------------------------------------------------------

/** @brief 取负运算，支持 __unm 元方法 */
static void vmUnm(LuaState* L, Value& result, const Value& val) {
    f64 num;
    if (tryToNumber(val, num)) { result = Value(-num); return; }

    Value tmResult;
    if (callBinaryTM(L, val, Value(), tmResult, TMS::TM_UNM)) {
        result = tmResult; return;
    }
    throw std::runtime_error("VM: attempt to perform arithmetic on a non-number value");
}

/** @brief 取长度运算，支持 __len 元方法 */
static void vmLen(LuaState* L, Value& result, const Value& val) {
    if (val.isString()) {
        result = Value(static_cast<f64>(val.asString()->getLength()));
        return;
    }
    if (val.isTable()) {
        Value tm = getMetamethodByObject(L, val, TMS::TM_LEN);
        if (!tm.isNil() && tm.isFunction()) {
            Value r; callTMWithResult(L, r, tm, val, Value());
            if (r.isNumber()) { result = r; return; }
            throw std::runtime_error("VM: __len metamethod must return a number");
        }
        result = Value(static_cast<f64>(val.asTable()->length()));
        return;
    }
    // 其他类型
    Value tm = getMetamethodByObject(L, val, TMS::TM_LEN);
    if (!tm.isNil() && tm.isFunction()) {
        Value r; callTMWithResult(L, r, tm, val, Value());
        if (r.isNumber()) { result = r; return; }
        throw std::runtime_error("VM: __len metamethod must return a number");
    }
    throw std::runtime_error("VM: attempt to get length of a value without __len metamethod");
}

// -----------------------------------------------------------------
// 字符串连接
// -----------------------------------------------------------------

/** @brief 连接操作，支持 __concat 元方法 */
static void vmConcat(LuaState* L, Value* base, i32 a, i32 b, i32 c) {
    i32 total = c - b + 1;
    i32 last = c;
    StringPool& pool = GlobalState::getInstance().getStringPool();

    while (total > 1) {
        Value& top1 = base[last];
        Value& top2 = base[last - 1];

        Str str1, str2;
        bool canConcat = false;

        if (top2.isString())      { str2 = top2.asString()->getData(); canConcat = true; }
        else if (top2.isNumber()) { str2 = std::to_string(top2.asNumber()); canConcat = true; }

        if (canConcat) {
            if (top1.isString())      str1 = top1.asString()->getData();
            else if (top1.isNumber()) str1 = std::to_string(top1.asNumber());
            else canConcat = false;
        }

        if (!canConcat) {
            Value result;
            if (!callBinaryTM(L, top2, top1, result, TMS::TM_CONCAT))
                throw std::runtime_error("VM: attempt to concatenate non-string/number values");
            base[last - 1] = result;
            total--; last--;
            continue;
        }

        if (!str1.empty()) {
            Str result = str2 + str1;
            base[last - 1] = Value(pool.intern(result));
        }
        total--; last--;
    }
    base[a] = base[b];
}

// -----------------------------------------------------------------
// 返回值处理 (postcall)
// -----------------------------------------------------------------

/**
 * @brief 处理函数返回后的返回值复制和栈调整
 * 对应 Lua C: luaD_poscall()
 *
 * @param firstResult 返回值的起始位置，0 表示返回值已在 funcPos 位置
 */
static void vmPostcall(LuaState* L, i32 funcPos, i32 wantedResults,
                       usize firstResult = 0) {
    Stack& stack = L->getStack();
    usize res = static_cast<usize>(funcPos);
    usize currentTop = L->getAbsoluteTop();

    if (firstResult == 0) {
        // 返回值已在 funcPos 位置 — 只调整数量
        i32 actualResults = static_cast<i32>(currentTop) - funcPos;
        if (wantedResults >= 0) {
            if (actualResults < wantedResults) {
                while (actualResults < wantedResults) {
                    if (currentTop >= stack.size()) stack.push(Value());
                    else stack.at(currentTop) = Value();
                    currentTop++;
                    actualResults++;
                }
            } else if (actualResults > wantedResults) {
                currentTop -= (actualResults - wantedResults);
                actualResults = wantedResults;
            }
        }
        L->setAbsoluteTop(funcPos + actualResults);
    } else {
        // 将返回值从 firstResult 复制到 funcPos
        i32 i = wantedResults;
        usize src = firstResult;
        while (i != 0 && src < currentTop) {
            stack[res++] = stack[src++];
            i--;
        }
        while (i-- > 0) stack[res++] = Value();
        L->setAbsoluteTop(res);
    }
}

// -----------------------------------------------------------------
// 函数调用准备 (precall)
// -----------------------------------------------------------------

/**
 * @brief 准备函数调用（C函数完整执行，Lua函数仅创建CallInfo）
 * 对应 Lua C: luaD_precall()
 *
 * @return true = Lua 函数（调用方需 goto reentry），false = C 函数（已执行完毕）
 */
static bool vmPrecall(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults) {
    Stack& stack = L->getStack();
    CallInfo& currentCI = L->getCurrentCallInfo();
    usize funcPos = currentCI.base + funcIndex;
    Value& funcVal = stack.at(funcPos);

    // __call 元方法支持
    if (!funcVal.isFunction()) {
        Value tm = getMetamethodByObject(L, funcVal, TMS::TM_CALL);
        if (tm.isNil() || !tm.isFunction())
            throw std::runtime_error("VM::precall: attempt to call non-function value without __call metamethod");

        Value originalFunc = funcVal;
        Vec<Value> args;
        for (i32 i = 1; i <= nArgs; i++) args.push_back(stack.at(funcPos + i));

        stack.at(funcPos) = tm;
        stack.at(funcPos + 1) = originalFunc;
        for (usize i = 0; i < args.size(); i++) stack.at(funcPos + 2 + i) = args[i];
        nArgs++;

        // 重新获取
        funcVal = stack.at(funcPos);
        if (!funcVal.isFunction())
            throw std::runtime_error("VM::precall: __call metamethod is not a function");
    }

    Function* func = funcVal.asFunction();

    if (func->isCFunction()) {
        // ---- C 函数：创建 CallInfo → 调用 → postcall → 弹出 ----
        CFunction cfunc = func->getCFunction();

        CallInfo& ci = L->pushCallInfo();
        ci.func = funcPos;
        ci.base = funcPos + 1;
        ci.top = funcPos + 1 + nArgs + 20;
        ci.nresults = nResults;
        ci.savedpc = nullptr;
        ci.tailcalls = 0;

        while (stack.size() < ci.top) stack.push(Value());
        L->setAbsoluteTop(funcPos + 1 + nArgs);

        i32 nReturnValues = cfunc(L);

        // If C function triggered yield, preserve its CallInfo for resume
        if (L->getStatus() == ThreadStatus::Yield) {
            return false;
        }

        usize currentTop = L->getAbsoluteTop();
        usize firstResult = currentTop - static_cast<usize>(nReturnValues);
        vmPostcall(L, static_cast<i32>(funcPos), nResults, firstResult);

        L->popCallInfo();
        return false; // C 函数已执行完毕
    } else {
        // ---- Lua 函数：创建 CallInfo → 返回 true ----
        Proto* proto = func->getProto();
        i32 actualArgs = nArgs;
        if (nArgs < 0) {
            actualArgs = static_cast<i32>(L->getAbsoluteTop())
                       - static_cast<i32>(funcPos + 1);
        }

        i32 numParams = proto->getNumParams();
        usize base;

        if (proto->isVararg()) {
            // adjust_varargs 逻辑
            while (actualArgs < numParams) { stack.push(Value()); actualArgs++; }
            usize oldBase = funcPos + 1;
            base = oldBase + static_cast<usize>(actualArgs);
            stack.checkSpace(static_cast<usize>(numParams) + 1);
            for (i32 i = 0; i < numParams; i++) {
                stack.push(stack[oldBase + i]);
                stack[oldBase + i] = Value();
            }
        } else {
            base = funcPos + 1;
            while (actualArgs < numParams) { stack.push(Value()); actualArgs++; }
        }

        CallInfo& ci = L->pushCallInfo();
        ci.func = funcPos;
        ci.base = base;
        ci.top = base + proto->getMaxStackSize();
        ci.nresults = nResults;
        ci.savedpc = nullptr;
        ci.tailcalls = 0;

        while (stack.size() < ci.top) stack.push(Value());
        L->setAbsoluteTop(ci.top);

        return true; // Lua 函数，调用方需 goto reentry
    }
}

// -----------------------------------------------------------------
// SETLIST
// -----------------------------------------------------------------

static void vmSetList(LuaState* L, Value* base, i32 a, i32 b, i32 c) {
    if (!base[a].isTable())
        throw std::runtime_error("VM: SETLIST requires table");

    Table* table = base[a].asTable();
    i32 n = b;

    if (n == 0) {
        CallInfo& ci = L->getCurrentCallInfo();
        Stack& stack = L->getStack();
        usize ra = ci.base + static_cast<usize>(a);
        n = static_cast<i32>(stack.size() - ra) - 1;
        L->setAbsoluteTop(ci.top);
    }

    i32 base_index = (c - 1) * FIELDS_PER_FLUSH;
    for (i32 i = 1; i <= n; i++) {
        table->setArray(base_index + i, base[a + i]);
    }
}

// -----------------------------------------------------------------
// TFORLOOP（泛型 for 循环迭代）
// -----------------------------------------------------------------

static void vmTForLoop(LuaState* L, Value*& base, Proto* proto,
                       usize& pc, i32 a, i32 c) {
    i32 cb = a + 3;
    CallInfo& ci = L->getCurrentCallInfo();
    Stack& stack = L->getStack();
    usize requiredSize = ci.base + cb + 3 + c;
    while (stack.size() < requiredSize) stack.push(Value());
    base = &stack[ci.base]; // refresh

    // 复制迭代器函数和参数
    base[cb + 2] = base[a + 2];
    base[cb + 1] = base[a + 1];
    base[cb]     = base[a];

    if (!base[cb].isFunction())
        throw std::runtime_error("VM: TFORLOOP requires function at R("
                                 + std::to_string(cb) + ")");

    Function* func = base[cb].asFunction();

    if (func->isCFunction()) {
        ci.savedpc = &proto->getCode()[pc];

        bool isLua = vmPrecall(L, cb, 2, c);
        if (isLua) {
            throw std::runtime_error("VM: TFORLOOP Lua iterators not supported yet");
        }

        // 与普通 CALL 一样，恢复调用者寄存器窗口，避免后续写寄存器时覆盖高位槽位。
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
            // 模拟执行“下一条 JMP”指令本身：主循环在进入 case 前已经先做过 pc++，
            // 因此这里除了加上 JMP 的 sBx，还需要额外前进一步，才能落到循环体开头。
            pc += GETARG_sBx(proto->getCode()[pc]) + 1; // 跳回循环体
        }
    } else {
        pc++; // 跳过 JMP，退出循环
    }
}

// -----------------------------------------------------------------
// CLOSURE
// -----------------------------------------------------------------

static void vmClosure(LuaState* L, Value* base, Proto* currentProto,
                      Function* currentFunc, usize& pc, i32 a, i32 bx) {
    if (bx < 0 || static_cast<usize>(bx) >= currentProto->getSubProtoCount())
        throw std::runtime_error("VM: CLOSURE proto index out of range");

    Proto* childProto = currentProto->getSubProto(bx);
    Function* closure = new Function(childProto);

    i32 nups = childProto->getNumUpvalues();
    if (nups > 0) {
        const Vec<Instruction>& code = currentProto->getCode();
        const CallInfo& ci = L->getCurrentCallInfo();

        for (i32 j = 0; j < nups; j++) {
            if (pc >= code.size()) {
                throw std::runtime_error("VM: CLOSURE missing upvalue pseudo instruction");
            }

            Instruction inst = code[pc++];
            OpCode pop = GET_OPCODE(inst);
            i32 b = GETARG_B(inst);

            if (pop == OpCode::MOVE) {
                // 从父函数栈槽捕获upvalue（open upvalue，可共享）
                closure->addUpvalue(L->findOrCreateUpvalue(ci.base + static_cast<usize>(b)));
            } else if (pop == OpCode::GETUPVAL) {
                // 复用父闭包的upvalue
                Upvalue* uv = currentFunc->getUpvalue(static_cast<usize>(b));
                if (!uv) {
                    throw std::runtime_error("VM: CLOSURE invalid parent upvalue index");
                }
                closure->addUpvalue(uv);
            } else {
                throw std::runtime_error("VM: CLOSURE expects MOVE/GETUPVAL pseudo instruction");
            }
        }
    }

    base[a] = Value(closure);
}

// -----------------------------------------------------------------
// VARARG
// -----------------------------------------------------------------

static void vmVararg(LuaState* L, Value*& base, Proto* proto, i32 a, i32 b) {
    CallInfo& ci = L->getCurrentCallInfo();
    Stack& stack = L->getStack();
    i32 numParams = proto->getNumParams();

    i32 n = static_cast<i32>(ci.base - ci.func - 1) - numParams;
    if (n < 0) n = 0;

    i32 wanted;
    if (b == 0) {
        wanted = n;
        usize neededTop = ci.base + static_cast<usize>(a) + static_cast<usize>(n);
        if (stack.size() < neededTop) {
            stack.checkSpace(neededTop - stack.size());
            base = refreshBase(L);
        }
        L->setAbsoluteTop(ci.base + static_cast<usize>(a) + static_cast<usize>(n));
    } else {
        wanted = b - 1;
    }

    for (i32 j = 0; j < wanted; j++) {
        if (j < n) {
            usize srcIndex = ci.base - static_cast<usize>(n) + static_cast<usize>(j);
            base[a + j] = stack[srcIndex];
        } else {
            base[a + j] = Value();
        }
    }
}

} // anonymous namespace

// =====================================================================
// 公共 API：namespace VM
// =====================================================================

namespace VM {

// -----------------------------------------------------------------
// VM::call — 从 CFunction 内部安全调用一个函数
//
// 调用方（CFunction 上下文）已将 func + args 压入栈。
// 本函数在 *不清除* 栈的前提下执行被调用函数，
// 执行完毕后结果留在原 func 位置。
//
// nargs: 参数个数（不含函数本身）
// nresults: 期望的返回值数量（MULTRET = -1 表示全部）
//
// 栈布局（before）：  [...existing... func arg1 arg2]
//   absoluteTop 指向 arg2 之后
// 栈布局（after）：   [...existing... result1 result2]
//   absoluteTop 指向最后一个 result 之后
// -----------------------------------------------------------------
void call(LuaState* L, i32 nargs, i32 nresults) {
    // funcPos（绝对栈索引）= top - nargs - 1
    usize absTop = L->getAbsoluteTop();
    usize funcPos = absTop - static_cast<usize>(nargs) - 1;

    CallInfo& ci = L->getCurrentCallInfo();
    i32 funcIndex = static_cast<i32>(funcPos - ci.base);

    bool isLua = vmPrecall(L, funcIndex, nargs, nresults);
    if (isLua) {
        // Lua 函数：vmPrecall 已创建新的 CallInfo。
        // 使用 nexeccalls=1，这样 OP_RETURN 的 --nexeccalls==0 路径会
        // 直接返回（不 goto reentry）。该路径已经将返回值移至 ci.func
        // 并关闭了 upvalues，但不调用 vmPostcall/popCallInfo。
        CallInfo& newCI = L->getCurrentCallInfo();
        Proto* proto = L->getStack()[newCI.func].asFunction()->getProto();
        executeProto(L, proto, 1);

        // OP_RETURN (nexeccalls==0) 已经把返回值放到 newCI.func 位置，
        // 并调用了 closeUpvalues、shrunk stack。现在只需 popCallInfo 和
        // 调整 absoluteTop 以反映 nresults。
        i32 fpos = static_cast<i32>(newCI.func);
        i32 wantedResults = newCI.nresults;
        L->popCallInfo();
        vmPostcall(L, fpos, wantedResults);
    }
    // C 函数：vmPrecall 已经执行完毕并做了 postcall + popCallInfo
}

// -----------------------------------------------------------------
// VM::execute — 最外层入口
// -----------------------------------------------------------------

void execute(LuaState* L, Function* func) {
    if (!func)
        throw std::runtime_error("VM::execute: null function");
    if (func->isCFunction())
        throw std::runtime_error("VM::execute: C functions not supported yet");

    Stack& stack = L->getStack();

    // 将函数压入栈（模拟调用者）
    stack.push(Value(func));
    usize funcIndex = stack.size() - 1;

    // 创建初始 CallInfo
    CallInfo& ci = L->pushCallInfo();
    ci.func     = funcIndex;
    ci.base     = funcIndex + 1;
    ci.top      = ci.base;
    ci.savedpc  = nullptr;
    ci.nresults = -1;
    ci.tailcalls = 0;

    Proto* proto = func->getProto();
    usize requiredTop = ci.base + proto->getMaxStackSize();
    if (stack.capacity() < requiredTop)
        stack.checkSpace(requiredTop - stack.size());
    while (stack.size() < requiredTop)
        stack.push(Value());

    ci.top = requiredTop;
    L->setAbsoluteTop(requiredTop);

    // 执行（nexeccalls = 1）
    executeProto(L, proto, 1);

    L->popCallInfo();
}

// -----------------------------------------------------------------
// VM::executeProto — 主执行循环
// 局部变量：func, proto, pc, base（与 Lua C luaV_execute 一致）
// -----------------------------------------------------------------

ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    // 深度检查
    if (!proto)
        throw std::runtime_error("VM::executeProto: null proto");
    if (nexeccalls >= MAX_CALLS)
        throw std::runtime_error("VM: stack overflow (too many nested calls)");

    // ---- 局部执行状态 ----
    Function* func  = nullptr;
    Value*    base  = nullptr;
    usize     pc    = 0;

reentry: // ⭐ 重入点：从 CallInfo 恢复所有执行状态
    {
        CallInfo& ci = L->getCurrentCallInfo();
        Stack& stack = L->getStack();

        // 恢复 func
        if (ci.func >= stack.capacity())
            throw std::runtime_error("VM::executeProto: CallInfo.func out of range");
        Value& funcVal = stack[ci.func];
        if (!funcVal.isFunction())
            throw std::runtime_error("VM::executeProto: CallInfo.func is not a function");
        func = funcVal.asFunction();

        // 恢复 proto 和 pc
        proto = func->getProto();
        pc = ci.savedpc
           ? static_cast<usize>(ci.savedpc - proto->getCode().data())
           : 0;

        // 确保栈空间
        usize requiredTop = ci.base + proto->getMaxStackSize();
        if (stack.capacity() < requiredTop)
            stack.checkSpace(requiredTop - stack.size());
        while (stack.size() < requiredTop)
            stack.push(Value());

        // 刷新 base
        base = &stack[ci.base];
    }

    // ---- 主执行循环 ----
    {
        const Vec<Instruction>& code = proto->getCode();

        while (pc < code.size()) {
            Instruction inst = code[pc];
            OpCode op  = GET_OPCODE(inst);
            i32    a   = GETARG_A(inst);
            i32    b   = GETARG_B(inst);
            i32    c   = GETARG_C(inst);
            i32    bx  = GETARG_Bx(inst);
            i32    sbx = GETARG_sBx(inst);
            pc++;

            // ---- Trace: 指令事件 ----
            if (g_traceSink) {
                TraceEvent tevt;
                tevt.seq       = g_traceSeq++;
                tevt.kind      = TraceEventKind::Instruction;
                tevt.pc        = static_cast<i32>(pc - 1);
                tevt.op        = op;
                tevt.a = a; tevt.b = b; tevt.c = c;
                tevt.bx = bx; tevt.sbx = sbx;
                tevt.line      = proto->getLine(pc - 1);
                tevt.source    = proto->getSource() ? proto->getSource()->c_str() : "?";
                tevt.callDepth = nexeccalls;
                tevt.base      = base;
                tevt.maxStack  = proto->getMaxStackSize();
                tevt.proto     = proto;
                g_traceSink->onInstruction(tevt);
            }

            switch (op) {

            // ============== 基础操作 ==============

            case OpCode::MOVE:
                base[a] = base[b];
                break;

            case OpCode::LOADK:
                base[a] = proto->getConstant(bx);
                break;

            case OpCode::LOADBOOL:
                base[a] = Value(b != 0);
                if (c != 0) pc++;
                break;

            case OpCode::LOADNIL:
                for (i32 i = a; i <= b; i++) base[i] = Value();
                break;

            // ============== 全局变量操作 ==============

            case OpCode::GETGLOBAL: {
                const Value& key = proto->getConstant(bx);
                Table* env = func->getEnv();
                if (!env) env = L->getGlobalTable();
                base[a] = env->get(key);
                break;
            }

            case OpCode::SETGLOBAL: {
                const Value& key = proto->getConstant(bx);
                Table* env = func->getEnv();
                if (!env) env = L->getGlobalTable();
                env->set(key, base[a]);
                break;
            }

            // ============== Upvalue 操作 ==============

            case OpCode::GETUPVAL: {
                Upvalue* uv = func->getUpvalue(b);
                if (!uv) throw std::runtime_error("VM: GETUPVAL invalid upvalue index");
                base[a] = uv->getValue(L->getStack());
                break;
            }

            case OpCode::SETUPVAL: {
                Upvalue* uv = func->getUpvalue(b);
                if (!uv) throw std::runtime_error("VM: SETUPVAL invalid upvalue index");
                uv->setValue(L->getStack(), base[a]);
                break;
            }

            // ============== 表操作 ==============

            case OpCode::GETTABLE: {
                Value t   = base[b];            // copy（防止栈重分配后悬挂）
                Value key = getRK(proto, base, c); // copy
                Value result;
                vmGettable(L, t, key, result);
                base = refreshBase(L);          // 元方法可能导致栈重分配
                base[a] = result;
                break;
            }

            case OpCode::SETTABLE: {
                Value t   = base[a];
                Value key = getRK(proto, base, b);
                Value val = getRK(proto, base, c);
                vmSettable(L, t, key, val);
                base = refreshBase(L);
                break;
            }

            case OpCode::NEWTABLE:
                base[a] = Value(new Table());
                break;

            case OpCode::SELF: {
                Value obj = base[b];
                base[a + 1] = obj;
                Value key = getRK(proto, base, c);
                vmGettable(L, obj, key, base[a]);
                base = refreshBase(L);
                break;
            }

            case OpCode::SETLIST:
                vmSetList(L, base, a, b, c);
                break;

            // ============== 算术运算 ==============

            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::POW: {
                Value left  = getRK(proto, base, b);
                Value right = getRK(proto, base, c);
                Value result;
                vmArith(L, result, left, right, op);
                base = refreshBase(L);
                base[a] = result;
                break;
            }

            // ============== 一元运算 ==============

            case OpCode::UNM: {
                Value val = base[b]; // copy
                Value result;
                vmUnm(L, result, val);
                base = refreshBase(L);
                base[a] = result;
                break;
            }

            case OpCode::NOT:
                base[a] = Value(!base[b].isTrue());
                break;

            case OpCode::LEN: {
                Value val = base[b]; // copy
                Value result;
                vmLen(L, result, val);
                base = refreshBase(L);
                base[a] = result;
                break;
            }

            case OpCode::CONCAT:
                vmConcat(L, base, a, b, c);
                base = refreshBase(L);
                break;

            // ============== 跳转和比较 ==============

            case OpCode::JMP:
                pc += sbx;
                break;

            case OpCode::EQ: {
                Value left  = getRK(proto, base, b);
                Value right = getRK(proto, base, c);
                bool result = vmEqual(L, left, right);
                base = refreshBase(L);
                // Lua 5.1 语义：若比较结果与A不同，则跳过下一条指令（通常是JMP）
                if (result != (a != 0)) {
                    pc++;
                }
                break;
            }

            case OpCode::LT: {
                Value left  = getRK(proto, base, b);
                Value right = getRK(proto, base, c);
                bool result = vmLessThan(L, left, right);
                base = refreshBase(L);
                if (result != (a != 0)) {
                    pc++;
                }
                break;
            }

            case OpCode::LE: {
                Value left  = getRK(proto, base, b);
                Value right = getRK(proto, base, c);
                bool result = vmLessEqual(L, left, right);
                base = refreshBase(L);
                if (result != (a != 0)) {
                    pc++;
                }
                break;
            }

            // ============== 条件测试 ==============

            case OpCode::TEST: {
                bool val = base[a].isTrue();
                if ((!val) != (c != 0)) {
                    if (pc < code.size())
                        pc += GETARG_sBx(code[pc]);
                }
                pc++;
                break;
            }

            case OpCode::TESTSET: {
                bool val = base[b].isTrue();
                if ((!val) != (c != 0)) {
                    base[a] = base[b];
                    if (pc < code.size())
                        pc += GETARG_sBx(code[pc]);
                }
                pc++;
                break;
            }

            // ============== 函数调用 ==============

            case OpCode::CALL: {
                i32 nArgs    = b - 1;
                i32 nResults = c - 1;

                // ---- Trace: call 事件 ----
                if (g_traceSink) {
                    TraceEvent cevt;
                    cevt.seq       = g_traceSeq++;
                    cevt.kind      = TraceEventKind::Call;
                    cevt.line      = proto->getLine(pc - 1);
                    cevt.source    = proto->getSource() ? proto->getSource()->c_str() : "?";
                    cevt.callDepth = nexeccalls + 1;
                    // 尝试获取被调用函数名
                    Value& callee = base[a];
                    if (callee.isFunction()) {
                        Function* cf = callee.asFunction();
                        if (cf->getProto() && cf->getProto()->getSource())
                            cevt.funcName = cf->getProto()->getSource()->c_str();
                    }
                    g_traceSink->onCall(cevt);
                }

                // 保存 PC（返回后需要）
                L->getCurrentCallInfo().savedpc = &code[pc];

                bool isLua = vmPrecall(L, a, nArgs, nResults);

                if (isLua) {
                    nexeccalls++;
                    goto reentry;
                }
                // yield detection: C function (e.g. coroutine.yield) may set Yield
                if (L->getStatus() == ThreadStatus::Yield) {
                    L->setSavedNexeccalls(nexeccalls);
                    return ExecResult::Yielded;
                }

                {
                    CallInfo& callerCI = L->getCurrentCallInfo();
                    Stack& stack = L->getStack();
                    stack.setTop(callerCI.top);
                    L->setAbsoluteTop(callerCI.top);
                }
                base = refreshBase(L);
                break;
            }

            case OpCode::TAILCALL: {
                i32 nArgs = b - 1;

                CallInfo& currentCI = L->getCurrentCallInfo();
                L->closeUpvalues(currentCI.base);
                currentCI.savedpc = &code[pc];

                bool isLua = vmPrecall(L, a, nArgs, -1);

                if (isLua) {
                    currentCI.tailcalls++;
                    // 注意：当前实现简化处理尾调用，未做完整的栈帧复用优化
                    // TODO: 参考 Lua C 实现进行完整的尾调用优化
                    goto reentry;
                }
                // C 函数 tailcall，同步当前 Lua 帧的寄存器窗口。
                {
                    CallInfo& callerCI = L->getCurrentCallInfo();
                    Stack& stack = L->getStack();
                    stack.setTop(callerCI.top);
                    L->setAbsoluteTop(callerCI.top);
                }
                base = refreshBase(L);
                break;
            }

            case OpCode::RETURN: {
                // ---- Trace: return 事件 ----
                if (g_traceSink) {
                    TraceEvent revt;
                    revt.seq       = g_traceSeq++;
                    revt.kind      = TraceEventKind::Return;
                    revt.callDepth = nexeccalls;
                    g_traceSink->onReturn(revt);
                }

                CallInfo& ci = L->getCurrentCallInfo();
                Stack& stack = L->getStack();

                // 计算返回值数量
                i32 nres;
                if (b == 0) {
                    nres = static_cast<i32>(stack.size())
                         - (static_cast<i32>(ci.base) + a);
                } else {
                    nres = b - 1;
                }

                // 将返回值移到 funcPos
                for (i32 i = 0; i < nres; i++)
                    stack.at(ci.func + i) = base[a + i];

                // 收缩栈
                usize newTop = ci.func + nres;
                while (stack.size() > newTop) stack.pop();
                // 同步LuaState逻辑栈顶，避免postcall按过期top_计算返回值数量
                L->setAbsoluteTop(newTop);

                // 关闭 upvalues
                L->closeUpvalues(ci.base);

                if (--nexeccalls == 0) {
                    return ExecResult::Returned; // 最外层函数返回
                }

                // 弹出 CallInfo，处理返回值
                {
                    i32 funcPos       = static_cast<i32>(ci.func);
                    i32 wantedResults = ci.nresults;
                    L->popCallInfo();
                    vmPostcall(L, funcPos, wantedResults);
                }
                goto reentry; // 继续执行调用者
            }

            // ============== Upvalue 关闭 ==============

            case OpCode::CLOSE:
                L->closeUpvalues(a);
                break;

            // ============== 循环指令 ==============

            case OpCode::FORLOOP: {
                if (!base[a].isNumber() || !base[a + 1].isNumber() || !base[a + 2].isNumber())
                    throw std::runtime_error("VM: FORLOOP requires numeric values");

                f64 step  = base[a + 2].asNumber();
                f64 idx   = base[a].asNumber() + step;
                f64 limit = base[a + 1].asNumber();

                bool cont = (step > 0) ? (idx <= limit) : (idx >= limit);
                if (cont) {
                    pc += sbx;                  // 跳回循环体
                    base[a]     = Value(idx);   // 内部索引
                    base[a + 3] = Value(idx);   // 用户可见变量
                }
                break;
            }

            case OpCode::FORPREP: {
                if (!base[a].isNumber() || !base[a + 1].isNumber() || !base[a + 2].isNumber())
                    throw std::runtime_error("VM: FORPREP requires numeric values");
                f64 init = base[a].asNumber();
                f64 step = base[a + 2].asNumber();
                base[a] = Value(init - step);
                pc += sbx;
                break;
            }

            case OpCode::TFORLOOP:
                vmTForLoop(L, base, proto, pc, a, c);
                break;

            // ============== 闭包和变参 ==============

            case OpCode::CLOSURE:
                vmClosure(L, base, proto, func, pc, a, bx);
                break;

            case OpCode::VARARG:
                vmVararg(L, base, proto, a, b);
                break;

            // ============== 未知指令 ==============

            default:
                throw std::runtime_error("VM: unsupported opcode: "
                                         + Str(getOpName(op)));

            } // switch
        } // while
    } // scope for code reference

    return ExecResult::Returned;
}

} // namespace VM

} // namespace Lua
