/**
 * @file vm.cpp
 * @brief Lua虚拟机执行引擎实现
 */

#include "vm/vm.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
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

} // namespace Lua

