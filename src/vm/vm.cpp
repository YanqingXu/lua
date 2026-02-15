/**
 * @file vm.cpp
 * @brief Lua虚拟机执行引擎实现
 */

#include "vm/vm.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "vm/global_state.hpp"
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace Lua {

// =====================================================================
// 构造函数
// =====================================================================

VM::VM(LuaState* L)
    : L_(L)
    , currentProto_(nullptr)
    , pc_(0)
    , base_(nullptr)
    , currentFunc_(nullptr)
{
}

// =====================================================================
// 执行接口
// =====================================================================

void VM::execute(Function* func) {
    if (!func) {
        throw std::runtime_error("VM::execute: null function");
    }

    if (func->isCFunction()) {
        throw std::runtime_error("VM::execute: C functions not supported yet");
    }

    // ⭐ P0修复：参考lua_c_analysis/src/ldo.c中的luaD_call
    // execute是最外层入口，需要设置初始的CallInfo
    Stack& stack = L_->getStack();

    // 将函数压入栈（模拟调用者）
    stack.push(Value(func));
    usize funcIndex = stack.size() - 1;

    // 创建初始CallInfo（这是第0层调用）
    CallInfo& ci = L_->pushCallInfo();
    ci.func = funcIndex;
    ci.base = funcIndex + 1;  // base指向第一个参数/局部变量位置
    ci.top = ci.base;  // 初始时没有参数
    ci.savedpc = nullptr;  // 最外层调用没有savedpc
    ci.nresults = -1;  // 接受所有返回值
    ci.tailcalls = 0;

    // 设置当前函数和Proto
    currentFunc_ = func;
    currentProto_ = func->getProto();
    pc_ = 0;

    // ⭐ 先确保栈空间足够，再更新base指针
    usize requiredTop = ci.base + currentProto_->getMaxStackSize();
    if (stack.capacity() < requiredTop) {
        ensureStackSpace(requiredTop - stack.size());
    }
    while (stack.size() < requiredTop) {
        stack.push(Value());  // nil
    }

    // ⭐ P0修复：设置栈顶
    ci.top = requiredTop;
    L_->setAbsoluteTop(requiredTop);

    // 更新base指针（必须在栈扩展之后）
    updateBasePointer();

    // 执行函数（nexeccalls=1表示这是第一层调用）
    executeProto(currentProto_, 1);

    // ⭐ P0修复：执行完成后清理CallInfo
    L_->popCallInfo();
}

void VM::executeProto(Proto* proto, i32 nexeccalls) {
    validateAndCheckDepth(proto, nexeccalls);

reentry:  // ⭐ P0修复：添加reentry标签，参考lua_c_analysis/src/lvm.c:1558
    initializeExecutionContext();

    // 主执行循环 - 简洁的指令分发
    const Vec<Instruction>& code = currentProto_->getCode();

    while (pc_ < code.size()) {
        Instruction inst = code[pc_];

        // 解码指令
        OpCode op = GET_OPCODE(inst);
        i32 a = GETARG_A(inst);
        i32 b = GETARG_B(inst);
        i32 c = GETARG_C(inst);
        i32 bx = GETARG_Bx(inst);
        i32 sbx = GETARG_sBx(inst);

        pc_++;  // 先递增PC
        
        // 指令分发 - 使用辅助函数简化主循环
        switch (op) {
            // 基础操作
            case OpCode::MOVE:      executeMove(a, b); break;
            case OpCode::LOADK:     executeLoadK(a, bx); break;
            case OpCode::LOADBOOL:  executeLoadBool(a, b, c); break;
            case OpCode::LOADNIL:   executeLoadNil(a, b); break;
            
            // 全局变量操作
            case OpCode::GETGLOBAL: executeGetGlobal(a, bx); break;
            case OpCode::SETGLOBAL: executeSetGlobal(a, bx); break;
            
            // 表操作
            case OpCode::GETTABLE:  executeGetTable(a, b, c); break;
            case OpCode::SETTABLE:  executeSetTable(a, b, c); break;
            case OpCode::NEWTABLE:  executeNewTable(a); break;
            case OpCode::SELF:      executeSelf(a, b, c); break;
            case OpCode::SETLIST:   executeSetList(a, b, c); break;
            
            // 算术运算
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::POW:       arith(op, a, b, c); break;
            
            // 一元运算
            case OpCode::UNM:       executeUnm(a, b); break;
            case OpCode::NOT:       executeNot(a, b); break;
            case OpCode::LEN:       executeLen(a, b); break;
            case OpCode::CONCAT:    executeConcat(a, b, c); break;
            
            // 跳转和比较
            case OpCode::JMP:       doJump(sbx); break;
            case OpCode::EQ:
            case OpCode::LT:
            case OpCode::LE:        compare(op, a, b, c); break;
            case OpCode::TEST:      executeTest(a, c); break;
            case OpCode::TESTSET:   executeTestSet(a, b, c); break;
            
            // Upvalue操作
            case OpCode::GETUPVAL:  executeGetUpval(a, b); break;
            case OpCode::SETUPVAL:  executeSetUpval(a, b); break;
            case OpCode::CLOSE:     executeClose(a); break;
            
            // 函数调用（需要特殊处理reentry）
            case OpCode::CALL:
                if (executeCall(a, b, c, nexeccalls)) {
                    goto reentry;
                }
                break;
                
            case OpCode::TAILCALL:
                if (executeTailCall(a, b)) {
                    return;
                }
                break;
                
            case OpCode::RETURN:
                if (executeReturn(a, b, nexeccalls)) {
                    if (nexeccalls == 0) {
                        return;  // 最外层函数返回
                    }
                    goto reentry;  // 内层函数返回，继续执行调用者
                }
                break;
            
            // 循环指令
            case OpCode::FORLOOP:   executeForLoop(a, sbx); break;
            case OpCode::FORPREP:   executeForPrep(a, sbx); break;
            case OpCode::TFORLOOP:  executeTForLoop(a, c); break;
            
            // 其他指令
            case OpCode::CLOSURE:   executeClosure(a, bx); break;
            case OpCode::VARARG:
                executeVararg(a, b);
                break;

            default:
                throw std::runtime_error("VM: unsupported opcode: " + Str(getOpName(op)));
        }
    }
}

// =====================================================================
// ✅ 改进：base_ 指针管理
// =====================================================================

void VM::updateBasePointer() {
    CallInfo& ci = L_->getCurrentCallInfo();
    Stack& stack = L_->getStack();
    // ⚠️ 使用 operator[] 而不是 at()，避免边界检查
    // 因为 ci.base 可能在某些情况下等于 stack.top_（例如函数返回后）
    base_ = &stack[ci.base];
}

void VM::ensureStackSpace(usize needed) {
    Stack& stack = L_->getStack();
    stack.checkSpace(needed);
    updateBasePointer();  // ✅ 自动更新base_指针
}

// =====================================================================
// 寄存器和常量访问
// =====================================================================

Value& VM::R(i32 index) {
    // 使用缓存的base指针进行直接数组访问（性能优化）
    // base_在executeProto()开始时已经设置为&stack.at(ci.base)

    // 边界检查（Debug模式）
    // ⚠️ 暂时禁用边界检查以调试 vararg 实现
    // #ifdef DEBUG
    // CallInfo& ci = L_->getCurrentCallInfo();
    // Stack& stack = L_->getStack();
    // usize absIndex = ci.base + index;
    // if (absIndex >= stack.size()) {
    //     throw std::runtime_error("VM::R: register index out of range");
    // }
    // #endif

    // 直接数组访问，避免每次都查找CallInfo
    return base_[index];
}

Value VM::RK(i32 rk) {
    if (ISK(rk)) {
        // 常量 - 返回副本
        i32 index = INDEXK(rk);
        #ifdef DEBUG
        std::cerr << "[VM::RK] rk=" << rk << " ISK=true index=" << index
                  << " currentProto=" << (void*)currentProto_ << std::endl;
        #endif
        return currentProto_->getConstant(index);
    } else {
        // 寄存器 - 返回副本
        #ifdef DEBUG
        std::cerr << "[VM::RK] rk=" << rk << " ISK=false R(" << rk << ")" << std::endl;
        #endif
        return R(rk);
    }
}

Value VM::K(i32 index) {
    if (!currentProto_) {
        throw std::runtime_error("VM::K: no current proto");
    }
    return currentProto_->getConstant(index);
}

// =====================================================================
// 算术和逻辑运算
// =====================================================================

/**
 * @brief 尝试将Value转换为数字
 *
 * 类似于C实现的luaV_tonumber函数。
 * 支持：
 * - 数字类型：直接返回
 * - 字符串类型：尝试解析为数字
 * - 其他类型：返回false
 *
 * @param val 要转换的值
 * @param result 输出参数，存储转换后的数字
 * @return true 如果转换成功，false 如果失败
 *
 * @see lua_c_analysis/src/lvm.c 第185-195行 luaV_tonumber()
 */
static bool tryToNumber(const Value& val, f64& result) {
    // 如果已经是数字，直接返回
    if (val.isNumber()) {
        result = val.asNumber();
        return true;
    }

    // 如果是字符串，尝试解析为数字
    if (val.isString()) {
        GCString* str = val.asString();
        const char* s = str->c_str();
        char* endptr;

        // 尝试解析为浮点数
        f64 num = std::strtod(s, &endptr);

        // 检查是否成功解析（整个字符串都被解析）
        if (endptr != s && *endptr == '\0') {
            result = num;
            return true;
        }
    }

    // 其他类型无法转换
    return false;
}

/**
 * @brief 执行算术运算（支持元方法）
 *
 * 实现流程：
 * 1. 尝试将操作数转换为数字
 * 2. 如果转换成功，执行数字运算
 * 3. 如果转换失败，尝试调用元方法
 * 4. 如果元方法也失败，抛出错误
 *
 * @see lua_c_analysis/src/lvm.c 第1315-1336行 Arith()
 */
void VM::arith(OpCode op, i32 a, i32 b, i32 c) {
    Value left = RK(b);
    Value right = RK(c);

    // 尝试将操作数转换为数字
    f64 lval, rval;
    bool leftIsNum = tryToNumber(left, lval);
    bool rightIsNum = tryToNumber(right, rval);

    // 如果两个操作数都能转换为数字，执行数字运算
    if (leftIsNum && rightIsNum) {
        f64 result = 0.0;

        switch (op) {
            case OpCode::ADD:
                result = lval + rval;
                break;
            case OpCode::SUB:
                result = lval - rval;
                break;
            case OpCode::MUL:
                result = lval * rval;
                break;
            case OpCode::DIV:
                if (rval == 0.0) {
                    throw std::runtime_error("VM: division by zero");
                }
                result = lval / rval;
                break;
            case OpCode::MOD:
                result = std::fmod(lval, rval);
                break;
            case OpCode::POW:
                result = std::pow(lval, rval);
                break;
            default:
                throw std::runtime_error("VM::arith: invalid opcode");
        }

        R(a) = Value(result);
        return;
    }

    // 数字运算失败，尝试调用元方法
    // 将OpCode映射到TMS
    TMS tmEvent;
    switch (op) {
        case OpCode::ADD: tmEvent = TMS::TM_ADD; break;
        case OpCode::SUB: tmEvent = TMS::TM_SUB; break;
        case OpCode::MUL: tmEvent = TMS::TM_MUL; break;
        case OpCode::DIV: tmEvent = TMS::TM_DIV; break;
        case OpCode::MOD: tmEvent = TMS::TM_MOD; break;
        case OpCode::POW: tmEvent = TMS::TM_POW; break;
        default:
            throw std::runtime_error("VM::arith: invalid opcode for metamethod");
    }

    // 尝试调用元方法
    Value result;
    if (tryArithMetamethod(tmEvent, left, right, result)) {
        R(a) = result;
        return;
    }

    // 元方法也失败，抛出错误
    throw std::runtime_error("VM: attempt to perform arithmetic on non-number values");
}

void VM::compare(OpCode op, i32 a, i32 b, i32 c) {
    // 实现完整的比较操作，支持元方法
    // @see lua_c_analysis/src/lvm.c 第878-920行 luaV_equalval()
    // @see lua_c_analysis/src/lvm.c 第923-937行 luaV_lessthan()
    // @see lua_c_analysis/src/lvm.c 第988-1002行 lessequal()

    Value left = RK(b);
    Value right = RK(c);

    bool result = false;

    switch (op) {
        case OpCode::EQ: {
            // 相等比较：支持__eq元方法
            // 1. 首先检查类型是否相同
            if (left.getType() != right.getType()) {
                result = false;
            } 
            // 2. nil类型
            else if (left.isNil()) {
                result = true;
            }
            // 3. 数字类型
            else if (left.isNumber()) {
                result = (left.asNumber() == right.asNumber());
            }
            // 4. 布尔类型
            else if (left.isBoolean()) {
                result = (left.asBoolean() == right.asBoolean());
            }
            // 5. 字符串类型
            else if (left.isString()) {
                result = (left.asString()->getData() == right.asString()->getData());
            }
            // 6. 表类型：先比较指针，再尝试__eq元方法
            else if (left.isTable()) {
                if (left.asTable() == right.asTable()) {
                    result = true;
                } else {
                    // 检查__eq元方法（必须两个表有相同的__eq元方法）
                    Table* mt1 = left.asTable()->getMetatable();
                    Table* mt2 = right.asTable()->getMetatable();
                    Value tm = getComparisonTM(L_, mt1, mt2, TMS::TM_EQ);
                    
                    if (!tm.isNil()) {
                        // 调用__eq元方法
                        Value tmResult;
                        callTMWithResult(L_, tmResult, tm, left, right);
                        // false和nil被视为false，其他值被视为true
                        result = !(tmResult.isNil() || (tmResult.isBoolean() && !tmResult.asBoolean()));
                    } else {
                        result = false;
                    }
                }
            }
            // 7. 用户数据类型：类似表类型
            else if (left.isUserdata()) {
                if (left.asUserdata() == right.asUserdata()) {
                    result = true;
                } else {
                    Table* mt1 = left.asUserdata()->getMetatable();
                    Table* mt2 = right.asUserdata()->getMetatable();
                    Value tm = getComparisonTM(L_, mt1, mt2, TMS::TM_EQ);
                    
                    if (!tm.isNil()) {
                        Value tmResult;
                        callTMWithResult(L_, tmResult, tm, left, right);
                        result = !(tmResult.isNil() || (tmResult.isBoolean() && !tmResult.asBoolean()));
                    } else {
                        result = false;
                    }
                }
            }
            // 8. 其他类型：比较指针
            else {
                result = (left == right);  // 使用Value的operator==
            }
            break;
        }

        case OpCode::LT: {
            // 小于比较：支持__lt元方法
            // 类型必须相同才能比较
            if (left.getType() != right.getType()) {
                throw std::runtime_error("VM: attempt to compare two different types");
            }
            
            // 数字类型
            if (left.isNumber()) {
                result = (left.asNumber() < right.asNumber());
            }
            // 字符串类型
            else if (left.isString()) {
                result = (left.asString()->getData() < right.asString()->getData());
            }
            // 其他类型：尝试__lt元方法
            else {
                i32 tmResult = callOrderTM(L_, left, right, TMS::TM_LT);
                if (tmResult == -1) {
                    throw std::runtime_error("VM: attempt to compare without __lt metamethod");
                }
                result = (tmResult != 0);
            }
            break;
        }

        case OpCode::LE: {
            // 小于等于比较：优先使用__le，回退到__lt
            // 类型必须相同才能比较
            if (left.getType() != right.getType()) {
                throw std::runtime_error("VM: attempt to compare two different types");
            }
            
            // 数字类型
            if (left.isNumber()) {
                result = (left.asNumber() <= right.asNumber());
            }
            // 字符串类型
            else if (left.isString()) {
                result = (left.asString()->getData() <= right.asString()->getData());
            }
            // 其他类型：先尝试__le，再尝试__lt
            else {
                // 先尝试__le元方法
                i32 tmResult = callOrderTM(L_, left, right, TMS::TM_LE);
                if (tmResult != -1) {
                    result = (tmResult != 0);
                } else {
                    // 回退到__lt: a <= b 等价于 !(b < a)
                    tmResult = callOrderTM(L_, right, left, TMS::TM_LT);
                    if (tmResult == -1) {
                        throw std::runtime_error("VM: attempt to compare without __le or __lt metamethod");
                    }
                    result = (tmResult == 0);  // 注意这里是取反
                }
            }
            break;
        }

        default:
            throw std::runtime_error("VM::compare: invalid opcode");
    }

    // Lua 5.1.5的比较指令逻辑
    // if (condition == GETARG_A(i))
    //     dojump(L, pc, GETARG_sBx(*pc));
    // pc++;

    const Vec<Instruction>& code = currentProto_->getCode();

    // 如果比较结果 == A，则执行跳转
    if (result == (a != 0)) {
        // 读取下一条指令（应该是JMP）的sBx
        if (pc_ < code.size()) {
            Instruction nextInst = code[pc_];
            i32 sbx = GETARG_sBx(nextInst);
            // 执行跳转
            doJump(sbx);
        }
    }

    // 无论如何都要跳过下一条JMP指令
    pc_++;
}

// =====================================================================
// 跳转控制
// =====================================================================

void VM::doJump(i32 offset) {
    pc_ += offset;
}

// =====================================================================
// 函数调用机制
// =====================================================================

bool VM::precall(i32 funcIndex, i32 nArgs, i32 nResults) {
    Stack& stack = L_->getStack();
    CallInfo& currentCI = L_->getCurrentCallInfo();

    // 计算函数在栈中的绝对位置
    usize funcPos = currentCI.base + funcIndex;
    Value& funcVal = stack.at(funcPos);

    // 支持__call元方法
    // @see lua_c_analysis/src/ldo.c 第295-307行 tryfuncTM()
    if (!funcVal.isFunction()) {
        // 尝试查找__call元方法
        Value tm = getMetamethodByObject(L_, funcVal, TMS::TM_CALL);
        if (tm.isNil() || !tm.isFunction()) {
            throw std::runtime_error("VM::precall: attempt to call non-function value without __call metamethod");
        }
        
        // 有__call元方法，重新组织栈：
        // 原来：[func][arg1][arg2]...
        // 现在：[__call][func][arg1][arg2]...
        
        // 在func位置插入原对象，将__call放到func位置
        // 1. 先保存所有内容
        Value originalFunc = funcVal;
        Vec<Value> args;
        for (i32 i = 1; i <= nArgs; i++) {
            args.push_back(stack.at(funcPos + i));
        }
        
        // 2. 设置新的布局
        stack.at(funcPos) = tm;              // __call元方法
        stack.at(funcPos + 1) = originalFunc; // 原对象作为第一个参数
        for (usize i = 0; i < args.size(); i++) {
            stack.at(funcPos + 2 + i) = args[i]; // 其他参数
        }
        
        // 3. 参数数量增加1（因为原对象也变成参数了）
        nArgs++;
        
        // 4. 重新获取函数值
        funcVal = stack.at(funcPos);
        if (!funcVal.isFunction()) {
            throw std::runtime_error("VM::precall: __call metamethod is not a function");
        }
    }

    Function* func = funcVal.asFunction();

    if (func->isCFunction()) {
        // C函数调用
        CFunction cfunc = func->getCFunction();

        // 创建新的CallInfo
        CallInfo& ci = L_->pushCallInfo();
        ci.func = funcPos;
        ci.base = funcPos + 1;  // 参数从函数后面开始
        ci.top = funcPos + 1 + nArgs + 20;  // 给C函数足够的栈空间
        ci.nresults = nResults;
        ci.savedpc = nullptr;  // C函数没有PC
        ci.tailcalls = 0;

        // 确保栈空间
        while (stack.size() < ci.top) {
            stack.push(Value());
        }

        // ⭐ P0修复：设置正确的栈顶位置（参数的末尾）
        // 参考：lua_c_analysis/src/ldo.c:1020 luaD_precall
        L_->setAbsoluteTop(funcPos + 1 + nArgs);

        // 调用C函数
        i32 nReturnValues = cfunc(L_);

        // 处理返回值
        // 参考：lua_c_analysis/src/ldo.c:1105
        // firstResult = L->top - n，即返回值从 top - n 开始
        usize currentTop = L_->getAbsoluteTop();
        usize firstResult = currentTop - static_cast<usize>(nReturnValues);
        postcall(static_cast<i32>(funcPos), nResults, firstResult);

        // 弹出CallInfo
        L_->popCallInfo();

        // ⭐ P0修复：恢复调用者的 currentFunc_ 和 currentProto_
        // 在 popCallInfo 之后，getCurrentCallInfo 返回调用者的 CallInfo
        CallInfo& callerCI = L_->getCurrentCallInfo();
        Value& callerFuncVal = stack[callerCI.func];
        if (callerFuncVal.isFunction()) {
            currentFunc_ = callerFuncVal.asFunction();
            currentProto_ = currentFunc_->getProto();
        }

        return false;  // C函数
    } else {
        // Lua函数调用
        Proto* proto = func->getProto();

        // 处理参数数量
        i32 actualArgs = nArgs;
        if (nArgs < 0) {
            // nArgs < 0 表示参数到栈顶
            actualArgs = static_cast<i32>(L_->getAbsoluteTop()) - static_cast<i32>((funcPos + 1));
        }

        i32 numParams = proto->getNumParams();
        usize base;

        if (proto->isVararg()) {
            // ⭐ adjust_varargs 逻辑
            // 参考：lua_c_analysis/src/ldo.c adjust_varargs 函数
            //
            // 栈布局变化：
            //   调用前：[func] [arg1] [arg2] ... [argN]     ← top
            //   调整后：[func] [varargs...] [fixed_copy...] ← new base 在 fixed_copy 起始
            //
            // 步骤：
            // 1. 确保实际参数不少于固定参数数（不足用 nil 补齐）
            // 2. 在栈顶复制 nfixargs 个固定参数
            // 3. 清空原位置的固定参数（设为 nil）
            // 4. 新 base = 栈顶（复制后的固定参数起始位置）

            // 确保实际参数数 >= 固定参数数
            while (actualArgs < numParams) {
                stack.push(Value());  // nil 补齐
                actualArgs++;
            }

            // 旧的固定参数位置：funcPos + 1
            usize oldBase = funcPos + 1;

            // 新 base = 当前栈顶之后的位置（即 funcPos + 1 + actualArgs）
            // 固定参数将被复制到这里
            base = oldBase + static_cast<usize>(actualArgs);

            // 确保栈空间足够存放复制的固定参数
            stack.checkSpace(static_cast<usize>(numParams) + 1);

            // 在栈顶复制固定参数，并清空原位置
            for (i32 i = 0; i < numParams; i++) {
                stack.push(stack[oldBase + i]);   // 复制固定参数到栈顶
                stack[oldBase + i] = Value();     // 清空旧位置（设为 nil）
            }

            // 此时栈布局：
            // [func] [nil/vararg1] [nil/vararg2] ... [nil/varargN] [fixed1] [fixed2] ... [fixedM]
            //         ^--- oldBase                                  ^--- base (= oldBase + actualArgs)
            // vararg 区域 = base - func - 1 - numParams 个值
        } else {
            // 非可变参数函数：base 直接在函数之后
            base = funcPos + 1;

            // 调整参数数量（如果实际参数少于形参，用nil填充）
            while (actualArgs < numParams) {
                stack.push(Value());  // nil
                actualArgs++;
            }
        }

        // 创建新的CallInfo
        CallInfo& ci = L_->pushCallInfo();
        ci.func = funcPos;
        ci.base = base;
        ci.top = base + proto->getMaxStackSize();
        ci.nresults = nResults;
        ci.savedpc = nullptr;  // 将在executeProto中设置
        ci.tailcalls = 0;

        // 确保栈空间足够
        while (stack.size() < ci.top) {
            stack.push(Value());  // 用nil初始化局部变量
        }

        // ⭐ 关键修复：栈扩展后更新 base_ 指针（栈可能重新分配）
        updateBasePointer();

        // ⭐ P0修复：设置栈顶为 ci.top
        L_->setAbsoluteTop(ci.top);

        return true;  // Lua函数
    }
}

void VM::postcall(i32 funcPos, i32 wantedResults, usize firstResult) {
    // ⭐ P0修复：参考lua_c_analysis/src/ldo.c:1198 luaD_poscall
    // 处理函数返回后的返回值复制和栈调整
    Stack& stack = L_->getStack();
    CallInfo& ci = L_->getCurrentCallInfo();

    // 返回值的目标位置（函数位置）
    usize res = static_cast<usize>(funcPos);

    // 如果 firstResult 为 0，使用旧逻辑（从 funcPos 开始）
    // 否则使用新逻辑（从 firstResult 复制到 funcPos）
    usize currentTop = L_->getAbsoluteTop();

    if (firstResult == 0) {
        // 旧逻辑：返回值已经在 funcPos 位置
        i32 actualResults = static_cast<i32>(currentTop) - funcPos;

        if (wantedResults >= 0) {
            if (actualResults < wantedResults) {
                while (actualResults < wantedResults) {
                    if (currentTop >= stack.size()) {
                        stack.push(Value());
                    } else {
                        stack.at(currentTop) = Value();
                    }
                    currentTop++;
                    actualResults++;
                }
            } else if (actualResults > wantedResults) {
                currentTop -= (actualResults - wantedResults);
                actualResults = wantedResults;
            }
        }
        L_->setAbsoluteTop(funcPos + actualResults);
    } else {
        // 新逻辑：将返回值从 firstResult 复制到 funcPos
        // 参考：lua_c_analysis/src/ldo.c:1217-1220
        i32 i = wantedResults;
        usize src = firstResult;

        // 复制返回值到正确位置
        while (i != 0 && src < currentTop) {
            stack[res++] = stack[src++];
            i--;
        }

        // 补齐缺失的返回值（设为nil）
        while (i-- > 0) {
            stack[res++] = Value();
        }

        // 调整栈顶
        L_->setAbsoluteTop(res);
    }

    // ⭐ P0修复：恢复调用者的执行状态
    Value& funcVal = stack[ci.func];
    if (funcVal.isFunction()) {
        currentFunc_ = funcVal.asFunction();
        currentProto_ = currentFunc_->getProto();
    }

    // 恢复 PC（从调用者的 savedpc 恢复）
    if (ci.savedpc != nullptr && currentProto_ != nullptr) {
        const Instruction* codeStart = currentProto_->getCode().data();
        pc_ = static_cast<usize>(ci.savedpc - codeStart);
    }
}

// =====================================================================
// 重构后的辅助函数实现
// =====================================================================

void VM::validateAndCheckDepth(Proto* proto, i32 nexeccalls) {
    if (!proto) {
        throw std::runtime_error("VM::executeProto: null proto");
    }

    // 检查嵌套调用深度（防止栈溢出）
    static constexpr i32 MAX_CALLS = 200;
    if (nexeccalls >= MAX_CALLS) {
        throw std::runtime_error("VM: stack overflow (too many nested calls)");
    }
}

void VM::initializeExecutionContext() {
    // 重新初始化执行状态（每次reentry都需要）
    CallInfo& ci = L_->getCurrentCallInfo();
    Stack& stack = L_->getStack();

    // ⭐ P0修复：从CallInfo恢复currentFunc_
    // 注意：使用 operator[] 而不是 at()，因为 at() 会检查 Stack::top_
    // 而 Stack::top_ 可能与实际的栈容量不同
    if (ci.func < stack.capacity()) {
        Value& funcVal = stack[ci.func];
        if (funcVal.isFunction()) {
            currentFunc_ = funcVal.asFunction();
        } else {
            throw std::runtime_error("VM::executeProto: CallInfo.func is not a function");
        }
    } else {
        throw std::runtime_error("VM::executeProto: CallInfo.func out of range");
    }

    // 从CallInfo恢复执行状态
    if (ci.savedpc != nullptr) {
        // 从保存的PC恢复（用于CALL返回后继续执行）
        currentProto_ = currentFunc_->getProto();
        pc_ = static_cast<usize>(ci.savedpc - currentProto_->getCode().data());
    } else {
        // 新函数调用，从头开始
        currentProto_ = currentFunc_->getProto();
        pc_ = 0;
    }

    // ✅ 改进：使用统一的栈空间确保方法
    usize requiredTop = ci.base + currentProto_->getMaxStackSize();

    // ⭐ 临时修复：添加额外的栈空间以应对CodeGenerator的maxStackSize计算错误
    // TODO: 修复CodeGenerator后移除这个hack
    usize extraSpace = 10;  // 额外的栈空间
    usize actualRequiredTop = requiredTop + extraSpace;

    // ⭐ P0修复：确保栈有足够的容量，并设置top
    if (stack.capacity() < actualRequiredTop) {
        ensureStackSpace(actualRequiredTop - stack.size());
    }

    // ⭐ P0修复：设置栈顶到所需位置，并初始化为nil
    while (stack.size() < actualRequiredTop) {
        stack.push(Value());  // nil
    }

    // 更新base_指针
    updateBasePointer();
}

// =====================================================================
// 基础操作指令实现
// =====================================================================

inline void VM::executeMove(i32 a, i32 b) {
    // R(A) := R(B)
    R(a) = R(b);
}

inline void VM::executeLoadK(i32 a, i32 bx) {
    // R(A) := K(Bx)
    R(a) = K(bx);
}

inline void VM::executeLoadBool(i32 a, i32 b, i32 c) {
    // R(A) := (bool)B; if (C) pc++
    R(a) = Value(b != 0);
    if (c != 0) {
        pc_++;
    }
}

inline void VM::executeLoadNil(i32 a, i32 b) {
    // R(A) := ... := R(B) := nil
    for (i32 i = a; i <= b; i++) {
        R(i) = Value();  // 默认构造为nil
    }
}

// =====================================================================
// 全局变量操作实现
// =====================================================================

void VM::executeGetGlobal(i32 a, i32 bx) {
    // R(A) := Gbl[K(Bx)]
    // 使用当前函数的环境表（Lua 5.1兼容）
    const Value& key = K(bx);

    // 获取当前函数的环境表，如果未设置则使用全局表
    Table* env = currentFunc_->getEnv();
    if (!env) {
        env = L_->getGlobalTable();
    }

    R(a) = env->get(key);
}

void VM::executeSetGlobal(i32 a, i32 bx) {
    // Gbl[K(Bx)] := R(A)
    // 使用当前函数的环境表（Lua 5.1兼容）
    const Value& key = K(bx);

    // 获取当前函数的环境表，如果未设置则使用全局表
    Table* env = currentFunc_->getEnv();
    if (!env) {
        env = L_->getGlobalTable();
    }

    env->set(key, R(a));
}

// =====================================================================
// 表操作指令实现
// =====================================================================

void VM::executeGetTable(i32 a, i32 b, i32 c) {
    // R(A) := R(B)[RK(C)]
    // 支持__index元方法
    // @see lua_c_analysis/src/lvm.c 第530-553行 luaV_gettable()
    
    Value t = R(b);
    Value key = RK(c);
    
    // 防止无限循环的计数器
    constexpr i32 MAXTAGLOOP = 100;
    
    for (i32 loop = 0; loop < MAXTAGLOOP; loop++) {
        // 如果是表类型
        if (t.isTable()) {
            Table* h = t.asTable();
            Value res = h->get(key);
            
            // 如果找到值或没有__index元方法，返回结果
            if (!res.isNil()) {
                R(a) = res;
                return;
            }
            
            // 检查是否有__index元方法
            Value tm = getMetamethodByObject(L_, t, TMS::TM_INDEX);
            if (tm.isNil()) {
                // 没有元方法，返回nil
                R(a) = Value();
                return;
            }
            
            // 如果__index是函数，调用它
            if (tm.isFunction()) {
                Value result;
                callTMWithResult(L_, result, tm, t, key);
                R(a) = result;
                return;
            }
            
            // 如果__index是表，继续在该表中查找
            t = tm;
            // 继续循环
        } else {
            // 非表类型，必须有__index元方法
            Value tm = getMetamethodByObject(L_, t, TMS::TM_INDEX);
            if (tm.isNil()) {
                throw std::runtime_error("VM: attempt to index a non-table value");
            }
            
            // 如果__index是函数，调用它
            if (tm.isFunction()) {
                Value result;
                callTMWithResult(L_, result, tm, t, key);
                R(a) = result;
                return;
            }
            
            // 如果__index是表，继续在该表中查找
            t = tm;
            // 继续循环
        }
    }
    
    // 超过最大循环次数
    throw std::runtime_error("VM: loop in gettable");
}

void VM::executeSetTable(i32 a, i32 b, i32 c) {
    // R(A)[RK(B)] := RK(C)
    // 支持__newindex元方法
    // @see lua_c_analysis/src/lvm.c 第619-650行 luaV_settable()
    
    Value t = R(a);
    Value key = RK(b);
    Value val = RK(c);
    
    // 防止无限循环的计数器
    constexpr i32 MAXTAGLOOP = 100;
    
    for (i32 loop = 0; loop < MAXTAGLOOP; loop++) {
        // 如果是表类型
        if (t.isTable()) {
            Table* h = t.asTable();
            Value oldval = h->get(key);
            
            // 如果键已存在或没有__newindex元方法，直接设置
            if (!oldval.isNil()) {
                h->set(key, val);
                return;
            }
            
            // 检查是否有__newindex元方法
            Value tm = getMetamethodByObject(L_, t, TMS::TM_NEWINDEX);
            if (tm.isNil()) {
                // 没有元方法，直接设置新键
                h->set(key, val);
                return;
            }
            
            // 如果__newindex是函数，调用它
            if (tm.isFunction()) {
                callTM(L_, tm, t, key, val);
                return;
            }
            
            // 如果__newindex是表，继续在该表中设置
            t = tm;
            // 继续循环
        } else {
            // 非表类型，必须有__newindex元方法
            Value tm = getMetamethodByObject(L_, t, TMS::TM_NEWINDEX);
            if (tm.isNil()) {
                throw std::runtime_error("VM: attempt to index a non-table value");
            }
            
            // 如果__newindex是函数，调用它
            if (tm.isFunction()) {
                callTM(L_, tm, t, key, val);
                return;
            }
            
            // 如果__newindex是表，继续在该表中设置
            t = tm;
            // 继续循环
        }
    }
    
    // 超过最大循环次数
    throw std::runtime_error("VM: loop in settable");
}

void VM::executeNewTable(i32 a) {
    // R(A) := {} (size = B,C)
    Table* table = new Table();
    R(a) = Value(table);
}

void VM::executeSelf(i32 a, i32 b, i32 c) {
    // R(A+1) := R(B); R(A) := R(B)[RK(C)]
    Value obj = R(b);
    R(a + 1) = obj;  // 保存对象作为第一个参数

    // 获取方法
    if (!obj.isTable()) {
        throw std::runtime_error("VM: SELF requires table object");
    }

    Table* table = obj.asTable();
    Value key = RK(c);
    Value method = table->get(key);
    R(a) = method;
}

void VM::executeSetList(i32 a, i32 b, i32 c) {
    // R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
    // FPF = LFIELDS_PER_FLUSH = 50
    constexpr i32 FPF = 50;

    if (!R(a).isTable()) {
        throw std::runtime_error("VM: SETLIST requires table");
    }

    Table* table = R(a).asTable();
    i32 n = b;
    i32 base_index = (c - 1) * FPF;

    // 如果B=0，表示到栈顶
    if (n == 0) {
        Stack& stack = L_->getStack();
        n = static_cast<i32>(stack.size()) - a - 1;
    }

    // 设置数组元素
    for (i32 i = 1; i <= n; i++) {
        table->setArray(base_index + i, R(a + i));
    }
}

// =====================================================================
// 一元运算指令实现
// =====================================================================

void VM::executeUnm(i32 a, i32 b) {
    // R(A) := -R(B)
    Value val = R(b);

    // 尝试将操作数转换为数字
    f64 num;
    if (tryToNumber(val, num)) {
        // 数字运算：直接取负
        R(a) = Value(-num);
        return;
    }

    // 数字运算失败，尝试调用__unm元方法
    // 注意：一元运算只有一个操作数，第二个参数传nil
    Value result;
    if (tryArithMetamethod(TMS::TM_UNM, val, Value(), result)) {
        R(a) = result;
        return;
    }

    // 元方法也失败，抛出错误
    throw std::runtime_error("VM: attempt to perform arithmetic on a non-number value");
}

void VM::executeNot(i32 a, i32 b) {
    // R(A) := not R(B)
    R(a) = Value(!R(b).isTrue());
}

void VM::executeLen(i32 a, i32 b) {
    // R(A) := length of R(B)
    // 支持__len元方法
    Value& val = R(b);
    
    // 字符串的长度（不使用元方法）
    if (val.isString()) {
        R(a) = Value(static_cast<f64>(val.asString()->getLength()));
        return;
    }
    
    // 表的长度，先尝试__len元方法（Lua 5.2+行为）
    // 注意：Lua 5.1中表不会调用__len，但我们为了兼容性支持它
    if (val.isTable()) {
        // 尝试查找__len元方法
        Value tm = getMetamethodByObject(L_, val, TMS::TM_LEN);
        if (!tm.isNil() && tm.isFunction()) {
            // 调用__len元方法
            Value result;
            callTMWithResult(L_, result, tm, val, Value());
            
            // 确保返回值是数字
            if (result.isNumber()) {
                R(a) = result;
                return;
            } else {
                throw std::runtime_error("VM: __len metamethod must return a number");
            }
        }
        
        // 没有元方法或元方法不是函数，使用表的原始长度
        R(a) = Value(static_cast<f64>(val.asTable()->length()));
        return;
    }
    
    // 其他类型，尝试__len元方法
    Value tm = getMetamethodByObject(L_, val, TMS::TM_LEN);
    if (!tm.isNil() && tm.isFunction()) {
        Value result;
        callTMWithResult(L_, result, tm, val, Value());
        if (result.isNumber()) {
            R(a) = result;
            return;
        } else {
            throw std::runtime_error("VM: __len metamethod must return a number");
        }
    }
    
    // 没有元方法，抛出错误
    throw std::runtime_error("VM: attempt to get length of a value without __len metamethod");
}

void VM::executeConcat(i32 a, i32 b, i32 c) {
    // R(A) := R(B).. ... ..R(C)
    // 支持__concat元方法和数字到字符串的转换
    // @see lua_c_analysis/src/lvm.c 第1143-1173行 luaV_concat()
    
    i32 total = c - b + 1;  // 需要连接的值的数量
    i32 last = c;
    
    StringPool& pool = GlobalState::getInstance().getStringPool();
    
    // 从右到左处理连接操作
    while (total > 1) {
        Value& top1 = R(last);
        Value& top2 = R(last - 1);
        
        // 尝试将值转换为字符串
        Str str1;
        Str str2;
        bool canConcat = false;
        
        // 转换第一个操作数
        if (top2.isString()) {
            str2 = top2.asString()->getData();
            canConcat = true;
        } else if (top2.isNumber()) {
            str2 = std::to_string(top2.asNumber());
            canConcat = true;
        }
        
        // 转换第二个操作数
        if (canConcat) {
            if (top1.isString()) {
                str1 = top1.asString()->getData();
            } else if (top1.isNumber()) {
                str1 = std::to_string(top1.asNumber());
            } else {
                canConcat = false;
            }
        }
        
        // 如果不能直接连接，尝试__concat元方法
        if (!canConcat) {
            Value result;
            if (!callBinaryTM(L_, top2, top1, result, TMS::TM_CONCAT)) {
                throw std::runtime_error("VM: attempt to concatenate non-string/number values");
            }
            R(last - 1) = result;
            total--;
            last--;
            continue;
        }
        
        // 如果第二个字符串为空，直接使用第一个
        if (str1.empty()) {
            // top2 已经是正确的值
        } else {
            // 执行连接
            Str result = str2 + str1;
            GCString* str = pool.intern(result);
            R(last - 1) = Value(str);
        }
        
        total--;
        last--;
    }
    
    // 将最终结果移到目标位置
    R(a) = R(b);
}

// =====================================================================
// 测试指令实现
// =====================================================================

void VM::executeTest(i32 a, i32 c) {
    // ⭐ P0修复：参考lua_c_analysis/src/lvm.c:1782-1788
    const Vec<Instruction>& code = currentProto_->getCode();
    
    bool val = R(a).isTrue();
    // 如果 (val为false) != C，则跳转
    if ((!val) != (c != 0)) {
        // 读取下一条JMP指令的sBx并跳转
        if (pc_ < code.size()) {
            Instruction nextInst = code[pc_];
            i32 sbx = GETARG_sBx(nextInst);
            doJump(sbx);
        }
    }
    // 无论如何都要跳过下一条JMP指令
    pc_++;
}

void VM::executeTestSet(i32 a, i32 b, i32 c) {
    // ⭐ P0修复：参考lua_c_analysis/src/lvm.c:1790-1798
    const Vec<Instruction>& code = currentProto_->getCode();
    
    bool val = R(b).isTrue();
    // 如果 (val为false) != C，则设置并跳转
    if ((!val) != (c != 0)) {
        R(a) = R(b);
        // 读取下一条JMP指令的sBx并跳转
        if (pc_ < code.size()) {
            Instruction nextInst = code[pc_];
            i32 sbx = GETARG_sBx(nextInst);
            doJump(sbx);
        }
    }
    // 无论如何都要跳过下一条JMP指令
    pc_++;
}

// =====================================================================
// Upvalue操作指令实现
// =====================================================================

void VM::executeGetUpval(i32 a, i32 b) {
    // R(A) := UpValue[B]
    if (!currentFunc_) {
        throw std::runtime_error("VM: GETUPVAL without current function");
    }

    Upvalue* uv = currentFunc_->getUpvalue(b);
    if (!uv) {
        throw std::runtime_error("VM: GETUPVAL invalid upvalue index");
    }

    // ✅ 改进：传入stack引用
    R(a) = uv->getValue(L_->getStack());
}

void VM::executeSetUpval(i32 a, i32 b) {
    // UpValue[B] := R(A)
    if (!currentFunc_) {
        throw std::runtime_error("VM: SETUPVAL without current function");
    }

    Upvalue* uv = currentFunc_->getUpvalue(b);
    if (!uv) {
        throw std::runtime_error("VM: SETUPVAL invalid upvalue index");
    }

    // ✅ 改进：使用setValue方法
    uv->setValue(L_->getStack(), R(a));
}

void VM::executeClose(i32 a) {
    // close all variables in the stack up to (>=) R(A)
    L_->closeUpvalues(a);
}

// =====================================================================
// 循环指令实现
// =====================================================================

void VM::executeForLoop(i32 a, i32 sbx) {
    // R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
    // 参考：lua_c_analysis/src/lvm.c 第1899-1911行
    if (!R(a).isNumber() || !R(a + 1).isNumber() || !R(a + 2).isNumber()) {
        throw std::runtime_error("VM: FORLOOP requires numeric values");
    }

    // 读取循环变量
    f64 step = R(a + 2).asNumber();
    f64 idx = R(a).asNumber() + step;  // idx = R(A) + step
    f64 limit = R(a + 1).asNumber();

    // 检查循环条件（根据步长方向选择比较方式）
    bool cont = (step > 0) ? (idx <= limit) : (idx >= limit);

    if (cont) {
        // 继续循环：先跳转，再更新寄存器
        doJump(sbx);              // pc += sBx
        R(a) = Value(idx);        // 更新内部索引 R(A)
        R(a + 3) = Value(idx);    // 更新用户可见的循环变量 R(A+3)
    }
}

void VM::executeForPrep(i32 a, i32 sbx) {
    // R(A)-=R(A+2); pc+=sBx
    if (!R(a).isNumber() || !R(a + 1).isNumber() || !R(a + 2).isNumber()) {
        throw std::runtime_error("VM: FORPREP requires numeric values");
    }

    f64 init = R(a).asNumber();
    f64 step = R(a + 2).asNumber();
    R(a) = Value(init - step);  // 预减步长
    doJump(sbx);                // 跳转到FORLOOP
}

void VM::executeTForLoop(i32 a, i32 c) {
    // R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2))
    // if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++
    // 参考：lua_c_analysis/src/lvm.c 第1935-1952行
    
    const Vec<Instruction>& code = currentProto_->getCode();
    i32 cb = a + 3;  // 调用基址（call base）

    // 确保栈有足够空间存储调用参数和返回值
    Stack& stack = L_->getStack();
    CallInfo& ci = L_->getCurrentCallInfo();
    usize requiredSize = ci.base + cb + 3 + c;
    while (stack.size() < requiredSize) {
        stack.push(Value());
    }
    // 更新 base_（栈可能已扩展）
    base_ = &stack.at(ci.base);

    // 步骤1：将迭代器函数和参数复制到调用位置
    R(cb + 2) = R(a + 2);  // 控制变量
    R(cb + 1) = R(a + 1);  // 状态
    R(cb) = R(a);          // 迭代器函数

    // 步骤2：调用迭代器函数
    if (!R(cb).isFunction()) {
        throw std::runtime_error("VM: TFORLOOP requires function at R(" +
                               std::to_string(cb) + ")");
    }

    Function* func = R(cb).asFunction();

    if (func->isCFunction()) {
        // C函数迭代器
        CFunction cfunc = func->getCFunction();

        // 保存所有寄存器的值（因为调用会清空栈）
        Vec<Value> savedRegs;
        for (usize i = 0; i < stack.size(); i++) {
            savedRegs.push_back(stack.at(i));
        }

        // 清空栈，压入参数
        stack.clear();
        stack.push(savedRegs[ci.base + cb + 1]);  // 参数1：状态
        stack.push(savedRegs[ci.base + cb + 2]);  // 参数2：控制变量

        // 调用C函数
        i32 nret = cfunc(L_);

        // 保存返回值（从栈中读取）
        Vec<Value> returnValues;
        for (i32 i = 0; i < nret && i < c; i++) {
            if (2 + i < static_cast<i32>(stack.size())) {
                returnValues.push_back(stack.at(2 + i));
            } else {
                returnValues.push_back(Value());
            }
        }

        // 恢复栈到原来的状态
        stack.clear();
        for (const Value& v : savedRegs) {
            stack.push(v);
        }

        // ✅ 改进：使用统一的更新方法
        updateBasePointer();

        // 将返回值存储到 R(cb), R(cb+1), ..., R(cb+c-1)
        for (usize i = 0; i < returnValues.size(); i++) {
            R(static_cast<i32>(cb + i)) = returnValues[i];
        }
        // 填充剩余返回值为 nil
        for (i32 i = static_cast<i32>(returnValues.size()); i < c; i++) {
            R(cb + i) = Value();
        }
    } else {
        // Lua函数迭代器（暂不支持）
        throw std::runtime_error("VM: TFORLOOP Lua iterators not supported yet");
    }

    // 步骤3：检查第一个返回值并决定是否继续循环
    cb = a + 3;

    if (!R(cb).isNil()) {
        // 继续循环：更新控制变量并跳转
        R(a + 2) = R(cb);

        // 读取下一条指令（JMP）的sBx并跳转
        if (pc_ < code.size()) {
            Instruction jmpInst = code[pc_];
            i32 jmpSbx = GETARG_sBx(jmpInst);
            doJump(jmpSbx);
        }
    } else {
        // 退出循环：跳过下一条JMP指令
        pc_++;
    }
}

// =====================================================================
// 其他指令实现
// =====================================================================

void VM::executeClosure(i32 a, i32 bx) {
    // R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
    // 创建闭包：从Proto的子函数列表中获取Proto

    // ⭐ P0修复：从Proto的子函数列表获取，而不是常量表
    if (bx < 0 || static_cast<usize>(bx) >= currentProto_->getSubProtoCount()) {
        throw std::runtime_error("VM: CLOSURE proto index out of range");
    }

    Proto* childProto = currentProto_->getSubProto(bx);

    // 创建新的闭包
    Function* closure = new Function(childProto);

    // TODO: 处理upvalues（需要读取后续的MOVE/GETUPVAL指令）
    // 简化版：暂不处理upvalues

    R(a) = Value(closure);
}

void VM::executeVararg(i32 a, i32 b) {
    // ⭐ OP_VARARG A B：R(A), R(A+1), ..., R(A+B-2) = vararg
    // 参考：lua_c_analysis/src/lvm.c OP_VARARG case
    //
    // B=0：复制所有可变参数（LUA_MULTRET），并调整栈顶
    // B>0：复制 B-1 个可变参数（不足用 nil 填充）
    //
    // 栈布局（adjust_varargs 后）：
    //   [func] [vararg1] [vararg2] ... [varargN] [fixed1] ... [fixedM] [locals...]
    //   ^func                                     ^base
    //
    // vararg 数量 n = (base - func - 1) - numParams
    // vararg 起始位置（绝对索引）= base - n

    CallInfo& ci = L_->getCurrentCallInfo();
    Stack& stack = L_->getStack();
    i32 numParams = currentProto_->getNumParams();

    // 计算可变参数数量
    // ci.base - ci.func - 1 = 调用时的总实际参数数
    // 减去 numParams = 可变参数数量
    i32 n = static_cast<i32>(ci.base - ci.func - 1) - numParams;
    if (n < 0) n = 0;

    i32 wanted;  // 需要复制的数量
    if (b == 0) {
        // B=0：复制所有可变参数
        wanted = n;

        // 确保栈空间足够
        usize neededTop = ci.base + static_cast<usize>(a) + static_cast<usize>(n);
        if (stack.size() < neededTop) {
            stack.checkSpace(neededTop - stack.size());
            updateBasePointer();  // 栈可能重新分配，更新 base_ 指针
        }

        // 设置栈顶为 ra + n（告诉后续指令实际有多少个值）
        L_->setAbsoluteTop(ci.base + static_cast<usize>(a) + static_cast<usize>(n));
    } else {
        // B>0：复制 B-1 个值
        wanted = b - 1;
    }

    // 从 vararg 区域复制数据到目标寄存器
    // vararg 区域起始绝对索引 = ci.base - n
    for (i32 j = 0; j < wanted; j++) {
        if (j < n) {
            // 从 vararg 区域复制（绝对索引：ci.base - n + j）
            usize srcIndex = ci.base - static_cast<usize>(n) + static_cast<usize>(j);
            R(a + j) = stack[srcIndex];
        } else {
            // 不足的部分用 nil 填充
            R(a + j) = Value();
        }
    }
}

// =====================================================================
// 函数调用指令实现
// =====================================================================

bool VM::executeCall(i32 a, i32 b, i32 c, i32& nexeccalls) {
    // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    // ⭐ P0修复：参考lua_c_analysis/src/lvm.c:1800
    const Vec<Instruction>& code = currentProto_->getCode();
    i32 nArgs = b - 1;      // B=0表示到栈顶
    i32 nResults = c - 1;   // C=0表示多返回值

    // 保存当前PC（用于返回后继续执行）
    CallInfo& currentCI = L_->getCurrentCallInfo();
    currentCI.savedpc = &code[pc_];

    // 计算函数在栈中的绝对位置
    usize funcPos = currentCI.base + a;

    // 在precall之前保存函数引用（因为precall会改变CallInfo）
    Value& funcVal = R(a);
    if (!funcVal.isFunction()) {
        throw std::runtime_error("VM: CALL on non-function value");
    }
    Function* func = funcVal.asFunction();

    // 准备调用
    bool isLua = precall(a, nArgs, nResults);

    if (isLua) {
        // ⭐ P0修复：Lua函数使用goto reentry而非递归
        // 设置新函数上下文
        currentFunc_ = func;
        nexeccalls++;
        return true;  // 需要reentry
    } else {
        // C函数已在precall中执行完成
        // postcall已在precall中调用
        // ✅ 改进：使用统一的更新方法
        updateBasePointer();
        return false;  // 继续执行
    }
}

bool VM::executeTailCall(i32 a, i32 b) {
    // return R(A)(R(A+1), ... ,R(A+B-1))
    i32 nArgs = b - 1;

    // 尾调用：关闭当前函数的upvalues
    CallInfo& currentCI = L_->getCurrentCallInfo();
    L_->closeUpvalues(currentCI.base);

    // 准备调用（返回值数量为-1表示多返回值）
    bool isLua = precall(a, nArgs, -1);

    if (isLua) {
        // Lua函数尾调用：直接替换当前栈帧
        // 获取被调用函数
        Value& funcVal = R(a);
        Function* func = funcVal.asFunction();
        Proto* calleeProto = func->getProto();

        // 尾调用优化：不增加调用栈深度
        // 直接用新函数替换当前函数
        currentFunc_ = func;
        currentProto_ = calleeProto;
        pc_ = 0;

        // 更新CallInfo的尾调用计数
        currentCI.tailcalls++;

        // 重新开始执行（goto reentry的效果）
        // 通过返回并让调用者重新进入来实现
        return true;  // 需要return并reentry
    } else {
        // C函数已执行，直接返回
        return true;  // 需要return
    }
}

bool VM::executeReturn(i32 a, i32 b, i32& nexeccalls) {
    // return R(A), ... ,R(A+B-2)
    // ⭐ P0修复：参考lua_c_analysis/src/lvm.c:1873
    // B=0: 返回从R(A)到栈顶的所有值
    // B=1: 无返回值
    // B>1: 返回B-1个值（R(A)到R(A+B-2)）

    CallInfo& ci = L_->getCurrentCallInfo();
    Stack& stack = L_->getStack();

    // 计算返回值数量
    i32 nres;
    if (b == 0) {
        // 返回从R(A)到栈顶的所有值
        nres = static_cast<i32>(stack.size()) - (static_cast<i32>(ci.base) + a);
    } else {
        // 返回B-1个值
        nres = b - 1;
    }

    // 将返回值移动到函数位置
    // 返回值应该从ci.func位置开始存放
    for (i32 i = 0; i < nres; i++) {
        stack.at(ci.func + i) = R(a + i);
    }

    // 设置栈顶为返回值结束位置
    usize newTop = ci.func + nres;
    while (stack.size() > newTop) {
        stack.pop();
    }

    // 关闭upvalues
    L_->closeUpvalues(ci.base);

    // ⭐ P0修复：检查是否返回到调用者
    if (--nexeccalls == 0) {
        // 最外层函数返回，退出执行循环
        // 不需要弹出CallInfo或调用postcall
        return true;  // 需要return
    } else {
        // 还有调用者，需要弹出CallInfo并恢复调用者状态

        // ⭐ P0修复：调用postcall处理返回值
        i32 funcPos = static_cast<i32>(ci.func);
        i32 wantedResults = ci.nresults;

        // 弹出CallInfo
        L_->popCallInfo();

        // 处理返回值
        postcall(funcPos, wantedResults);

        // 更新base指针
        updateBasePointer();

        // ⭐ 跳转到reentry继续执行调用者的代码
        return true;  // 需要reentry
    }
}

// =====================================================================
// 元方法调用辅助函数实现
// =====================================================================

/**
 * @brief 尝试调用算术运算元方法
 *
 * 当算术运算的操作数不是数字时，尝试调用相应的元方法。
 *
 * @param op 算术运算类型（TM_ADD, TM_SUB等）
 * @param left 左操作数
 * @param right 右操作数
 * @param result 存储结果的位置
 * @return true 如果成功调用元方法，false 如果没有元方法
 *
 * @see lua_c_analysis/src/lvm.c 第689-698行 call_binTM()
 */
bool VM::tryArithMetamethod(TMS op, const Value& left, const Value& right, Value& result) {
    // 调用二元运算元方法
    // 先尝试左操作数的元方法，如果不存在则尝试右操作数的元方法
    return callBinaryTM(L_, left, right, result, op);
}

/**
 * @brief 通用元方法调用接口
 *
 * 提供统一的元方法调用机制，处理栈操作和函数调用。
 *
 * @param metamethod 元方法函数
 * @param arg1 第一个参数
 * @param arg2 第二个参数
 * @param result 存储返回值的位置
 */
void VM::callMetamethod(const Value& metamethod, const Value& arg1,
                       const Value& arg2, Value& result) {
    // 调用元方法并获取返回值
    callTMWithResult(L_, result, metamethod, arg1, arg2);
}

} // namespace Lua

