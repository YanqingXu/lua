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
    executeProto(proto);
}

void VM::executeProto(Proto* proto) {
    if (!proto) {
        throw std::runtime_error("VM::executeProto: null proto");
    }

    currentProto_ = proto;
    pc_ = 0;

    // 获取栈基址
    Stack& stack = L_->getStack();

    // 确保栈有足够的空间（至少maxStackSize个槽位）
    usize requiredSize = proto->getMaxStackSize();
    while (stack.size() < requiredSize) {
        stack.push(Value());  // 用nil填充
    }

    base_ = &stack.at(0);  // 简化版：假设从栈底开始

    // 主执行循环
    const Vec<Instruction>& code = proto->getCode();
    
    while (pc_ < code.size()) {
        Instruction inst = code[pc_];
        pc_++;  // 先递增PC
        
        // 解码指令
        OpCode op = GET_OPCODE(inst);
        i32 a = GETARG_A(inst);
        i32 b = GETARG_B(inst);
        i32 c = GETARG_C(inst);
        i32 bx = GETARG_Bx(inst);
        i32 sbx = GETARG_sBx(inst);
        
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
                const Value& key = K(bx);
                Table* globalTable = L_->getGlobalTable();
                R(a) = globalTable->get(key);
                break;
            }
            
            case OpCode::SETGLOBAL: {
                // Gbl[K(Bx)] := R(A)
                const Value& key = K(bx);
                Table* globalTable = L_->getGlobalTable();
                globalTable->set(key, R(a));
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

                // 准备调用
                bool isLua = precall(a, nArgs, nResults);

                if (isLua) {
                    // Lua函数：需要递归执行（简化版：暂不支持）
                    throw std::runtime_error("VM: CALL nested Lua functions not supported yet");
                }
                // C函数已在precall中执行完成
                break;
            }

            case OpCode::TAILCALL: {
                // return R(A)(R(A+1), ... ,R(A+B-1))
                i32 nArgs = b - 1;

                // 尾调用：关闭当前函数的upvalues
                L_->closeUpvalues(0);

                // 准备调用（返回值数量为-1表示多返回值）
                bool isLua = precall(a, nArgs, -1);

                if (isLua) {
                    // Lua函数：需要特殊处理（简化版：暂不支持）
                    throw std::runtime_error("VM: TAILCALL Lua functions not supported yet");
                }

                // C函数已执行，直接返回
                return;
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
                if (!R(a).isNumber() || !R(a + 1).isNumber() || !R(a + 2).isNumber()) {
                    throw std::runtime_error("VM: FORLOOP requires numeric values");
                }

                f64 step = R(a + 2).asNumber();
                f64 idx = R(a).asNumber() + step;
                f64 limit = R(a + 1).asNumber();

                // 检查循环条件
                bool cont = (step > 0) ? (idx <= limit) : (idx >= limit);

                if (cont) {
                    R(a) = Value(idx);        // 更新索引
                    R(a + 3) = Value(idx);    // 设置循环变量
                    doJump(sbx);              // 跳转到循环体
                }
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
                // 泛型for循环：调用迭代器函数
                i32 nResults = c;

                // 准备调用：R(A)是迭代器函数，R(A+1)和R(A+2)是参数
                bool isLua = precall(a, 2, nResults);

                if (isLua) {
                    throw std::runtime_error("VM: TFORLOOP Lua iterators not supported yet");
                }

                // 检查第一个返回值
                if (!R(a + 3).isNil()) {
                    R(a + 2) = R(a + 3);  // 更新控制变量
                } else {
                    pc_++;  // 跳过下一条指令（跳出循环）
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
                // 简化实现：只处理单个返回值或无返回值
                if (b == 0) {
                    // 返回所有值（暂不支持）
                    return;
                } else if (b == 1) {
                    // 无返回值
                    return;
                } else {
                    // 返回 R(A) 到 R(A+B-2)
                    // 简化：只返回 R(A)
                    Stack& stack = L_->getStack();
                    stack.push(R(a));
                    return;
                }
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
    // 简化实现：直接从栈访问
    Stack& stack = L_->getStack();
    return stack.at(index);
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
    Value& funcVal = R(funcIndex);

    if (!funcVal.isFunction()) {
        throw std::runtime_error("VM::precall: attempt to call non-function value");
    }

    Function* func = funcVal.asFunction();

    if (func->isCFunction()) {
        // C函数调用
        CFunction cfunc = func->getCFunction();

        // 简化实现：直接调用C函数
        // 注意：标准Lua会设置CallInfo，这里简化处理
        i32 result = cfunc(L_);

        // 处理返回值（简化：假设返回值已在栈上）
        postcall(funcIndex, nResults);

        return false;  // C函数
    } else {
        // Lua函数调用
        // 简化实现：暂不支持嵌套Lua函数调用
        // 标准实现需要：
        // 1. 创建新的CallInfo
        // 2. 调整栈帧
        // 3. 设置参数
        // 4. 递归调用executeProto

        throw std::runtime_error("VM::precall: nested Lua function calls not supported yet");
        return true;  // Lua函数
    }
}

void VM::postcall(i32 firstResult, i32 nResults) {
    // 简化实现：处理返回值
    // 标准实现需要：
    // 1. 从栈上复制返回值到正确位置
    // 2. 调整栈顶
    // 3. 恢复CallInfo

    Stack& stack = L_->getStack();

    if (nResults >= 0) {
        // 固定数量的返回值
        // 简化：假设返回值已在正确位置
    } else {
        // 多返回值（LUA_MULTRET）
        // 简化：保留栈上所有值
    }
}

} // namespace Lua

