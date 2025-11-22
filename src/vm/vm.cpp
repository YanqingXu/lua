/**
 * @file vm.cpp
 * @brief Lua虚拟机执行引擎实现
 */

#include "vm/vm.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "core/upvalue.hpp"
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

    // 设置当前函数（用于访问upvalues）
    currentFunc_ = func;

    // 获取Lua函数的Proto
    Proto* proto = func->getProto();
    executeProto(proto, 1);
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
            case OpCode::VARARG:    executeVararg(a, b); break;
            
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
    base_ = &stack.at(ci.base);
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
    #ifdef DEBUG
    CallInfo& ci = L_->getCurrentCallInfo();
    Stack& stack = L_->getStack();
    usize absIndex = ci.base + index;
    if (absIndex >= stack.size()) {
        throw std::runtime_error("VM::R: register index out of range");
    }
    #endif

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
    // ⭐ P0修复：参考lua_c_analysis/src/lvm.c:1753-1780
    // 比较指令的语义：
    // - 执行比较操作 RK(B) op RK(C)
    // - 如果结果 == A，则读取下一条指令（JMP）的sBx并跳转
    // - 无论如何都要 pc++ 跳过下一条JMP指令

    Value left = RK(b);
    Value right = RK(c);

    bool result = false;

    // 类型必须相同才能比较
    if (left.getType() != right.getType()) {
        result = false;
    } else if (left.isNumber() && right.isNumber()) {
        f64 lval = left.asNumber();
        f64 rval = right.asNumber();

        switch (op) {
            case OpCode::EQ:
                result = (lval == rval);
                break;
            case OpCode::LT:
                result = (lval < rval);
                break;
            case OpCode::LE:
                result = (lval <= rval);
                break;
            default:
                throw std::runtime_error("VM::compare: invalid opcode");
        }
    } else if (left.isString() && right.isString()) {
        const Str& lstr = left.asString()->getData();
        const Str& rstr = right.asString()->getData();

        switch (op) {
            case OpCode::EQ:
                result = (lstr == rstr);
                break;
            case OpCode::LT:
                result = (lstr < rstr);
                break;
            case OpCode::LE:
                result = (lstr <= rstr);
                break;
            default:
                throw std::runtime_error("VM::compare: invalid opcode");
        }
    } else if (left.isBoolean() && right.isBoolean()) {
        bool lval = left.asBoolean();
        bool rval = right.asBoolean();

        if (op == OpCode::EQ) {
            result = (lval == rval);
        } else {
            throw std::runtime_error("VM::compare: cannot compare booleans with < or <=");
        }
    } else if (left.isNil() && right.isNil()) {
        result = (op == OpCode::EQ);
    }

    // ⭐ P0修复：Lua 5.1.5的比较指令逻辑
    // if (condition == GETARG_A(i))
    //     dojump(L, pc, GETARG_sBx(*pc));
    // pc++;

    const Vec<Instruction>& code = currentProto_->getCode();

    #ifdef DEBUG
    std::cerr << "[COMPARE] op=" << static_cast<int>(op)
              << " left=" << left.toString()
              << " right=" << right.toString()
              << " result=" << result
              << " A=" << a
              << " shouldJump=" << (result == (a != 0)) << std::endl;
    #endif

    // 如果比较结果 == A，则执行跳转
    if (result == (a != 0)) {
        // 读取下一条指令（应该是JMP）的sBx
        if (pc_ < code.size()) {
            Instruction nextInst = code[pc_];
            i32 sbx = GETARG_sBx(nextInst);
            #ifdef DEBUG
            std::cerr << "[COMPARE] Jumping by " << sbx << " (pc " << pc_ << " -> " << (pc_ + sbx + 1) << ")" << std::endl;
            #endif
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

    if (!funcVal.isFunction()) {
        throw std::runtime_error("VM::precall: attempt to call non-function value");
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

        // 调用C函数
        i32 result = cfunc(L_);

        // 处理返回值
        postcall(static_cast<i32>(funcPos), nResults);

        // 弹出CallInfo
        L_->popCallInfo();

        return false;  // C函数
    } else {
        // Lua函数调用
        Proto* proto = func->getProto();

        // 处理参数数量
        i32 actualArgs = nArgs;
        if (nArgs < 0) {
            // nArgs < 0 表示参数到栈顶
            actualArgs = static_cast<i32>(stack.size()) - static_cast<i32>((funcPos + 1));
        }

        // 创建新的CallInfo
        CallInfo& ci = L_->pushCallInfo();
        ci.func = funcPos;
        ci.base = funcPos + 1;  // 第一个参数/局部变量的位置
        ci.top = ci.base + proto->getMaxStackSize();
        ci.nresults = nResults;
        ci.savedpc = nullptr;  // 将在executeProto中设置
        ci.tailcalls = 0;

        // 调整参数数量（如果实际参数少于形参，用nil填充）
        i32 numParams = proto->getNumParams();
        while (actualArgs < numParams) {
            stack.push(Value());  // nil
            actualArgs++;
        }

        // 确保栈空间足够
        while (stack.size() < ci.top) {
            stack.push(Value());  // 用nil初始化局部变量
        }

        return true;  // Lua函数
    }
}

void VM::postcall(i32 funcPos, i32 wantedResults) {
    // ⭐ P0修复：参考lua_c_analysis/src/ldo.c:1198
    // 处理函数返回后的返回值复制和栈调整
    Stack& stack = L_->getStack();
    CallInfo& ci = L_->getCurrentCallInfo();

    // 返回值从funcPos开始（函数位置被返回值覆盖）
    // 计算实际返回值数量
    i32 actualResults = static_cast<i32>(stack.size()) - funcPos;

    if (wantedResults >= 0) {
        // 调用者期望固定数量的返回值
        if (actualResults < wantedResults) {
            // 返回值不够，用nil填充
            while (actualResults < wantedResults) {
                stack.push(Value());
                actualResults++;
            }
        } else if (actualResults > wantedResults) {
            // 返回值太多，丢弃多余的
            while (actualResults > wantedResults) {
                stack.pop();
                actualResults--;
            }
        }
    } else {
        // wantedResults < 0 (MULTRET): 接受所有返回值
        // 不需要调整
    }

    // ⭐ P0修复：恢复调用者的执行状态
    // 注意：此时ci已经指向调用者的CallInfo（因为在RETURN中已经popCallInfo）
    // 恢复currentFunc_和currentProto_
    if (ci.func < stack.size()) {
        Value& funcVal = stack.at(ci.func);
        if (funcVal.isFunction()) {
            currentFunc_ = funcVal.asFunction();
            currentProto_ = currentFunc_->getProto();
        }
    }

    // savedpc会在reentry时从ci.savedpc恢复，这里不需要额外处理

    // 栈顶现在应该在 funcPos + actualResults
    // 已经通过上面的push/pop调整好了
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
    if (ci.func < stack.size()) {
        Value& funcVal = stack.at(ci.func);
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
    Value& table = R(b);
    Value key = RK(c);
    if (!table.isTable()) {
        throw std::runtime_error("VM: attempt to index a non-table value");
    }
    R(a) = table.asTable()->get(key);
}

void VM::executeSetTable(i32 a, i32 b, i32 c) {
    // R(A)[RK(B)] := RK(C)
    Value& table = R(a);
    Value key = RK(b);
    Value value = RK(c);
    if (!table.isTable()) {
        throw std::runtime_error("VM: attempt to index a non-table value");
    }
    table.asTable()->set(key, value);
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
    Value& val = R(b);
    if (val.isString()) {
        R(a) = Value(static_cast<f64>(val.asString()->getLength()));
    } else if (val.isTable()) {
        R(a) = Value(static_cast<f64>(val.asTable()->length()));
    } else {
        throw std::runtime_error("VM: attempt to get length of a non-string/table value");
    }
}

void VM::executeConcat(i32 a, i32 b, i32 c) {
    // R(A) := R(B).. ... ..R(C)
    // 简化实现：只支持两个字符串连接
    Value& left = R(b);
    Value& right = R(c);

    if (!left.isString() || !right.isString()) {
        throw std::runtime_error("VM: attempt to concatenate non-string values");
    }

    Str result = left.asString()->getData() + right.asString()->getData();
    StringPool& pool = GlobalState::getInstance().getStringPool();
    GCString* str = pool.intern(result);
    R(a) = Value(str);
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
    // R(A), R(A+1), ..., R(A+B-1) = vararg
    // 简化实现：暂不支持可变参数
    if (b == 0) {
        // 复制所有可变参数（暂不支持）
        throw std::runtime_error("VM: VARARG with B=0 not supported yet");
    } else {
        // 复制B-1个可变参数
        // 简化：设置为nil
        for (i32 i = 0; i < b - 1; i++) {
            R(a + i) = Value();
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

