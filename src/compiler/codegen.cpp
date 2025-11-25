/**
 * @file codegen.cpp
 * @brief Lua字节码生成器实现
 */

#include "compiler/codegen.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/value.hpp"
#include <stdexcept>
#include <cassert>
#include <iostream>

namespace Lua {

// =====================================================================
// 构造和析构
// =====================================================================

CodeGenerator::CodeGenerator(StringPool* pool)
    : pool_(pool)
    , proto_(nullptr)
    , freereg_(0)
    , nactvar_(0)
    , localVars_()
    , pc_(0)
    , jpc_(NO_JUMP)
{
    if (pool == nullptr) {
        throw std::invalid_argument("StringPool cannot be null");
    }
}

CodeGenerator::~CodeGenerator() {
    // Proto由调用者管理
}

// =====================================================================
// 主生成函数
// =====================================================================

Proto* CodeGenerator::generate(const Chunk& chunk) {
    // 创建新的Proto对象
    proto_ = new Proto();
    proto_->setMaxStackSize(2);  // 最小栈大小
    
    // 重置状态
    freereg_ = 0;
    nactvar_ = 0;
    localVars_.clear();
    pc_ = 0;
    
    // 生成语句块
    block(chunk.statements);
    
    // 添加RETURN指令（如果最后一条指令不是RETURN）
    if (proto_->getInstructionCount() == 0 || 
        GET_OPCODE(proto_->getInstruction(proto_->getInstructionCount() - 1)) != OpCode::RETURN) {
        codeABC(OpCode::RETURN, 0, 1, 0);  // return (no values)
    }
    
    return proto_;
}

// =====================================================================
// 指令生成
// =====================================================================

i32 CodeGenerator::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    Instruction inst = CREATE_ABC(op, a, b, c);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(0);  // TODO: 添加实际行号
    return pc;
}

i32 CodeGenerator::codeABx(OpCode op, i32 a, i32 bx) {
    Instruction inst = CREATE_ABx(op, a, bx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(0);  // TODO: 添加实际行号
    return pc;
}

i32 CodeGenerator::codeAsBx(OpCode op, i32 a, i32 sbx) {
    Instruction inst = CREATE_AsBx(op, a, sbx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(0);  // TODO: 添加实际行号
    return pc;
}

// =====================================================================
// 寄存器管理
// =====================================================================

i32 CodeGenerator::allocReg() {
    i32 reg = freereg_++;
    if (freereg_ > static_cast<i32>(proto_->getMaxStackSize())) {
        proto_->setMaxStackSize(static_cast<u8>(freereg_));
    }
    return reg;
}

void CodeGenerator::freeReg(i32 reg) {
    if (reg >= nactvar_ && reg == freereg_ - 1) {
        freereg_--;
    }
}

void CodeGenerator::freeRegs(i32 n) {
    for (i32 i = 0; i < n; i++) {
        freeReg(freereg_ - 1);
    }
}

// =====================================================================
// 常量表管理
// =====================================================================

i32 CodeGenerator::numberConstant(f64 value) {
    Value v(value);
    return static_cast<i32>(proto_->addConstant(v));
}

i32 CodeGenerator::stringConstant(const Str& value) {
    GCString* str = pool_->intern(value);
    Value v(str);
    return static_cast<i32>(proto_->addConstant(v));
}

i32 CodeGenerator::boolConstant(bool value) {
    Value v(value);
    return static_cast<i32>(proto_->addConstant(v));
}

i32 CodeGenerator::nilConstant() {
    Value v;  // nil
    return static_cast<i32>(proto_->addConstant(v));
}

// =====================================================================
// 局部变量管理
// =====================================================================

i32 CodeGenerator::addLocalVar(const Str& name) {
    i32 reg = freereg_;
    localVars_.emplace_back(name, reg, static_cast<i32>(proto_->getInstructionCount()));
    return reg;
}

i32 CodeGenerator::findLocalVar(const Str& name) {
    // 从后向前查找（内层作用域优先）
    for (i32 i = static_cast<i32>(localVars_.size()) - 1; i >= 0; i--) {
        if (localVars_[i].name == name && localVars_[i].endpc == -1) {
            return localVars_[i].reg;
        }
    }
    return -1;  // 未找到
}

void CodeGenerator::adjustLocalVars(i32 nvars) {
    nactvar_ += nvars;
    freereg_ = nactvar_;
}

void CodeGenerator::removeLocalVars(i32 tolevel) {
    i32 pc = static_cast<i32>(proto_->getInstructionCount());
    while (nactvar_ > tolevel) {
        nactvar_--;
        if (!localVars_.empty() && localVars_.back().endpc == -1) {
            localVars_.back().endpc = pc;
        }
    }
    freereg_ = nactvar_;
}

// =====================================================================
// 跳转管理
// =====================================================================

i32 CodeGenerator::jump() {
    i32 jpc = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    #ifdef DEBUG
    std::cerr << "[CodeGenerator::jump] Generated JMP at pc=" << jpc
              << " with sBx=" << NO_JUMP << std::endl;
    #endif
    return jpc;
}

void CodeGenerator::patchList(i32 list, i32 target) {
    while (list != NO_JUMP) {
        i32 next = GETARG_sBx(proto_->getInstruction(list));
        Instruction& inst = proto_->getCode()[list];
        i32 offset = target - list - 1;
        #ifdef DEBUG
        std::cerr << "[CodeGenerator::patchList] Patching pc=" << list
                  << " to target=" << target
                  << " offset=" << offset << std::endl;
        #endif
        SETARG_sBx(inst, offset);
        list = next;
    }
}

void CodeGenerator::patchToHere(i32 list) {
    patchList(list, static_cast<i32>(proto_->getInstructionCount()));
}

i32 CodeGenerator::getLabel() {
    return static_cast<i32>(proto_->getInstructionCount());
}

// =====================================================================
// 表达式代码生成（简化版）
// =====================================================================

void CodeGenerator::expr(const Expr& e, ExprDesc& desc) {
    // 访问variant获取具体的表达式类型
    std::visit([this, &desc](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, NilExpr>) {
            desc.kind = ExprKind::Nil;
        }
        else if constexpr (std::is_same_v<T, BoolExpr>) {
            desc.kind = arg.value ? ExprKind::True : ExprKind::False;
        }
        else if constexpr (std::is_same_v<T, NumberExpr>) {
            desc.kind = ExprKind::Number;
            desc.u.nval = arg.value;
        }
        else if constexpr (std::is_same_v<T, StringExpr>) {
            i32 k = stringConstant(arg.value);
            desc.kind = ExprKind::Const;
            desc.u.s.info = k;
        }
        else if constexpr (std::is_same_v<T, NameExpr>) {
            // 查找局部变量
            i32 reg = findLocalVar(arg.name);
            if (reg >= 0) {
                // 局部变量
                desc.kind = ExprKind::Local;
                desc.u.s.info = reg;
            } else {
                // 全局变量：使用GETGLOBAL/SETGLOBAL指令
                i32 k = stringConstant(arg.name);
                desc.kind = ExprKind::Global;
                desc.u.s.info = k;
            }
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            binaryExpr(arg, desc);
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            unaryExpr(arg, desc);
        }
        else if constexpr (std::is_same_v<T, FunctionExpr>) {
            functionExpr(arg, desc);
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            callExpr(arg, desc);
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) {
            // 表索引访问 table[key]
            // 1. 计算表表达式
            ExprDesc t;
            expr(*arg.table, t);
            // 2. 将表表达式转换到寄存器
            luaK_dischargevars(t);
            // 3. 计算索引表达式
            ExprDesc k;
            expr(*arg.index, k);
            // 4. 设置为索引表达式
            luaK_indexed(t, k);
            desc = t;
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            // 成员访问 table.member
            // 等价于 table["member"]
            // 1. 计算表表达式
            ExprDesc t;
            expr(*arg.table, t);
            // 2. 将表表达式转换到寄存器
            luaK_dischargevars(t);
            // 3. 创建字符串常量作为索引
            ExprDesc k;
            k.kind = ExprKind::Const;
            k.u.s.info = stringConstant(arg.member);
            k.t = NO_JUMP;
            k.f = NO_JUMP;
            // 4. 设置为索引表达式
            luaK_indexed(t, k);
            desc = t;
        }
        else {
            // 其他表达式类型暂不支持
            desc.kind = ExprKind::Void;
        }
    }, e.variant);
}

void CodeGenerator::discharge(ExprDesc& desc, i32 reg) {
    switch (desc.kind) {
        case ExprKind::Nil:
            codeABC(OpCode::LOADNIL, reg, 0, 0);
            break;
        case ExprKind::True:
        case ExprKind::False:
            codeABC(OpCode::LOADBOOL, reg, desc.kind == ExprKind::True ? 1 : 0, 0);
            break;
        case ExprKind::Number: {
            i32 k = numberConstant(desc.u.nval);
            codeABx(OpCode::LOADK, reg, k);
            break;
        }
        case ExprKind::Const:
            codeABx(OpCode::LOADK, reg, desc.u.s.info);
            break;
        case ExprKind::Local:
            if (desc.u.s.info != reg) {
                codeABC(OpCode::MOVE, reg, desc.u.s.info, 0);
            }
            break;
        case ExprKind::Global:
            // 全局变量读取：GETGLOBAL A Bx
            // A = 目标寄存器
            // Bx = 全局变量名在常量表中的索引
            codeABx(OpCode::GETGLOBAL, reg, desc.u.s.info);
            break;
        case ExprKind::Indexed: {
            // 表索引访问：GETTABLE A B C
            // A = 目标寄存器
            // B = 表的寄存器索引（存储在info中）
            // C = 键的RK操作数（存储在aux中）
            codeABC(OpCode::GETTABLE, reg, desc.u.s.info, desc.u.s.aux);
            break;
        }
        default:
            break;
    }
    desc.kind = ExprKind::NonRelocatable;
    desc.u.s.info = reg;
}

i32 CodeGenerator::exp2RK(ExprDesc& desc) {
    exp2Val(desc);
    switch (desc.kind) {
        case ExprKind::Number: {
            i32 k = numberConstant(desc.u.nval);
            if (k <= MAXINDEXRK) {
                desc.kind = ExprKind::Const;
                desc.u.s.info = k;
                return RKASK(k);
            }
            break;
        }
        case ExprKind::Const:
            if (desc.u.s.info <= MAXINDEXRK) {
                return RKASK(desc.u.s.info);
            }
            break;
        default:
            break;
    }
    return exp2AnyReg(desc);
}

i32 CodeGenerator::exp2AnyReg(ExprDesc& desc) {
    if (desc.kind != ExprKind::NonRelocatable) {
        i32 reg = allocReg();
        discharge(desc, reg);
        return reg;
    }
    return desc.u.s.info;
}

void CodeGenerator::exp2NextReg(ExprDesc& desc) {
    exp2Val(desc);
    i32 reg = allocReg();
    discharge(desc, reg);
}

void CodeGenerator::exp2Val(ExprDesc& desc) {
    // 简化实现：大多数情况下不需要特殊处理
}

// =====================================================================
// 语句代码生成（简化版）
// =====================================================================

void CodeGenerator::statement(const Stmt& s) {
    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, AssignStmt>) {
            // 赋值语句：使用 luaK_storevar 统一接口
            // 支持多重赋值：a, b, c = 1, 2, 3
            i32 nvars = static_cast<i32>(arg.targets.size());
            i32 nexps = static_cast<i32>(arg.values.size());

            // 计算所有右值表达式
            for (i32 i = 0; i < nexps && i < nvars; i++) {
                ExprDesc val;
                expr(*arg.values[i], val);

                // 计算左值（目标变量）
                ExprDesc var;
                expr(*arg.targets[i], var);

                // 使用统一接口存储值到变量
                luaK_storevar(var, val);
            }

            // 如果变量多于值，剩余变量赋值为 nil
            for (i32 i = nexps; i < nvars; i++) {
                ExprDesc var;
                expr(*arg.targets[i], var);

                ExprDesc nil;
                nil.kind = ExprKind::Nil;
                nil.t = NO_JUMP;
                nil.f = NO_JUMP;

                luaK_storevar(var, nil);
            }
        }
        else if constexpr (std::is_same_v<T, LocalStmt>) {
            // 局部变量声明
            i32 nvars = static_cast<i32>(arg.names.size());
            i32 nexps = static_cast<i32>(arg.values.size());

            // 为每个变量分配寄存器
            for (i32 i = 0; i < nvars; i++) {
                addLocalVar(arg.names[i]);
            }

            // 生成初始化代码
            for (i32 i = 0; i < nexps && i < nvars; i++) {
                ExprDesc val;
                expr(*arg.values[i], val);
                discharge(val, nactvar_ + i);
            }

            // 未初始化的变量设为nil
            if (nexps < nvars) {
                codeABC(OpCode::LOADNIL, nactvar_ + nexps, nactvar_ + nvars - 1, 0);
            }

            adjustLocalVars(nvars);
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            // 返回语句
            i32 nret = static_cast<i32>(arg.values.size());
            if (nret == 0) {
                codeABC(OpCode::RETURN, 0, 1, 0);
            } else {
                i32 base = freereg_;
                for (i32 i = 0; i < nret; i++) {
                    ExprDesc val;
                    expr(*arg.values[i], val);
                    exp2NextReg(val);
                }
                codeABC(OpCode::RETURN, base, nret + 1, 0);
                freeRegs(nret);
            }
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            // if语句（简化实现）
            if (!arg.branches.empty()) {
                Vec<i32> escapelist;

                for (const auto& branch : arg.branches) {
                    ExprDesc cond;
                    expr(*branch.condition, cond);
                    i32 condreg = exp2AnyReg(cond);

                    // TEST指令：如果条件为假则跳过then块
                    codeABC(OpCode::TEST, condreg, 0, 0);
                    i32 jf = jump();
                    freeReg(condreg);

                    // then块
                    block(branch.body);

                    // 跳过else块
                    escapelist.push_back(jump());

                    // 回填假值跳转
                    patchToHere(jf);
                }

                // else块
                if (!arg.elseBranch.empty()) {
                    block(arg.elseBranch);
                }

                // 回填所有escape跳转
                for (i32 jmp : escapelist) {
                    patchToHere(jmp);
                }
            }
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            // while循环
            i32 whileinit = getLabel();

            ExprDesc cond;
            expr(*arg.condition, cond);
            i32 condreg = exp2AnyReg(cond);

            codeABC(OpCode::TEST, condreg, 0, 0);
            i32 condexit = jump();
            freeReg(condreg);

            block(arg.body);

            codeAsBx(OpCode::JMP, 0, whileinit - getLabel() - 1);
            patchToHere(condexit);
        }
        else if constexpr (std::is_same_v<T, DoStmt>) {
            // do块
            block(arg.body);
        }
        else if constexpr (std::is_same_v<T, ForNumStmt>) {
            // 数值for循环
            forNumStmt(arg);
        }
        else if constexpr (std::is_same_v<T, ForInStmt>) {
            // 泛型for循环
            forInStmt(arg);
        }
        else if constexpr (std::is_same_v<T, FunctionStmt>) {
            functionStmt(arg);
        }
        // 其他语句类型暂不支持
    }, s.variant);
}

// =====================================================================
// 二元和一元表达式代码生成
// =====================================================================

void CodeGenerator::binaryExpr(const BinaryExpr& e, ExprDesc& desc) {
    // 处理左操作数
    ExprDesc e1;
    expr(*e.left, e1);

    // 根据运算符类型处理
    BinaryExpr::Op op = e.op;

    // 短路运算符需要特殊处理
    if (op == BinaryExpr::Op::And) {
        // and: 如果左操作数为假，跳过右操作数
        // 实现: if not e1 then result = e1 else result = e2
        luaK_goiftrue(e1);
        ExprDesc e2;
        expr(*e.right, e2);
        luaK_dischargevars(e2);
        luaK_concat(e2.f, e1.f);
        desc = e2;
        return;
    }
    else if (op == BinaryExpr::Op::Or) {
        // or: 如果左操作数为真，跳过右操作数
        // 实现: if e1 then result = e1 else result = e2
        luaK_goiffalse(e1);
        ExprDesc e2;
        expr(*e.right, e2);
        luaK_dischargevars(e2);
        luaK_concat(e2.t, e1.t);
        desc = e2;
        return;
    }

    // 对于其他运算符，先处理左操作数
    if (op == BinaryExpr::Op::Concat) {
        // 字符串连接需要操作数在栈上
        exp2NextReg(e1);
    } else {
        // 算术和比较运算符：转换为RK格式
        exp2RK(e1);
    }

    // 处理右操作数
    ExprDesc e2;
    expr(*e.right, e2);

    // 生成对应的指令
    switch (op) {
        case BinaryExpr::Op::Add:
            codearith(OpCode::ADD, e1, e2);
            break;
        case BinaryExpr::Op::Sub:
            codearith(OpCode::SUB, e1, e2);
            break;
        case BinaryExpr::Op::Mul:
            codearith(OpCode::MUL, e1, e2);
            break;
        case BinaryExpr::Op::Div:
            codearith(OpCode::DIV, e1, e2);
            break;
        case BinaryExpr::Op::Mod:
            codearith(OpCode::MOD, e1, e2);
            break;
        case BinaryExpr::Op::Pow:
            codearith(OpCode::POW, e1, e2);
            break;
        case BinaryExpr::Op::Concat:
            exp2NextReg(e2);
            codearith(OpCode::CONCAT, e1, e2);
            break;
        case BinaryExpr::Op::Eq:
            codecomp(OpCode::EQ, 1, e1, e2);
            break;
        case BinaryExpr::Op::Ne:
            codecomp(OpCode::EQ, 0, e1, e2);
            break;
        case BinaryExpr::Op::Lt:
            codecomp(OpCode::LT, 1, e1, e2);
            break;
        case BinaryExpr::Op::Le:
            codecomp(OpCode::LE, 1, e1, e2);
            break;
        case BinaryExpr::Op::Gt:
            codecomp(OpCode::LT, 1, e2, e1);  // a > b 等价于 b < a
            break;
        case BinaryExpr::Op::Ge:
            codecomp(OpCode::LE, 1, e2, e1);  // a >= b 等价于 b <= a
            break;
        default:
            break;
    }

    desc = e1;
}

void CodeGenerator::unaryExpr(const UnaryExpr& e, ExprDesc& desc) {
    // 处理操作数
    ExprDesc e1;
    expr(*e.operand, e1);

    // 创建虚拟的第二操作数（值为0）
    ExprDesc e2;
    e2.kind = ExprKind::Number;
    e2.u.nval = 0;
    e2.t = NO_JUMP;
    e2.f = NO_JUMP;

    switch (e.op) {
        case UnaryExpr::Op::Neg:
            // 取负：如果是数值常量，可以直接取负
            if (e1.kind != ExprKind::Number) {
                exp2AnyReg(e1);
            }
            codearith(OpCode::UNM, e1, e2);
            break;
        case UnaryExpr::Op::Not:
            // 逻辑非
            codenot(e1);
            break;
        case UnaryExpr::Op::Len:
            // 长度运算符：不能对常量操作
            exp2AnyReg(e1);
            codearith(OpCode::LEN, e1, e2);
            break;
    }

    desc = e1;
}

// =====================================================================
// 辅助函数：算术和比较指令生成
// =====================================================================

void CodeGenerator::codearith(OpCode op, ExprDesc& e1, ExprDesc& e2) {
    i32 o2 = (op != OpCode::UNM && op != OpCode::LEN) ? exp2RK(e2) : 0;
    i32 o1 = exp2RK(e1);

    if (o1 > o2) {
        freeReg(o1);
        freeReg(o2);
    } else {
        freeReg(o2);
        freeReg(o1);
    }

    e1.u.s.info = codeABC(op, 0, o1, o2);
    e1.kind = ExprKind::Relocatable;
}

void CodeGenerator::codecomp(OpCode op, i32 cond, ExprDesc& e1, ExprDesc& e2) {
    i32 o1 = exp2RK(e1);
    i32 o2 = exp2RK(e2);

    freeReg(o1);
    freeReg(o2);

    // ⭐ 关键修复：比较指令后面必须跟JMP指令
    // 参考 lua_c_analysis/src/lcode.c:277-280 condjump函数
    codeABC(op, cond, o1, o2);  // 生成比较指令（LE/LT/EQ）
    e1.u.s.info = jump();        // 生成JMP指令
    e1.kind = ExprKind::Jump;
    e1.t = e1.u.s.info;
    e1.f = NO_JUMP;
}

void CodeGenerator::codenot(ExprDesc& e) {
    luaK_dischargevars(e);

    switch (e.kind) {
        case ExprKind::Nil:
        case ExprKind::False:
            e.kind = ExprKind::True;
            break;
        case ExprKind::True:
        case ExprKind::Number:
        case ExprKind::Const:
            e.kind = ExprKind::False;
            break;
        case ExprKind::Jump:
            invertJump(e);
            break;
        default: {
            discharge(e, allocReg());
            i32 pc = codeABC(OpCode::NOT, 0, e.u.s.info, 0);
            e.u.s.info = pc;
            e.kind = ExprKind::Relocatable;
            break;
        }
    }
}

// =====================================================================
// 辅助函数：跳转处理
// =====================================================================

void CodeGenerator::luaK_goiftrue(ExprDesc& e) {
    luaK_dischargevars(e);

    switch (e.kind) {
        case ExprKind::Jump:
            invertJump(e);
            break;
        case ExprKind::True:
        case ExprKind::Number:
        case ExprKind::Const:
            // 常量真值：无需跳转
            break;
        default: {
            i32 pc = jumponcond(e, 0);  // 如果为假则跳转
            luaK_concat(e.f, pc);
            patchtohere(e.t);
            e.t = NO_JUMP;
            break;
        }
    }
}

void CodeGenerator::luaK_goiffalse(ExprDesc& e) {
    luaK_dischargevars(e);

    switch (e.kind) {
        case ExprKind::Jump:
            // 已经是跳转：无需处理
            break;
        case ExprKind::Nil:
        case ExprKind::False:
            // 常量假值：无需跳转
            break;
        default: {
            i32 pc = jumponcond(e, 1);  // 如果为真则跳转
            luaK_concat(e.t, pc);
            patchtohere(e.f);
            e.f = NO_JUMP;
            break;
        }
    }
}

void CodeGenerator::luaK_dischargevars(ExprDesc& e) {
    switch (e.kind) {
        case ExprKind::Local:
            // 局部变量：转换为非可重定位表达式（已经在寄存器中）
            e.kind = ExprKind::NonRelocatable;
            break;
        case ExprKind::Global:
            // 全局变量访问：生成GETGLOBAL指令
            // GETGLOBAL A Bx：R(A) := Gbl[Kst(Bx)]
            // A = 分配的寄存器（freereg_）
            // Bx = 全局变量名在常量表中的索引
            e.u.s.info = codeABx(OpCode::GETGLOBAL, 0, e.u.s.info);
            e.kind = ExprKind::Relocatable;
            break;
        case ExprKind::Indexed:
            // 表索引访问：生成GETTABLE指令
            // GETTABLE A B C：R(A) := R(B)[RK(C)]
            // A = 分配的寄存器（freereg_）
            // B = 表的寄存器（e.u.s.info）
            // C = 键的RK操作数（e.u.s.aux）
            e.u.s.info = codeABC(OpCode::GETTABLE, 0, e.u.s.info, e.u.s.aux);
            e.kind = ExprKind::Relocatable;
            break;
        case ExprKind::Upval:
            // Upvalue访问：生成GETUPVAL指令
            e.u.s.info = codeABC(OpCode::GETUPVAL, 0, e.u.s.info, 0);
            e.kind = ExprKind::Relocatable;
            break;
        default:
            break;
    }
}

void CodeGenerator::luaK_concat(i32& l1, i32 l2) {
    if (l2 == NO_JUMP) return;
    if (l1 == NO_JUMP) {
        l1 = l2;
    } else {
        i32 list = l1;
        i32 next;
        while ((next = getjump(list)) != NO_JUMP) {
            list = next;
        }
        fixjump(list, l2);
    }
}

void CodeGenerator::invertJump(ExprDesc& e) {
    std::swap(e.t, e.f);
}

i32 CodeGenerator::jumponcond(ExprDesc& e, i32 cond) {
    if (e.kind == ExprKind::Relocatable) {
        Instruction inst = proto_->getInstruction(e.u.s.info);
        OpCode op = GET_OPCODE(inst);
        if (op == OpCode::NOT) {
            // 优化：移除NOT指令，直接使用TEST
            // 注意：这里简化处理，实际应该修改指令
            return condjump(OpCode::TEST, GETARG_B(inst), 0, !cond);
        }
    }

    discharge(e, allocReg());
    freeReg(e.u.s.info);
    return condjump(OpCode::TESTSET, NO_REG, e.u.s.info, cond);
}

i32 CodeGenerator::condjump(OpCode op, i32 a, i32 b, i32 c) {
    codeABC(op, a, b, c);
    i32 jpc = jpc_;
    jpc_ = NO_JUMP;
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    luaK_concat(j, jpc);
    return j;
}

void CodeGenerator::patchtohere(i32 list) {
    pc_ = static_cast<i32>(proto_->getInstructionCount());
    luaK_concat(jpc_, list);
}

void CodeGenerator::luaK_getlabel() {
    pc_ = static_cast<i32>(proto_->getInstructionCount());
}

i32 CodeGenerator::getjump(i32 pc) {
    Instruction inst = proto_->getInstruction(pc);
    i32 offset = GETARG_sBx(inst);
    if (offset == NO_JUMP) {
        return NO_JUMP;
    } else {
        return (pc + 1) + offset;
    }
}

void CodeGenerator::fixjump(i32 pc, i32 dest) {
    Instruction jmp = proto_->getInstruction(pc);
    i32 offset = dest - (pc + 1);
    if (offset > MAXARG_sBx || offset < -MAXARG_sBx) {
        throw std::runtime_error("control structure too long");
    }
    SETARG_sBx(jmp, offset);
    proto_->setInstruction(pc, jmp);
}

// =====================================================================
// 函数定义和调用
// =====================================================================

Proto* CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body) {
    // 保存当前编译状态
    Proto* savedProto = proto_;
    i32 savedFreereg = freereg_;
    i32 savedNactvar = nactvar_;
    Vec<LocalVar> savedLocalVars = std::move(localVars_);
    i32 savedPc = pc_;

    // 创建新的Proto
    Proto* newProto = new Proto();
    newProto->setNumParams(static_cast<u8>(params.size()));
    newProto->setVararg(isVararg);

    // 切换到新的编译上下文
    proto_ = newProto;
    freereg_ = 0;
    nactvar_ = 0;
    localVars_.clear();
    pc_ = 0;

    // 添加参数作为局部变量
    for (const Str& param : params) {
        addLocalVar(param);
    }
    adjustLocalVars(static_cast<i32>(params.size()));

    // 编译函数体
    block(body);

    // 添加隐式return（如果函数体没有显式return）
    if (proto_->getInstructionCount() == 0 ||
        GET_OPCODE(proto_->getInstruction(proto_->getInstructionCount() - 1)) != OpCode::RETURN) {
        codeABC(OpCode::RETURN, 0, 1, 0);
    }

    // 设置最大栈大小
    newProto->setMaxStackSize(static_cast<u8>(freereg_));

    // 恢复编译状态
    proto_ = savedProto;
    freereg_ = savedFreereg;
    nactvar_ = savedNactvar;
    localVars_ = std::move(savedLocalVars);
    pc_ = savedPc;

    return newProto;
}

void CodeGenerator::functionExpr(const FunctionExpr& e, ExprDesc& desc) {
    // 编译函数体，生成新的Proto
    Proto* funcProto = compileFunction(e.params, e.isVararg, e.body);

    // 将Proto添加到当前Proto的子函数列表
    i32 protoIdx = static_cast<i32>(proto_->addProto(funcProto));

    // 生成CLOSURE指令
    i32 reg = allocReg();
    codeABx(OpCode::CLOSURE, reg, protoIdx);

    desc.kind = ExprKind::NonRelocatable;
    desc.u.s.info = reg;
}

void CodeGenerator::callExpr(const CallExpr& e, ExprDesc& desc) {
    i32 funcReg;
    i32 nargs = static_cast<i32>(e.args.size());

    // 检查是否为方法调用（obj:method(args)）
    if (e.isMethodCall) {
        // 方法调用：使用SELF指令
        // func应该是MemberExpr（obj.method）
        const MemberExpr* memberExpr = std::get_if<MemberExpr>(&e.func->variant);
        if (!memberExpr) {
            throw std::runtime_error("Method call must have MemberExpr as func");
        }

        // 计算对象表达式
        ExprDesc obj;
        expr(*memberExpr->table, obj);

        // 创建方法名的常量表达式
        ExprDesc key;
        key.kind = ExprKind::Const;
        key.u.s.info = stringConstant(memberExpr->member);
        key.t = NO_JUMP;
        key.f = NO_JUMP;

        // 生成SELF指令
        // SELF会将对象放入func+1，方法放入func
        luaK_self(obj, key);
        funcReg = obj.u.s.info;

        // 参数数量+1（包含隐式的self参数）
        nargs++;
    } else {
        // 普通函数调用
        ExprDesc func;
        expr(*e.func, func);
        funcReg = exp2AnyReg(func);
    }

    // 计算参数
    for (const auto& arg : e.args) {
        ExprDesc argDesc;
        expr(*arg, argDesc);
        exp2NextReg(argDesc);
    }

    // 生成CALL指令
    // CALL A B C: 调用R(A)，B-1个参数，C-1个返回值
    // B=0表示参数到栈顶，C=0表示返回值到栈顶
    codeABC(OpCode::CALL, funcReg, nargs + 1, 2);  // 默认1个返回值

    // 释放参数寄存器
    freeRegs(nargs);

    // 函数调用结果在funcReg
    desc.kind = ExprKind::Call;
    desc.u.s.info = funcReg;
}

void CodeGenerator::functionStmt(const FunctionStmt& s) {
    // 编译函数体
    Proto* funcProto = compileFunction(s.params, s.isVararg, s.body);

    // 将Proto添加到当前Proto的子函数列表
    i32 protoIdx = static_cast<i32>(proto_->addProto(funcProto));

    if (s.isLocal) {
        // 局部函数：local function name() end
        // 先添加局部变量
        addLocalVar(s.name);

        // 生成CLOSURE指令
        i32 reg = nactvar_;
        codeABx(OpCode::CLOSURE, reg, protoIdx);

        // 激活局部变量
        adjustLocalVars(1);
    } else {
        // 全局函数：function name() end
        // 生成CLOSURE指令到临时寄存器
        i32 reg = allocReg();
        codeABx(OpCode::CLOSURE, reg, protoIdx);

        // 设置全局变量
        i32 k = stringConstant(s.name);
        codeABx(OpCode::SETGLOBAL, reg, k);

        freeReg(reg);
    }
}

void CodeGenerator::forNumStmt(const ForNumStmt& s) {
    // 数值for循环的字节码模式：
    // R(base) = init
    // R(base+1) = limit
    // R(base+2) = step
    // FORPREP base sBx    ; R(base) -= step, pc += sBx
    // <loop body>
    // FORLOOP base sBx    ; R(base) += step, if R(base) <= limit then pc += sBx

    i32 base = freereg_;  // 循环变量的基址

    // 计算init, limit, step并存储到R(base), R(base+1), R(base+2)
    ExprDesc initDesc;
    expr(*s.init, initDesc);
    exp2NextReg(initDesc);  // R(base)

    ExprDesc limitDesc;
    expr(*s.limit, limitDesc);
    exp2NextReg(limitDesc);  // R(base+1)

    if (s.step) {
        ExprDesc stepDesc;
        expr(*s.step, stepDesc);
        exp2NextReg(stepDesc);  // R(base+2)
    } else {
        // 默认步长为1
        i32 stepReg = allocReg();
        codeABx(OpCode::LOADK, stepReg, numberConstant(1.0));
    }

    // 添加循环变量作为局部变量（R(base+3)）
    addLocalVar(s.var);
    adjustLocalVars(1);

    // 生成FORPREP指令（跳转到FORLOOP）
    i32 prep = codeAsBx(OpCode::FORPREP, base, 0);  // sBx稍后回填

    // 生成循环体
    i32 bodyStart = getLabel();
    block(s.body);

    // 生成FORLOOP指令（跳转回循环开始）
    i32 loop = codeAsBx(OpCode::FORLOOP, base, bodyStart - getLabel() - 1);

    // 回填FORPREP的跳转目标（跳到FORLOOP）
    fixjump(prep, loop);

    // 移除循环变量
    removeLocalVars(nactvar_ - 1);

    // 释放寄存器
    freeRegs(4);  // init, limit, step, var
}

void CodeGenerator::forInStmt(const ForInStmt& s) {
    // 泛型for循环的字节码模式：
    // R(base) = iterator_func
    // R(base+1) = state
    // R(base+2) = control_var
    // JMP -> TFORLOOP
    // loop:
    // R(base+3), ..., R(base+3+nvars-1) = loop variables
    // <loop body>
    // TFORLOOP base nvars
    // JMP -> loop

    // 参考lua_c_analysis/src/lcode.c中的forbody()和forlist()

    i32 base = freereg_;  // 迭代器变量的基址
    i32 nvars = static_cast<i32>(s.vars.size());  // 循环变量数量

    // 计算迭代器表达式（应该返回3个值：func, state, var）
    // 例如：for k, v in pairs(t) do ... end
    // pairs(t) 返回 (next, t, nil)

    if (s.iterators.size() != 1) {
        throw std::runtime_error("CodeGenerator: for-in loop requires exactly 1 iterator expression");
    }

    // 计算迭代器表达式
    ExprDesc iterDesc;
    expr(*s.iterators[0], iterDesc);

    // 迭代器表达式应该返回3个值，我们需要确保它们在连续的寄存器中
    // 对于函数调用，exp2NextReg会处理多返回值
    if (iterDesc.kind == ExprKind::Call) {
        // 函数调用，需要3个返回值
        // 确保返回值存储在R(base), R(base+1), R(base+2)
        discharge(iterDesc, base);
        freereg_ = base + 3;  // 预留3个寄存器
    } else {
        // 不是函数调用，这是错误的
        throw std::runtime_error("CodeGenerator: for-in loop iterator must be a function call");
    }

    // 添加循环变量作为局部变量（R(base+3), R(base+4), ...）
    for (const Str& var : s.vars) {
        addLocalVar(var);
    }
    adjustLocalVars(nvars);

    // 跳转到TFORLOOP（跳过循环体）
    i32 jmpToTfor = codeAsBx(OpCode::JMP, 0, 0);  // sBx稍后回填

    // 循环体开始
    i32 loopStart = getLabel();
    block(s.body);

    // 回填JMP到TFORLOOP的跳转目标
    patchToHere(jmpToTfor);

    // 生成TFORLOOP指令
    codeABC(OpCode::TFORLOOP, base, 0, nvars);

    // 生成JMP回循环开始
    codeAsBx(OpCode::JMP, 0, loopStart - getLabel() - 1);

    // 移除循环变量
    removeLocalVars(nactvar_ - nvars);

    // 释放寄存器
    freeRegs(3 + nvars);  // func, state, var, loop_vars
}

void CodeGenerator::block(const Vec<StmtPtr>& stmts) {
    i32 oldnactvar = nactvar_;

    for (const auto& stmt : stmts) {
        statement(*stmt);
    }

    removeLocalVars(oldnactvar);
}

// =====================================================================
// 表索引和方法调用
// =====================================================================

/**
 * @brief 处理表索引操作（table[key]）
 *
 * 参考：lua_c_analysis/src/lcode.c:2283 luaK_indexed
 *
 * 将表达式标记为表索引访问，准备后续的GETTABLE或SETTABLE操作。
 * 这是实现Lua表访问语法的基础函数。
 *
 * 设置过程：
 * 1. 将键表达式转换为RK操作数（可以是寄存器或常量）
 * 2. 存储键的RK值到aux字段
 * 3. 设置表达式类型为VINDEXED
 *
 * RK操作数优化：
 * - 键可以是寄存器或常量
 * - 常量键直接编码在指令中（使用ISK位标记）
 * - 减少LOADK指令的生成
 *
 * 表达式状态：
 * - t.kind = Indexed：标记为表索引
 * - t.u.s.info：表的寄存器索引
 * - t.u.s.aux：键的RK操作数
 *
 * 后续操作：
 * - GETTABLE：读取表元素（在discharge或exp2anyreg中生成）
 * - SETTABLE：设置表元素（在luaK_storevar中生成）
 *
 * @param t 表表达式描述符（输入输出参数）
 * @param k 键表达式描述符
 */
void CodeGenerator::luaK_indexed(ExprDesc& t, ExprDesc& k) {
    // 将键转换为RK操作数格式
    t.u.s.aux = exp2RK(k);
    // 设置表达式类型为索引表达式
    t.kind = ExprKind::Indexed;
}

/**
 * @brief 处理方法调用的SELF指令生成（obj:method(args)）
 *
 * 参考：lua_c_analysis/src/lcode.c:1906 luaK_self
 *
 * Lua方法调用语法糖：
 * - obj:method(args) 等价于 obj.method(obj, args)
 * - SELF指令同时完成两个操作，避免重复表访问
 *
 * SELF指令格式：
 * - SELF A B C: R(A+1) := R(B); R(A) := R(B)[RK(C)]
 * - A: 目标寄存器（存放方法）
 * - B: 对象所在寄存器
 * - C: 方法名的RK操作数
 *
 * 执行效果：
 * 1. R(A+1) = R(B)：复制对象到A+1（作为self参数）
 * 2. R(A) = R(B)[RK(C)]：获取方法到A（作为函数）
 *
 * 优化说明：
 * - 避免两次表访问（相比 obj.method(obj, args)）
 * - 自动处理self参数的传递
 * - 为后续CALL指令准备好函数和第一个参数
 *
 * 调用序列：
 * 1. SELF A B C：准备方法和self
 * 2. [加载其他参数到A+2, A+3, ...]
 * 3. CALL A nargs+1 nresults：调用方法
 *
 * @param e 对象表达式描述符（输入输出参数）
 * @param key 方法名表达式描述符
 */
void CodeGenerator::luaK_self(ExprDesc& e, ExprDesc& key) {
    // 将对象表达式放入任意寄存器
    exp2AnyReg(e);

    // 释放对象表达式占用的资源（如果是VNONRELOC）
    if (e.kind == ExprKind::NonRelocatable) {
        freeReg(e.u.s.info);
    }

    // 分配函数寄存器（连续分配2个：func和self）
    i32 func = freereg_;
    freereg_ += 2;  // 保留2个寄存器
    if (freereg_ > proto_->getMaxStackSize()) {
        proto_->setMaxStackSize(static_cast<u8>(freereg_));
    }

    // 生成SELF指令
    // SELF func obj method_key
    // R(func+1) = R(obj); R(func) = R(obj)[RK(method_key)]
    codeABC(OpCode::SELF, func, e.u.s.info, exp2RK(key));

    // 释放键表达式（如果是VNONRELOC）
    if (key.kind == ExprKind::NonRelocatable) {
        freeReg(key.u.s.info);
    }

    // 更新表达式描述符
    e.u.s.info = func;  // 函数在func寄存器
    e.kind = ExprKind::NonRelocatable;  // 固定在func寄存器
}

/**
 * @brief 存储值到变量
 *
 * 参考：lua_c_analysis/src/lcode.c:1827
 *
 * 根据变量类型生成相应的存储指令。这是赋值操作的核心实现，
 * 提供统一的接口处理所有类型变量的赋值代码生成。
 *
 * 变量类型处理：
 * - Local（局部变量）：直接存储到指定寄存器，无需生成指令（最高效）
 * - Global（全局变量）：生成 SETGLOBAL 指令
 * - Upval（Upvalue）：生成 SETUPVAL 指令
 * - Indexed（表索引）：生成 SETTABLE 指令
 *
 * 指令格式：
 * - 局部变量：无指令，直接寄存器赋值（通过 discharge）
 * - 全局变量：SETGLOBAL A Bx - Gbl[Kst(Bx)] := R(A)
 * - Upvalue：SETUPVAL A B - UpValue[B] := R(A)
 * - 表索引：SETTABLE A B C - R(A)[RK(B)] := RK(C)
 *
 * 优化策略：
 * - 局部变量赋值最高效（无指令开销）
 * - 表索引使用 RK 操作数优化（键和值都可以是常量）
 * - 自动选择最优的操作数格式
 *
 * 资源管理：
 * - 释放表达式占用的临时寄存器
 * - 确保寄存器使用的正确性
 * - 避免寄存器泄漏
 *
 * 使用场景：
 * - 赋值语句：a = 10, g = 20, t[k] = 30
 * - 变量初始化：local x = 10
 * - 表元素设置：t.field = value
 * - 函数返回值赋值：x = func()
 *
 * @param var 目标变量的表达式描述符
 * @param ex 要存储的值的表达式描述符
 */
void CodeGenerator::luaK_storevar(ExprDesc& var, ExprDesc& ex) {
    switch (var.kind) {
        case ExprKind::Local: {
            // 局部变量：直接存储到指定寄存器
            // 释放表达式占用的临时寄存器（如果是 VNONRELOC）
            if (ex.kind == ExprKind::NonRelocatable) {
                freeReg(ex.u.s.info);
            }
            // 将值存储到局部变量的寄存器
            discharge(ex, var.u.s.info);
            return;  // 注意：局部变量处理后直接返回，不执行后面的 freeReg
        }

        case ExprKind::Upval: {
            // Upvalue：生成 SETUPVAL 指令
            // SETUPVAL A B：UpValue[B] := R(A)
            i32 e = exp2AnyReg(ex);
            codeABC(OpCode::SETUPVAL, e, var.u.s.info, 0);
            break;
        }

        case ExprKind::Global: {
            // 全局变量：生成 SETGLOBAL 指令
            // SETGLOBAL A Bx：Gbl[Kst(Bx)] := R(A)
            // A = 值所在的寄存器
            // Bx = 全局变量名在常量表中的索引
            i32 e = exp2AnyReg(ex);
            codeABx(OpCode::SETGLOBAL, e, var.u.s.info);
            break;
        }

        case ExprKind::Indexed: {
            // 表索引：生成 SETTABLE 指令
            // SETTABLE A B C：R(A)[RK(B)] := RK(C)
            // A = 表的寄存器（存储在 var.u.s.info）
            // B = 键的 RK 操作数（存储在 var.u.s.aux）
            // C = 值的 RK 操作数
            i32 e = exp2RK(ex);
            codeABC(OpCode::SETTABLE, var.u.s.info, var.u.s.aux, e);
            break;
        }

        default:
            // 不应该到达这里
            throw std::runtime_error("Invalid variable type for assignment");
    }

    // 释放表达式占用的临时寄存器（除了局部变量，已经在上面返回）
    if (ex.kind == ExprKind::NonRelocatable) {
        freeReg(ex.u.s.info);
    }
}

}  // namespace Lua

