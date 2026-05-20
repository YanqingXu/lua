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
#include "vm/vm_dispatch.hpp"
#include "vm/vm_internal.hpp"
#include "vm/lua_state.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/metatable.hpp"
#include "vm/global_state.hpp"
#include "runtime/runtime_services.hpp"
#include "compiler/opcode.hpp"
#include <stdexcept>

namespace Lua {

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

} // anonymous namespace

// =====================================================================
// 公共 API：namespace VM
// =====================================================================

namespace VM {

// -----------------------------------------------------------------
// VM::executeProto — 主执行循环
// 局部变量：func, proto, pc, base（与 Lua C luaV_execute 一致）
// -----------------------------------------------------------------

ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    return executeProto(services, L, proto, nexeccalls);
}

ExecResult executeProto(RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls) {
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

        // dump bytecode at entry
        if (VM::detail::shouldDumpBytecode())
        {
            const Vec<Instruction>& dcode = proto->getCode();
            std::fprintf(stderr, "[BCDUMP] proto(%p) %zu instructions, pc=%zu\n",
                (void*)proto, dcode.size(), pc);
            for (usize di = 0; di < dcode.size(); di++) {
                Instruction dinst = dcode[di];
                std::fprintf(stderr, "  [%zu] op=%d A=%d B=%d C=%d Bx=%d sBx=%d\n",
                    di, static_cast<int>(GET_OPCODE(dinst)),
                    GETARG_A(dinst), GETARG_B(dinst), GETARG_C(dinst),
                    GETARG_Bx(dinst), GETARG_sBx(dinst));
            }
        }


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
            usize instructionPc = pc;
            Instruction inst = code[pc];
            OpCode op  = GET_OPCODE(inst);
            i32    a   = GETARG_A(inst);
            i32    b   = GETARG_B(inst);
            i32    c   = GETARG_C(inst);
            i32    bx  = GETARG_Bx(inst);
            i32    sbx = GETARG_sBx(inst);
            pc++;

            CallInfo& currentCI = L->getCurrentCallInfo();
            currentCI.savedpc = code.data() + pc;

            VM::detail::dispatchCountHook(L);
            base = refreshBase(L);
            VM::detail::dispatchLineHook(L, proto, instructionPc);
            base = refreshBase(L);

            VM::detail::emitInstructionTrace(proto, base, instructionPc, inst, nexeccalls);

            switch (op) {

            // ============== 基础操作 ==============

            case OpCode::MOVE:
                if (VM::detail::shouldDumpBytecode()) {
                    std::fprintf(stderr, "[MOVE] pc=%zu a=%d b=%d base[b]=", instructionPc, a, b);
                    if (base[b].isNumber()) std::fprintf(stderr, "%g", base[b].asNumber());
                    else if (base[b].isNil()) std::fprintf(stderr, "nil");
                    else if (base[b].isFunction()) std::fprintf(stderr, "function");
                    else if (base[b].isString()) std::fprintf(stderr, "'%s'", base[b].asString()->c_str());
                    else std::fprintf(stderr, "other");
                    std::fprintf(stderr, "\n");
                }
                
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
                Value result;
                VM::detail::gettable(L, Value(env), key, result);
                base = refreshBase(L);
                base[a] = result;
                break;
            }

            case OpCode::SETGLOBAL: {
                const Value& key = proto->getConstant(bx);
                Table* env = func->getEnv();
                if (!env) env = L->getGlobalTable();
                Value val = base[a];
                VM::detail::settable(L, Value(env), key, val);
                base = refreshBase(L);
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
                VM::detail::gettable(L, t, key, result);
                base = refreshBase(L);          // 元方法可能导致栈重分配
                base[a] = result;
                break;
            }

            case OpCode::SETTABLE: {
                Value t   = base[a];
                Value key = getRK(proto, base, b);
                Value val = getRK(proto, base, c);
                VM::detail::settable(L, t, key, val);
                base = refreshBase(L);
                break;
            }

            case OpCode::NEWTABLE:
                {
                    Table* table = new Table();
                    L->getGlobalState().getGC().registerObject(table);
                    base[a] = Value(table);
                }
                break;

            case OpCode::SELF: {
                Value obj = base[b];
                base[a + 1] = obj;
                Value key = getRK(proto, base, c);
                Value result;
                VM::detail::gettable(L, obj, key, result);
                base = refreshBase(L);
                base[a] = result;
                break;
            }

            case OpCode::SETLIST:
                VM::detail::setList(L, base, a, b, c);
                break;

            // ============== 算术运算 ==============

            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::POW: {
                VM::detail::execArithmetic(L, proto, base, a, b, c, op);
                break;
            }

            // ============== 一元运算 ==============

            case OpCode::UNM: {
                Value val = base[b]; // copy
                Value result;
                VM::detail::unaryMinus(L, result, val);
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
                VM::detail::length(L, result, val);
                base = refreshBase(L);
                base[a] = result;
                break;
            }

            case OpCode::CONCAT:
                VM::detail::concat(services, L, base, a, b, c);
                base = refreshBase(L);
                break;

            // ============== 跳转和比较 ==============

            case OpCode::JMP:
                pc += sbx;
                break;

            case OpCode::EQ: {
                Value left  = getRK(proto, base, b);
                Value right = getRK(proto, base, c);
                bool result = VM::detail::equal(L, left, right);
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
                bool result = VM::detail::lessThan(L, left, right);
                base = refreshBase(L);
                if (result != (a != 0)) {
                    pc++;
                }
                break;
            }

            case OpCode::LE: {
                Value left  = getRK(proto, base, b);
                Value right = getRK(proto, base, c);
                bool result = VM::detail::lessEqual(L, left, right);
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

                if (VM::detail::shouldDumpBytecode())
                {
                    CallInfo& dbgCI = L->getCurrentCallInfo();
                    std::fprintf(stderr, "[CALL] pc=%zu a=%d B=%d C=%d nArgs=%d nRes=%d base=%zu absTop=%zu\n",
                        instructionPc, a, b, c, nArgs, nResults, dbgCI.base, L->getAbsoluteTop());
                    if (nArgs < 0) {
                        // show stack from base+a to absTop
                        usize funcP = dbgCI.base + a;
                        //std::fprintf(stderr, "[CALL] B=0 stack from %zu to %zu:\n", funcP, L->getAbsoluteTop());
                        Stack& dbgStk = L->getStack();
                        for (usize si = funcP; si < L->getAbsoluteTop(); si++) {
                            Value& v = dbgStk[si];
                            if (v.isNumber()) std::fprintf(stderr, "  [%zu] number=%g\n", si, v.asNumber());
                            else if (v.isFunction()) std::fprintf(stderr, "  [%zu] function\n", si);
                            else if (v.isString()) std::fprintf(stderr, "  [%zu] string='%s'\n", si, v.asString()->c_str());
                            else if (v.isNil()) std::fprintf(stderr, "  [%zu] nil\n", si);
                            else std::fprintf(stderr, "  [%zu] other\n", si);
                        }
                    }
                }

                VM::detail::emitCallTrace(proto, base, instructionPc, a, nexeccalls + 1);

                // 保存 PC（返回后需要）
                L->getCurrentCallInfo().savedpc = &code[pc];

                bool isLua = VM::detail::precall(L, a, nArgs, nResults);

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
                    // nResults = -1 (C=0) → 多返回值模式，postcall 已正确设置 absoluteTop，不可覆盖
                    if (nResults >= 0) {
                        // std::fprintf(stderr, "[CALL-POST] before setTop: callerCI.top=%zu stack.top=%zu absTop=%zu\n",
                        //              callerCI.top, stack.size(), L->getAbsoluteTop());
                        // std::fprintf(stderr, "[CALL-POST] base+a=%zu stack[base+a]=", callerCI.base + static_cast<usize>(a));
                        // {
                        //     usize pos = callerCI.base + static_cast<usize>(a);
                        //     if (stack[pos].isNumber()) std::fprintf(stderr, "%g", stack[pos].asNumber());
                        //     else if (stack[pos].isNil()) std::fprintf(stderr, "nil");
                        //     else if (stack[pos].isFunction()) std::fprintf(stderr, "function");
                        //     else std::fprintf(stderr, "other");
                        // }
                        // std::fprintf(stderr, "\n");
                        stack.setTop(callerCI.top);
                        L->setAbsoluteTop(callerCI.top);
                        // std::fprintf(stderr, "[CALL-POST] after setTop: stack[base+a]=");
                        // {
                        //     usize pos = callerCI.base + static_cast<usize>(a);
                        //     if (stack[pos].isNumber()) std::fprintf(stderr, "%g", stack[pos].asNumber());
                        //     else if (stack[pos].isNil()) std::fprintf(stderr, "nil");
                        //     else if (stack[pos].isFunction()) std::fprintf(stderr, "function");
                        //     else std::fprintf(stderr, "other");
                        // }
                        // std::fprintf(stderr, "\n");
                    }
                }
                base = refreshBase(L);
                break;
            }

            case OpCode::TAILCALL: {
                i32 nArgs = b - 1;

                usize callerIndex = L->getCurrentCI();
                CallInfo& currentCI = L->getCurrentCallInfo();
                usize callerFunc = currentCI.func;
                i32 callerTailcalls = currentCI.tailcalls;
                L->closeUpvalues(currentCI.base);
                currentCI.savedpc = &code[pc];

                bool isLua = VM::detail::precall(L, a, nArgs, -1);

                if (isLua) {
                    VM::detail::reuseCurrentFrameForTailCall(
                        L, callerIndex, callerFunc, callerTailcalls);
                    goto reentry;
                }

                // C 函数 tailcall 已经由 precall 把返回值放到 R(A)
                // 并设置 absoluteTop；紧随其后的 RETURN 会把这些值返回给调用者。
                base = refreshBase(L);
                break;
            }

            case OpCode::RETURN: {
                VM::detail::emitReturnTrace(nexeccalls);
                VM::detail::dispatchReturnHook(L);
                base = refreshBase(L);

                CallInfo& ci = L->getCurrentCallInfo();
                Stack& stack = L->getStack();

                // 关闭 upvalues（必须在移动返回值和收缩栈之前）
                // 参考 lua_c_analysis/src/lvm.c OP_RETURN: luaF_close(L, base) 在 luaD_poscall 之前
                L->closeUpvalues(ci.base);

                // 计算返回值数量
                i32 nres;
                if (b == 0) {
                    nres = static_cast<i32>(L->getAbsoluteTop())
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

                if (--nexeccalls == 0) {
                    return ExecResult::Returned; // 最外层函数返回
                }

                // 弹出 CallInfo，处理返回值
                {
                    i32 funcPos       = static_cast<i32>(ci.func);
                    i32 wantedResults = ci.nresults;
                    L->popCallInfo();
                    VM::detail::postcall(L, funcPos, wantedResults);
                }
                goto reentry; // 继续执行调用者
            }

            // ============== Upvalue 关闭 ==============

            case OpCode::CLOSE:
                {
                    const CallInfo& ci = L->getCurrentCallInfo();
                    L->closeUpvalues(ci.base + static_cast<usize>(a));
                }
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
                VM::detail::tforLoop(L, base, proto, pc, a, c);
                break;

            // ============== 闭包和变参 ==============

            case OpCode::CLOSURE:
                VM::detail::closure(L, base, proto, func, pc, a, bx);
                break;

            case OpCode::VARARG:
                VM::detail::vararg(L, base, proto, a, b);
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
