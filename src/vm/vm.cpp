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
    if (!proto) {
        throw std::runtime_error("VM::executeProto: null proto");
    }

    // 检查嵌套调用深度（防止栈溢出）
    static constexpr i32 MAX_CALLS = 200;
    if (nexeccalls >= MAX_CALLS) {
        throw std::runtime_error("VM: stack overflow (too many nested calls)");
    }

    currentProto_ = proto;
    pc_ = 0;

    // 获取当前CallInfo
    CallInfo& ci = L_->getCurrentCallInfo();

    // 确保栈有足够的空间
    Stack& stack = L_->getStack();
    usize requiredTop = ci.base + proto->getMaxStackSize();
    while (stack.size() < requiredTop) {
        stack.push(Value());  // 用nil填充
    }

    // 缓存base指针以提高性能（避免每次R()调用都查找CallInfo）
    // 注意：必须在栈扩展之后获取，因为vector可能重新分配内存
    base_ = &stack.at(ci.base);

    // 主执行循环
    const Vec<Instruction>& code = proto->getCode();

    while (pc_ < code.size()) {
        Instruction inst = code[pc_];

        // 解码指令
        OpCode op = GET_OPCODE(inst);
        i32 a = GETARG_A(inst);
        i32 b = GETARG_B(inst);
        i32 c = GETARG_C(inst);
        i32 bx = GETARG_Bx(inst);
        i32 sbx = GETARG_sBx(inst);

        #ifdef DEBUG
        // 只为Test 13打印执行流程（检测TFORLOOP指令）
        static bool inTest13 = false;
        if (op == OpCode::TFORLOOP) inTest13 = true;
        if (inTest13 && (op == OpCode::ADD || op == OpCode::TFORLOOP || op == OpCode::JMP)) {
            std::cout << "  [EXEC] pc=" << pc_ << " op=" << static_cast<i32>(op)
                      << " (" << (op == OpCode::ADD ? "ADD" : op == OpCode::JMP ? "JMP" : "TFORLOOP") << ")"
                      << " A=" << a << " B=" << b << " C=" << c << " sBx=" << sbx << std::endl;
            if (op == OpCode::ADD) {
                std::cout << "    Before: R(0)=" << R(0).toString() << " R(5)=" << R(5).toString() << std::endl;
            }
        }
        #endif

        pc_++;  // 先递增PC
        
        // 指令分发
        switch (op) {
            case OpCode::MOVE: {
                // R(A) := R(B)
                R(a) = R(b);
                break;
            }
            
            case OpCode::LOADK: {
                // R(A) := K(Bx)
                R(a) = K(bx);
                break;
            }
            
            case OpCode::LOADBOOL: {
                // R(A) := (bool)B; if (C) pc++
                R(a) = Value(b != 0);
                if (c != 0) {
                    pc_++;
                }
                break;
            }

            case OpCode::LOADNIL: {
                // R(A) := ... := R(B) := nil
                for (i32 i = a; i <= b; i++) {
                    R(i) = Value();  // 默认构造为nil
                }
                break;
            }
            
            case OpCode::GETGLOBAL: {
                // R(A) := Gbl[K(Bx)]
                // 使用当前函数的环境表（Lua 5.1兼容）
                const Value& key = K(bx);

                // 获取当前函数的环境表，如果未设置则使用全局表
                Table* env = currentFunc_->getEnv();
                if (!env) {
                    env = L_->getGlobalTable();
                }

                R(a) = env->get(key);
                break;
            }

            case OpCode::SETGLOBAL: {
                // Gbl[K(Bx)] := R(A)
                // 使用当前函数的环境表（Lua 5.1兼容）
                const Value& key = K(bx);

                // 获取当前函数的环境表，如果未设置则使用全局表
                Table* env = currentFunc_->getEnv();
                if (!env) {
                    env = L_->getGlobalTable();
                }

                env->set(key, R(a));
                break;
            }
            
            case OpCode::GETTABLE: {
                // R(A) := R(B)[RK(C)]
                Value& table = R(b);
                Value key = RK(c);
                if (!table.isTable()) {
                    throw std::runtime_error("VM: attempt to index a non-table value");
                }
                R(a) = table.asTable()->get(key);
                break;
            }
            
            case OpCode::SETTABLE: {
                // R(A)[RK(B)] := RK(C)
                Value& table = R(a);
                Value key = RK(b);
                Value value = RK(c);
                if (!table.isTable()) {
                    throw std::runtime_error("VM: attempt to index a non-table value");
                }
                table.asTable()->set(key, value);
                break;
            }
            
            case OpCode::NEWTABLE: {
                // R(A) := {} (size = B,C)
                Table* table = new Table();
                R(a) = Value(table);
                break;
            }

            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::POW: {
                // R(A) := RK(B) op RK(C)
                arith(op, a, b, c);
                break;
            }

            case OpCode::UNM: {
                // R(A) := -R(B)
                Value& val = R(b);
                if (!val.isNumber()) {
                    throw std::runtime_error("VM: attempt to perform arithmetic on a non-number value");
                }
                R(a) = Value(-val.asNumber());
                break;
            }

            case OpCode::NOT: {
                // R(A) := not R(B)
                R(a) = Value(!R(b).isTrue());
                break;
            }

            case OpCode::LEN: {
                // R(A) := length of R(B)
                Value& val = R(b);
                if (val.isString()) {
                    R(a) = Value(static_cast<f64>(val.asString()->getLength()));
                } else if (val.isTable()) {
                    R(a) = Value(static_cast<f64>(val.asTable()->length()));
                } else {
                    throw std::runtime_error("VM: attempt to get length of a non-string/table value");
                }
                break;
            }

            case OpCode::CONCAT: {
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
                break;
            }

            case OpCode::JMP: {
                // pc += sBx
                doJump(sbx);
                break;
            }

            case OpCode::EQ:
            case OpCode::LT:
            case OpCode::LE: {
                // if ((RK(B) op RK(C)) ~= A) then pc++
                compare(op, a, b, c);
                break;
            }

            case OpCode::TEST: {
                // if not (R(A) <=> C) then pc++
                bool val = R(a).isTrue();
                if (val != (c != 0)) {
                    pc_++;
                }
                break;
            }

            case OpCode::TESTSET: {
                // if (R(B) <=> C) then R(A) := R(B) else pc++
                bool val = R(b).isTrue();
                if (val == (c != 0)) {
                    R(a) = R(b);
                } else {
                    pc_++;
                }
                break;
            }

            // =====================================================================
            // Upvalue操作指令
            // =====================================================================

            case OpCode::GETUPVAL: {
                // R(A) := UpValue[B]
                if (!currentFunc_) {
                    throw std::runtime_error("VM: GETUPVAL without current function");
                }

                Upvalue* uv = currentFunc_->getUpvalue(b);
                if (!uv) {
                    throw std::runtime_error("VM: GETUPVAL invalid upvalue index");
                }

                R(a) = uv->getValue();
                break;
            }

            case OpCode::SETUPVAL: {
                // UpValue[B] := R(A)
                if (!currentFunc_) {
                    throw std::runtime_error("VM: SETUPVAL without current function");
                }

                Upvalue* uv = currentFunc_->getUpvalue(b);
                if (!uv) {
                    throw std::runtime_error("VM: SETUPVAL invalid upvalue index");
                }

                uv->getValue() = R(a);
                break;
            }

            case OpCode::CLOSE: {
                // close all variables in the stack up to (>=) R(A)
                L_->closeUpvalues(a);
                break;
            }

            // =====================================================================
            // 函数调用指令
            // =====================================================================

            case OpCode::CALL: {
                // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
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
                    // Lua函数：递归执行
                    Proto* calleeProto = func->getProto();

                    // 保存当前状态
                    Proto* savedProto = currentProto_;
                    usize savedPC = pc_;
                    Function* savedFunc = currentFunc_;
                    CallInfo& callerCI = currentCI;  // 保存调用者的CallInfo
                    usize savedTop = callerCI.top;   // 保存调用者的栈顶

                    // 设置新函数上下文
                    currentFunc_ = func;

                    // 递归执行被调用函数
                    // RETURN指令会将返回值移动到funcPos位置并缩小栈
                    executeProto(calleeProto, nexeccalls + 1);

                    // 恢复当前状态
                    currentProto_ = savedProto;
                    pc_ = savedPC;
                    currentFunc_ = savedFunc;

                    // 弹出CallInfo（必须在postcall之前，这样postcall才能访问调用者的CallInfo）
                    L_->popCallInfo();

                    // 处理返回值（使用绝对栈位置）
                    postcall(funcPos, nResults);

                    // 恢复调用者的栈大小
                    // 确保栈至少有savedTop个元素
                    Stack& stack = L_->getStack();
                    while (stack.size() < savedTop) {
                        stack.push(Value());
                    }

                    // 重要：恢复base_指针（因为栈可能被重新分配）
                    CallInfo& restoredCI = L_->getCurrentCallInfo();
                    base_ = &stack.at(restoredCI.base);
                } else {
                    // C函数已在precall中执行完成
                    // postcall已在precall中调用
                    // 同样需要恢复base_指针
                    Stack& stack = L_->getStack();
                    CallInfo& restoredCI = L_->getCurrentCallInfo();
                    base_ = &stack.at(restoredCI.base);
                }
                break;
            }

            case OpCode::TAILCALL: {
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
                    return;
                } else {
                    // C函数已执行，直接返回
                    return;
                }
            }

            case OpCode::SELF: {
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
                break;
            }

            // =====================================================================
            // 循环指令
            // =====================================================================

            case OpCode::FORLOOP: {
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
                // 正步长：idx <= limit
                // 负步长：idx >= limit
                bool cont = (step > 0) ? (idx <= limit) : (idx >= limit);

                if (cont) {
                    // 继续循环：先跳转，再更新寄存器
                    doJump(sbx);              // pc += sBx
                    R(a) = Value(idx);        // 更新内部索引 R(A)
                    R(a + 3) = Value(idx);    // 更新用户可见的循环变量 R(A+3)
                }
                // 如果不继续循环，则正常执行下一条指令
                break;
            }

            case OpCode::FORPREP: {
                // R(A)-=R(A+2); pc+=sBx
                if (!R(a).isNumber() || !R(a + 1).isNumber() || !R(a + 2).isNumber()) {
                    throw std::runtime_error("VM: FORPREP requires numeric values");
                }

                f64 init = R(a).asNumber();
                f64 step = R(a + 2).asNumber();
                R(a) = Value(init - step);  // 预减步长
                doJump(sbx);                // 跳转到FORLOOP
                break;
            }

            case OpCode::TFORLOOP: {
                // R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2))
                // if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++
                // 参考：lua_c_analysis/src/lvm.c 第1935-1952行

                // 泛型for循环（迭代器循环）执行步骤：
                // 1. 将迭代器函数和参数复制到调用位置 R(A+3)
                // 2. 调用迭代器：R(A+3), ..., R(A+2+C) := R(A)(R(A+1), R(A+2))
                // 3. 检查第一个返回值 R(A+3)：
                //    - 如果不是 nil：更新控制变量 R(A+2)=R(A+3)，读取下一条JMP指令并跳转
                //    - 如果是 nil：退出循环，pc++ 跳过下一条JMP指令

                i32 cb = a + 3;  // 调用基址（call base）

                // 确保栈有足够空间存储调用参数和返回值
                Stack& stack = L_->getStack();
                CallInfo& ci = L_->getCurrentCallInfo();
                usize requiredSize = ci.base + cb + 3 + c;  // 需要 cb+3+c 个寄存器
                while (stack.size() < requiredSize) {
                    stack.push(Value());
                }
                // 更新 base_（栈可能已扩展）
                base_ = &stack.at(ci.base);

                // 步骤1：将迭代器函数和参数复制到调用位置
                // setobjs2s(L, cb+2, ra+2);  // R(A+5) = R(A+2) (控制变量)
                // setobjs2s(L, cb+1, ra+1);  // R(A+4) = R(A+1) (状态)
                // setobjs2s(L, cb, ra);      // R(A+3) = R(A)   (迭代器函数)
                R(cb + 2) = R(a + 2);  // 控制变量
                R(cb + 1) = R(a + 1);  // 状态
                R(cb) = R(a);          // 迭代器函数

                // 步骤2：调用迭代器函数
                // L->top = cb + 3;  设置栈顶为函数+2个参数
                // Protect(luaD_call(L, cb, GETARG_C(i)));

                // 检查迭代器函数类型
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

                    // 恢复base_
                    base_ = &stack.at(ci.base);

                    // 将返回值存储到 R(cb), R(cb+1), ..., R(cb+c-1)
                    for (usize i = 0; i < returnValues.size(); i++) {
                        R(cb + i) = returnValues[i];
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
                // cb = RA(i) + 3;  (调用可能改变栈，重新计算)
                cb = a + 3;

                if (!R(cb).isNil()) {
                    // 继续循环：更新控制变量并跳转
                    // setobjs2s(L, cb-1, cb);  即 R(A+2) = R(A+3)
                    R(a + 2) = R(cb);

                    // 读取下一条指令（JMP）的sBx并跳转
                    // 参考：lua_c_analysis/src/lvm.c 第1948行
                    // dojump(L, pc, GETARG_sBx(*pc)); pc++;
                    //
                    // 在 Lua C 中：
                    // - pc 在指令开始时已经递增，所以 *pc 是下一条指令（JMP）
                    // - dojump(L, pc, offset): pc += offset
                    // - pc++: 跳过 JMP 指令
                    //
                    // 在我们的实现中：
                    // - 主循环在指令分发前执行 pc++，所以 pc_ 现在指向下一条指令（JMP）
                    // - doJump(offset): pc_ += offset
                    // - 不执行 pc++，因为主循环会在下一次迭代时执行
                    if (pc_ < code.size()) {
                        Instruction jmpInst = code[pc_];
                        i32 jmpSbx = GETARG_sBx(jmpInst);
                        doJump(jmpSbx);
                    }
                } else {
                    // 退出循环：跳过下一条JMP指令
                    pc_++;
                }

                break;
            }

            // =====================================================================
            // 闭包和表初始化指令
            // =====================================================================

            case OpCode::CLOSURE: {
                // R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
                // 创建闭包：从常量表中的Proto创建新的Function对象

                // 简化实现：从常量表获取Proto（假设Bx是常量索引）
                // 注意：标准Lua中CLOSURE使用单独的Proto数组，这里简化处理
                Value protoVal = K(bx);

                if (!protoVal.isFunction()) {
                    throw std::runtime_error("VM: CLOSURE requires function proto in constants");
                }

                // 创建新的闭包（复制函数）
                Function* proto = protoVal.asFunction();
                Function* closure = new Function(proto->getProto());

                // TODO: 处理upvalues（需要读取后续的MOVE/GETUPVAL指令）
                // 简化版：暂不处理upvalues

                R(a) = Value(closure);
                break;
            }

            case OpCode::SETLIST: {
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
                break;
            }

            case OpCode::VARARG: {
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
                break;
            }

            case OpCode::RETURN: {
                // return R(A), ... ,R(A+B-2)
                // B=0: 返回从R(A)到栈顶的所有值
                // B=1: 无返回值
                // B>1: 返回B-1个值（R(A)到R(A+B-2)）

                CallInfo& ci = L_->getCurrentCallInfo();
                Stack& stack = L_->getStack();

                // 计算返回值数量
                i32 nres;
                if (b == 0) {
                    // 返回从R(A)到栈顶的所有值
                    nres = static_cast<i32>(stack.size()) - (ci.base + a);
                } else {
                    // 返回B-1个值
                    nres = b - 1;
                }

                // 将返回值移动到函数位置
                // 返回值应该从ci.func位置开始存放
                for (i32 i = 0; i < nres; i++) {
                    stack.at(ci.func + i) = R(a + i);
                }

                // 不要在这里缩小栈！
                // 栈的调整应该由调用者（postcall）来处理
                // 因为调用者可能还需要访问其他寄存器

                // 但是，我们需要标记栈顶的位置，以便postcall知道有多少返回值
                // 我们通过调整栈大小到ci.func + nres来实现这一点
                // 但要确保不会缩小到调用者的栈帧以下

                // 实际上，让我们保持栈不变，让postcall来处理
                // 但我们需要某种方式告诉postcall有多少返回值
                // 标准做法是：将栈顶设置为ci.func + nres

                // 为了安全起见，我们只在必要时缩小栈
                // 即：只移除当前函数的局部变量，但不影响调用者的栈帧
                usize newTop = ci.func + nres;
                while (stack.size() > newTop) {
                    stack.pop();
                }

                // 关闭upvalues
                L_->closeUpvalues(ci.base);

                // 函数返回
                return;
            }

            default: {
                throw std::runtime_error("VM: unsupported opcode: " +
                    Str(getOpName(op)));
            }
        }
    }
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
        return currentProto_->getConstant(INDEXK(rk));
    } else {
        // 寄存器 - 返回副本
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

void VM::arith(OpCode op, i32 a, i32 b, i32 c) {
    Value left = RK(b);
    Value right = RK(c);

    if (!left.isNumber() || !right.isNumber()) {
        throw std::runtime_error("VM: attempt to perform arithmetic on non-number values");
    }

    f64 lval = left.asNumber();
    f64 rval = right.asNumber();
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
}

void VM::compare(OpCode op, i32 a, i32 b, i32 c) {
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

    // 如果结果与A不同，则跳过下一条指令
    if (result != (a != 0)) {
        pc_++;
    }
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
        postcall(funcPos, nResults);

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
            actualArgs = static_cast<i32>(stack.size()) - (funcPos + 1);
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

    // 栈顶现在应该在 funcPos + actualResults
    // 已经通过上面的push/pop调整好了
}

} // namespace Lua

