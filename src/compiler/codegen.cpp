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
    return jpc;
}

void CodeGenerator::patchList(i32 list, i32 target) {
    while (list != NO_JUMP) {
        i32 next = GETARG_sBx(proto_->getInstruction(list));
        Instruction& inst = proto_->getCode()[list];
        SETARG_sBx(inst, target - list - 1);
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
                desc.kind = ExprKind::Local;
                desc.u.s.info = reg;
            } else {
                // 全局变量
                i32 k = stringConstant(arg.name);
                desc.kind = ExprKind::Indexed;
                desc.u.s.info = k;
            }
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            binaryExpr(arg, desc);
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            unaryExpr(arg, desc);
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
        case ExprKind::Indexed: {
            // 全局变量读取
            codeABx(OpCode::GETGLOBAL, reg, desc.u.s.info);
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
            // 赋值语句：简化实现，只支持单个变量赋值
            if (!arg.targets.empty() && !arg.values.empty()) {
                ExprDesc val;
                expr(*arg.values[0], val);

                // 检查目标是否是局部变量
                if (auto* nameExpr = std::get_if<NameExpr>(&arg.targets[0]->variant)) {
                    i32 reg = findLocalVar(nameExpr->name);
                    if (reg >= 0) {
                        // 局部变量赋值
                        discharge(val, reg);
                    } else {
                        // 全局变量赋值
                        i32 k = stringConstant(nameExpr->name);
                        i32 valReg = exp2AnyReg(val);
                        codeABx(OpCode::SETGLOBAL, valReg, k);
                        freeReg(valReg);
                    }
                }
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

    e1.u.s.info = codeABC(op, cond, o1, o2);
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
            e.kind = ExprKind::NonRelocatable;
            break;
        case ExprKind::Indexed:
            e.u.s.info = codeABx(OpCode::GETGLOBAL, 0, e.u.s.info);
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

void CodeGenerator::block(const Vec<StmtPtr>& stmts) {
    i32 oldnactvar = nactvar_;

    for (const auto& stmt : stmts) {
        statement(*stmt);
    }

    removeLocalVars(oldnactvar);
}

}  // namespace Lua

