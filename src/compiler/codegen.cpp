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
    , currentBlock_(nullptr)
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
    proto_->setVararg(true);     // 主函数（chunk）默认是可变参数的

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
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:2886-2898 luaK_code实现
    // 在生成指令前，必须先修补所有待处理的跳转（jpc_）
    dischargejpc();

    Instruction inst = CREATE_ABC(op, a, b, c);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(0);  // TODO: 添加实际行号
    return pc;
}

i32 CodeGenerator::codeABx(OpCode op, i32 a, i32 bx) {
    // ⭐ P0修复：在生成指令前修补待处理的跳转
    dischargejpc();

    Instruction inst = CREATE_ABx(op, a, bx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(0);  // TODO: 添加实际行号
    return pc;
}

i32 CodeGenerator::codeAsBx(OpCode op, i32 a, i32 sbx) {
    // ⭐ P0修复：在生成指令前修补待处理的跳转
    dischargejpc();

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

void CodeGenerator::checkStack(i32 n) {
    // ⭐ P0修复：检查并更新maxStackSize
    // 参考：lua_c_analysis/src/lcode.c:790-797 luaK_checkstack()
    //
    // 作用：确保栈有足够空间容纳 freereg_ + n 个寄存器
    // 这是修复 vm.cpp:858 hack 的关键函数
    //
    // Lua 5.1 实现：
    //   int newstack = fs->freereg + n;
    //   if (newstack > fs->f->maxstacksize) {
    //       if (newstack >= MAXSTACK)
    //           luaX_syntaxerror(fs->ls, "function or expression too complex");
    //       fs->f->maxstacksize = cast_byte(newstack);
    //   }

    i32 newstack = freereg_ + n;
    if (newstack > static_cast<i32>(proto_->getMaxStackSize())) {
        // TODO: 添加MAXSTACK检查（当前简化版本）
        proto_->setMaxStackSize(static_cast<u8>(newstack));
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
    freereg_++;  // ⭐ P0修复：添加局部变量后需要递增freereg_
    checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
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
    checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
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
    checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
}

// =====================================================================
// 跳转管理
// =====================================================================

i32 CodeGenerator::jump() {
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:212-219 luaK_jump实现
    i32 jpc = jpc_;  // 保存跳转到这里的列表
    jpc_ = NO_JUMP;  // 清空jpc_
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);  // 生成JMP指令
    luaK_concat(j, jpc);  // 将jpc链表连接到j后面
    return j;
}

void CodeGenerator::patchList(i32 list, i32 target) {
    while (list != NO_JUMP) {
        i32 next = getjump(list);  // ⭐ P0修复：使用getjump获取下一个跳转的绝对位置
        fixjump(list, target);     // ⭐ P0修复：使用fixjump修补跳转
        list = next;
    }
}

void CodeGenerator::dischargejpc() {
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:608-611 dischargejpc实现
    // 将所有待处理的跳转（jpc_）修补到当前位置
    i32 target = static_cast<i32>(proto_->getInstructionCount());
    patchList(jpc_, target);
    jpc_ = NO_JUMP;
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
            // 参考：lua_c_analysis/src/lparser.c suffixedexp → luaK_exp2anyreg + yindex + luaK_indexed
            // 1. 计算表表达式
            ExprDesc t;
            expr(*arg.table, t);
            // 2. 将表表达式放入寄存器（必须是寄存器，不能是Relocatable）
            // ⭐ 关键修复：luaK_dischargevars只将Global转为Relocatable（info=pc），
            // 但luaK_indexed需要info是寄存器编号。必须调用exp2AnyReg确保在寄存器中。
            luaK_dischargevars(t);
            exp2AnyReg(t);
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
            // 参考：lua_c_analysis/src/lparser.c suffixedexp → luaK_exp2anyreg + checkname + luaK_indexed
            // 1. 计算表表达式
            ExprDesc t;
            expr(*arg.table, t);
            // 2. 将表表达式放入寄存器（必须是寄存器，不能是Relocatable）
            // ⭐ 关键修复：同IndexExpr，必须确保表在寄存器中
            luaK_dischargevars(t);
            exp2AnyReg(t);
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
        else if constexpr (std::is_same_v<T, TableExpr>) {
            tableExpr(arg, desc);
        }
        else if constexpr (std::is_same_v<T, VarargExpr>) {
            // ⭐ 可变参数表达式：生成 VARARG 指令
            // 参考：lua_c_analysis/src/lparser.c simpleexp() case TK_DOTS
            // VARARG A B：R(A), R(A+1), ..., R(A+B-2) = vararg
            // A = 0（稍后由 discharge 重定位）
            // B = 1（默认复制 0 个结果，即 B-1=0；由上层按需调整为实际数量）
            // 检查当前函数是否为可变参数函数
            if (!proto_->isVararg()) {
                throw std::runtime_error("CodeGenerator: cannot use '...' outside a vararg function");
            }
            i32 pc = codeABC(OpCode::VARARG, 0, 1, 0);
            desc.kind = ExprKind::Vararg;
            desc.u.s.info = pc;
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
        case ExprKind::Relocatable: {
            // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:1447-1450
            // Relocatable表达式的结果位置可以重定位
            // 需要修改之前生成的指令，将目标寄存器从0改为reg
            i32 pc = desc.u.s.info;  // 指令的位置
            Instruction inst = proto_->getInstruction(pc);
            // 修改指令的A参数（目标寄存器）
            SETARG_A(inst, reg);
            // 写回修改后的指令
            proto_->setInstruction(pc, inst);
            break;
        }
        case ExprKind::NonRelocatable:
            // ⭐ 字节码修复：NonRelocatable表达式需要MOVE指令
            // 参考：lua_c_analysis/src/lcode.c:1447-1450 discharge2reg函数
            //
            // NonRelocatable表示表达式结果已经在某个寄存器中（desc.u.s.info）
            // 如果目标寄存器不同，需要生成MOVE指令将值移动到目标寄存器
            //
            // 这是函数调用参数处理的关键：
            // - sum在R2（NonRelocatable，info=2）
            // - 需要移动到R5（函数参数位置）
            // - 生成：MOVE 5 2
            if (desc.u.s.info != reg) {
                codeABC(OpCode::MOVE, reg, desc.u.s.info, 0);
            }
            break;
        case ExprKind::Vararg:
        case ExprKind::Call: {
            // ⭐ Vararg/Call 表达式：与 Relocatable 类似，修改之前生成指令的 A 字段
            // 参考：lua_c_analysis/src/lcode.c discharge2reg() VCALL/VVARARG 分支
            i32 pc = (desc.kind == ExprKind::Call) ? desc.u.s.aux : desc.u.s.info;
            Instruction inst = proto_->getInstruction(pc);
            SETARG_A(inst, reg);
            proto_->setInstruction(pc, inst);
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
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:1659-1670
    // 如果表达式已经在寄存器中（NonRelocatable或Local），直接返回寄存器编号
    // 避免生成不必要的MOVE指令

    // 先处理变量访问（Local -> NonRelocatable）
    if (desc.kind == ExprKind::Local) {
        // Local变量已经在寄存器中，直接返回
        return desc.u.s.info;
    }

    // ⭐ 关键修复：函数调用表达式的结果已经在寄存器中
    // callExpr 会设置 desc.kind = ExprKind::Call，u.s.info = base（函数所在的寄存器）
    // 返回值就位于 base，不需要重新分配寄存器
    if (desc.kind == ExprKind::Call) {
        return desc.u.s.info;  // 返回函数所在的寄存器（返回值也在这里）
    }

    if (desc.kind == ExprKind::NonRelocatable) {
        return desc.u.s.info;
    }

    // 其他情况：分配新寄存器并discharge
    i32 reg = allocReg();
    discharge(desc, reg);
    return reg;
}

void CodeGenerator::exp2NextReg(ExprDesc& desc) {
    exp2Val(desc);

    // 如果表达式已经位于当前“下一个寄存器”前一个位置，
    // 说明它已经是连续参数区中的最后一个值，不需要再额外 MOVE。
    // 这对嵌套调用参数尤其重要，例如 print(type(print))。
    if (desc.kind == ExprKind::NonRelocatable && desc.u.s.info == freereg_ - 1) {
        return;
    }

    i32 reg = allocReg();
    discharge(desc, reg);
}

void CodeGenerator::exp2Val(ExprDesc& desc) {
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:1702-1707
    // exp2Val 调用 luaK_dischargevars，处理 Local/Vararg/Call 等需要"求值"的表达式类型
    // 确保多返回值表达式被固定为单一返回值
    if (desc.kind == ExprKind::Vararg || desc.kind == ExprKind::Call) {
        luaK_dischargevars(desc);
    } else if (desc.kind == ExprKind::Local) {
        desc.kind = ExprKind::NonRelocatable;
    }
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
            // ⭐ 局部变量声明
            // 参考：lua_c_analysis/src/lparser.c localstat() 函数
            i32 nvars = static_cast<i32>(arg.names.size());
            i32 nexps = static_cast<i32>(arg.values.size());

            // ⭐ 关键修复：保存 nactvar_ 的初始值（第一个变量的寄存器索引）
            i32 base = nactvar_;

            //std::fprintf(stderr, "DEBUG LocalStmt: nvars=%d, nexps=%d, base=%d, freereg=%d\n",
            //             nvars, nexps, base, freereg_);

            // ⭐ P0修复：在编译表达式之前，临时调整freereg_
            // 保存当前的freereg_（可能包含其他临时寄存器）
            i32 savedFreereg = freereg_;

            // 设置freereg_为base，这样表达式会从base开始分配寄存器
            freereg_ = base;

            // 为每个变量分配寄存器（addLocalVar 会递增 freereg_）
            for (i32 i = 0; i < nvars; i++) {
                addLocalVar(arg.names[i]);
            }

            //std::fprintf(stderr, "DEBUG LocalStmt: after addLocalVar, freereg=%d\n", freereg_);

            // ⭐ P0修复：重新设置freereg_为base
            // 这样在编译表达式时，寄存器会从base开始分配
            freereg_ = base;

            //std::fprintf(stderr, "DEBUG LocalStmt: reset freereg to %d\n", freereg_);

            // 生成初始化代码
            bool allVarsInitialized = false;  // 标记是否所有变量都已初始化
            if (nexps > 0) {
                // 处理前 nexps-1 个表达式（每个表达式对应一个变量）
                for (i32 i = 0; i < nexps - 1 && i < nvars; i++) {
                    ExprDesc val;
                    expr(*arg.values[i], val);
                    discharge(val, base + i);  // ⭐ 使用 base + i 而不是 nactvar_ + i
                }

                // 处理最后一个表达式（可能是多返回值表达式）
                if (nexps <= nvars) {
                    ExprDesc val;
                    expr(*arg.values[nexps - 1], val);

                    // 如果是多返回值表达式（Vararg 或 Call），调整返回值数量
                    if (val.kind == ExprKind::Vararg || val.kind == ExprKind::Call) {
                        // 需要 nvars - (nexps - 1) 个返回值
                        i32 wanted = nvars - (nexps - 1);
                        i32 targetReg = base + (nexps - 1);  // ⭐ 使用 base 而不是 nactvar_

                        // ⭐ P0修复：不要修改CALL指令的A参数！
                        // 相反，修改C参数（返回值数量），然后生成MOVE指令将返回值移动到目标寄存器
                        // ⭐ P0修复：对于Call表达式，PC存储在aux字段中，函数寄存器存储在info字段中
                        i32 pc = (val.kind == ExprKind::Call) ? val.u.s.aux : val.u.s.info;
                        i32 funcReg = (val.kind == ExprKind::Call) ? val.u.s.info : -1;
                        Instruction inst = proto_->getInstruction(pc);
                        i32 oldA = GETARG_A(inst);

                        //std::fprintf(stderr, "DEBUG LocalStmt: CALL at pc=%d, oldA=%d, funcReg=%d, targetReg=%d, wanted=%d\n",
                        //             pc, oldA, funcReg, targetReg, wanted);

                        if (val.kind == ExprKind::Vararg) {
                            // VARARG A B：修改A和B参数
                            SETARG_A(inst, targetReg);
                            SETARG_B(inst, wanted + 1);
                            proto_->setInstruction(pc, inst);
                        } else {
                            // CALL A B C：修改A和C参数
                            // ⭐ P0修复：由于我们在编译表达式之前设置了freereg_=base，
                            // 函数和参数应该已经在正确的位置（base, base+1, base+2, ...）
                            // 所以只需要修改A和C参数即可
                            SETARG_A(inst, targetReg);
                            SETARG_C(inst, wanted + 1);
                            proto_->setInstruction(pc, inst);

                            // std::fprintf(stderr, "DEBUG LocalStmt: modified CALL to A=%d, C=%d\n",
                            //             targetReg, wanted + 1);
                        }

                        // ⭐ 关键修复：多返回值表达式会初始化所有剩余变量
                        // 标记所有变量都已初始化，避免后续生成 LOADNIL 指令
                        allVarsInitialized = true;
                    } else {
                        // 普通表达式
                        discharge(val, base + (nexps - 1));  // ⭐ 使用 base 而不是 nactvar_
                    }
                }
            }

            // 未初始化的变量设为nil
            // ⭐ 关键修复：如果最后一个表达式是多返回值表达式（Vararg/Call），
            // 它已经初始化了所有剩余变量，不需要再生成 LOADNIL
            if (nexps < nvars && !allVarsInitialized) {
                codeABC(OpCode::LOADNIL, base + nexps, base + nvars - 1, 0);  // ⭐ 使用 base 而不是 nactvar_
            }

            // ⭐ P0修复：恢复freereg_
            freereg_ = savedFreereg;

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
            // ⭐ P0修复：参考lua_c_analysis/src/lparser.c:5522-5542 ifstat实现
            // if语句的正确处理逻辑
            if (arg.branches.empty()) {
                return;
            }

            i32 escapelist = NO_JUMP;  // 所有分支结束后的跳转列表
            i32 flist = NO_JUMP;       // 当前分支条件为假时的跳转列表

            // 处理第一个if分支
            {
                const auto& branch = arg.branches[0];
                ExprDesc cond;
                expr(*branch.condition, cond);

                // ⭐ P0修复：参考lua_c_analysis/src/lparser.c:4628 cond函数
                // 生成条件为假时跳转的代码（使用luaK_goiftrue）
                luaK_goiftrue(cond);

                // then块
                block(branch.body);

                // 保存条件为假时的跳转列表
                flist = cond.f;
            }

            // 处理elseif分支
            for (size_t i = 1; i < arg.branches.size(); i++) {
                // 在处理下一个分支之前，生成跳过后续分支的JMP
                luaK_concat(escapelist, jump());

                // ⭐ 关键修复：使用patchtohere（小写h）而不是patchToHere（大写H）
                // patchtohere将跳转添加到jpc_，在下一次生成指令时自动修补
                patchtohere(flist);

                const auto& branch = arg.branches[i];
                ExprDesc cond;
                expr(*branch.condition, cond);

                // ⭐ P0修复：生成条件为假时跳转的代码（使用luaK_goiftrue）
                luaK_goiftrue(cond);

                // then块
                block(branch.body);

                // 保存条件为假时的跳转列表
                flist = cond.f;
            }

            // 处理else块
            if (!arg.elseBranch.empty()) {
                // 在处理else块之前，生成跳过else块的JMP
                luaK_concat(escapelist, jump());

                // ⭐ 关键修复：使用patchtohere（小写h）
                patchtohere(flist);

                // else块
                block(arg.elseBranch);
            } else {
                // 没有else块：将最后一个分支的假值跳转添加到escapelist
                luaK_concat(escapelist, flist);
            }

            // ⭐ 关键修复：使用patchtohere（小写h）
            patchtohere(escapelist);
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            // ⭐ P0修复：参考lua_c_analysis/src/lparser.c:4808-4823 whilestat实现
            // while循环
            i32 whileinit = getLabel();

            // ⭐ 关键修复：使用cond函数的逻辑（参考lua_c_analysis/src/lparser.c:4625-4630）
            // 生成条件表达式并调用luaK_goiftrue，返回假值跳转列表
            ExprDesc cond;
            expr(*arg.condition, cond);
            if (cond.kind == ExprKind::Nil) {
                cond.kind = ExprKind::False;
            }
            luaK_goiftrue(cond);
            i32 condexit = cond.f;  // 保存假值跳转列表

            // 进入可break的代码块
            enterBlock(true);  // isbreakable = true

            block(arg.body);

            // 生成回跳到循环开始
            patchList(jump(), whileinit);

            // 离开代码块，修补所有break跳转
            leaveBlock();

            // ⭐ 关键修复：使用patchtohere（延迟修补），修补条件为假时的跳转
            // 参考官方Lua的whilestat实现（lua_c_analysis/src/lparser.c:4822）
            patchtohere(condexit);
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
        else if constexpr (std::is_same_v<T, CallStmt>) {
            // ⭐ P0修复：函数调用语句（如 print("Hello")）
            // 参考 lua_c_analysis/src/lparser.c 中的 exprstat 函数
            ExprDesc desc;
            expr(*arg.call, desc);

            // 对于作为语句的函数调用，需要丢弃返回值
            // 修改CALL指令的C参数为1，表示0个返回值
            if (desc.kind == ExprKind::Call) {
                // 找到最后生成的CALL指令并修改其C参数
                usize lastInst = proto_->getInstructionCount() - 1;
                Instruction inst = proto_->getInstruction(lastInst);
                if (GET_OPCODE(inst) == OpCode::CALL) {
                    // 重新设置C参数为1（表示0个返回值）
                    i32 a = GETARG_A(inst);
                    i32 b = GETARG_B(inst);
                    proto_->setInstruction(lastInst, CREATE_ABC(OpCode::CALL, a, b, 1));
                }
                // 释放函数寄存器
                freeReg(desc.u.s.info);
            }
        }
        else if constexpr (std::is_same_v<T, EmptyStmt>) {
            // 空语句，不生成代码
        }
        else if constexpr (std::is_same_v<T, BreakStmt>) {
            // ⭐ P0修复：参考lua_c_analysis/src/lparser.c:4712-4725 breakstat实现
            // 查找最近的可break代码块
            BlockInfo* bl = currentBlock_;
            while (bl && !bl->isbreakable) {
                bl = bl->previous;
            }

            // 如果没有找到可break的代码块，报错
            if (!bl) {
                throw std::runtime_error("no loop to break");
            }

            // 生成跳转指令并添加到break列表
            // 注意：官方Lua还会处理upvalue关闭（OP_CLOSE），但我们暂时不支持upvalue
            luaK_concat(bl->breaklist, jump());
        }
        else if constexpr (std::is_same_v<T, RepeatStmt>) {
            // repeat-until 语句，暂时抛出错误
            throw std::runtime_error("repeat-until statement not yet implemented");
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

    // ⭐ 负索引修复：特殊处理 -(数字常量) 的情况
    // 参考官方 Lua 5.1.5 行为：负数字面量应该直接作为常量，而非运行时计算
    //
    // 问题场景：arg[-1] 被解析为 arg[-(1)]
    // - 官方 Lua：词法分析器将 -1 识别为单个 NUMBER token
    // - 我们的实现：解析为 UnaryExpr(MINUS, 1)
    //
    // 修复策略：在代码生成阶段优化这种模式
    // - 检测到 UnaryExpr(Neg, Number) 时
    // - 直接将负数作为常量处理
    // - 避免生成 UNM 指令
    if (e.op == UnaryExpr::Op::Neg && e1.kind == ExprKind::Number) {
        // 直接取负数值，作为常量
        desc.kind = ExprKind::Number;
        desc.u.nval = -e1.u.nval;
        desc.t = NO_JUMP;
        desc.f = NO_JUMP;
        return;
    }

    // 创建虚拟的第二操作数（值为0）
    ExprDesc e2;
    e2.kind = ExprKind::Number;
    e2.u.nval = 0;
    e2.t = NO_JUMP;
    e2.f = NO_JUMP;

    switch (e.op) {
        case UnaryExpr::Op::Neg:
            // 取负：如果是数值常量，可以直接取负（已在上面处理）
            // 这里处理非常量的情况
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
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:2509-2522 codecomp实现
    i32 o1 = exp2RK(e1);
    i32 o2 = exp2RK(e2);

    freeReg(o1);
    freeReg(o2);

    // ⭐ 关键修复：当cond=0且op!=EQ时，交换参数并将cond改为1
    // 这样可以统一使用cond=1，简化后续处理
    if (cond == 0 && op != OpCode::EQ) {
        std::swap(o1, o2);  // 交换操作数
        cond = 1;
    }

    // 生成比较指令（LE/LT/EQ）后跟JMP指令
    codeABC(op, cond, o1, o2);
    e1.u.s.info = jump();  // 生成JMP指令，存储位置到e1.u.s.info
    e1.kind = ExprKind::Jump;
    // ⭐ 关键修复：不设置e1.t和e1.f，让它们保持之前的值
    // luaK_goiffalse会正确处理这些跳转列表
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
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:2073-2094 luaK_goiftrue实现
    luaK_dischargevars(e);

    i32 pc;  // 最后跳转的pc
    switch (e.kind) {
        case ExprKind::True:
        case ExprKind::Number:
        case ExprKind::Const:
            // 常量真值：无需跳转
            pc = NO_JUMP;
            break;
        case ExprKind::Jump:
            // ⭐ 关键修复：对于Jump类型，反转跳转并获取pc
            invertJump(e);
            pc = e.u.s.info;
            break;
        default:
            // 其他类型：生成条件跳转
            pc = jumponcond(e, 0);  // 如果为假则跳转
            break;
    }
    // ⭐ 关键修复：将最后跳转插入`f'列表（参考lcode.c:2091）
    luaK_concat(e.f, pc);
    patchtohere(e.t);
    e.t = NO_JUMP;
}

void CodeGenerator::luaK_goiffalse(ExprDesc& e) {
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:2136-2156 luaK_goiffalse实现
    // luaK_goiffalse的语义：生成"如果为假则跳转"的代码
    // - 将新的跳转添加到 f 列表（false跳转列表）
    // - 修补 t 列表（true跳转列表）到当前位置
    luaK_dischargevars(e);

    i32 pc;  // 最后跳转的pc
    switch (e.kind) {
        case ExprKind::Nil:
        case ExprKind::False:
            // 永远为假：无需操作
            pc = NO_JUMP;
            break;
        case ExprKind::Jump:
            // ⭐ 关键修复：VJMP类型需要反转跳转条件
            // codecomp生成的是"如果为真则跳过JMP"，我们需要"如果为假则执行JMP"
            // 所以需要反转比较指令的A参数
            invertJump(e);
            pc = e.u.s.info;
            break;
        default:
            // 其他类型：生成条件跳转指令
            pc = jumponcond(e, 1);  // 如果为真则跳转
            break;
    }

    // ⭐ 关键修复：将最后跳转插入到 f 列表（false跳转列表）
    // 注意：这里与luaK_goiftrue相反！
    luaK_concat(e.f, pc);
    // 修补 t 列表（true跳转列表）到当前位置
    patchtohere(e.t);
    e.t = NO_JUMP;
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
        case ExprKind::Vararg: {
            // ⭐ 可变参数：设置 B=2（默认返回 1 个结果），转换为 Relocatable
            // 参考：lua_c_analysis/src/lcode.c luaK_dischargevars() VVARARG 分支
            // VARARG A B：B-1 = 结果数量，B=2 表示 1 个结果
            i32 pc = e.u.s.info;
            Instruction inst = proto_->getInstruction(pc);
            SETARG_B(inst, 2);  // B=2 → 复制 1 个值
            proto_->setInstruction(pc, inst);
            e.kind = ExprKind::Relocatable;
            break;
        }
        case ExprKind::Call: {
            // ⭐ 函数调用：设置 C=2（默认返回 1 个结果），转换为 NonRelocatable
            // 参考：lua_c_analysis/src/lcode.c luaK_dischargevars() VCALL 分支
            i32 pc = e.u.s.aux;
            Instruction inst = proto_->getInstruction(pc);
            SETARG_C(inst, 2);  // C=2 → 返回 1 个值
            proto_->setInstruction(pc, inst);
            e.kind = ExprKind::NonRelocatable;
            e.u.s.info = GETARG_A(inst);
            break;
        }
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
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:1972-1977 invertjump实现
    // 反转跳转条件：修改比较指令的A参数

    // 获取跳转控制指令（比较指令，位于JMP之前）
    i32 pc = e.u.s.info;  // JMP指令的位置
    if (pc > 0) {
        i32 controlPc = pc - 1;  // 比较指令的位置
        Instruction inst = proto_->getInstruction(controlPc);
        OpCode op = GET_OPCODE(inst);

        // 只有比较指令才能反转
        if (op == OpCode::EQ || op == OpCode::LT || op == OpCode::LE) {
            i32 a = GETARG_A(inst);
            // 反转A参数：0 -> 1, 1 -> 0
            SETARG_A(inst, !a);
            // 写回修改后的指令
            proto_->setInstruction(controlPc, inst);
        }
    }

    // 交换真假跳转列表
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
    // ⭐ P0修复：参考lua_c_analysis/src/lcode.c:477-486 patchtestreg实现
    // 当TESTSET的A参数为NO_REG时，应该使用TEST指令
    // 这是因为NO_REG(255)是无效的寄存器索引，会导致VM运行时错误
    if (op == OpCode::TESTSET && a == NO_REG) {
        // 转换为TEST指令：TEST A B C
        // TESTSET的B参数变为TEST的A参数（要测试的寄存器）
        op = OpCode::TEST;
        a = b;
        b = 0;
    }

    codeABC(op, a, b, c);
    i32 jpc = jpc_;
    jpc_ = NO_JUMP;
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    luaK_concat(j, jpc);  // ⭐ 将jpc链表连接到j后面
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

// 辅助函数：获取语句块的最后一行号
static i32 getLastLineOfBlock(const Vec<StmtPtr>& body) {
    if (body.empty()) {
        return 0;
    }
    return body.back()->getLine();
}

Proto* CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                     i32 linedefined, i32 lastlinedefined) {
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
    newProto->setLineDefined(linedefined);
    newProto->setLastLineDefined(lastlinedefined);

    // 继承父Proto的源文件名
    if (savedProto != nullptr) {
        newProto->setSource(savedProto->getSource());
    }

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
    // 计算函数定义的行号范围
    i32 linedefined = e.line;
    i32 lastlinedefined = getLastLineOfBlock(e.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;  // 空函数体的情况
    }

    // 编译函数体，生成新的Proto
    Proto* funcProto = compileFunction(e.params, e.isVararg, e.body, linedefined, lastlinedefined);

    // 将Proto添加到当前Proto的子函数列表
    i32 protoIdx = static_cast<i32>(proto_->addProto(funcProto));

    // 生成CLOSURE指令
    i32 reg = allocReg();
    codeABx(OpCode::CLOSURE, reg, protoIdx);

    desc.kind = ExprKind::NonRelocatable;
    desc.u.s.info = reg;
}

void CodeGenerator::callExpr(const CallExpr& e, ExprDesc& desc) {
    // ⭐ 字节码修复：函数调用参数寄存器分配
    // 参考：lua_c_analysis/src/lparser.c:3356-3401 funcargs函数
    //
    // Lua调用约定：
    // - 函数必须在寄存器base
    // - 参数必须连续分配在base+1, base+2, ..., base+nargs
    // - CALL指令格式：CALL base (nargs+1) (nresults+1)
    //
    // 修复前的问题：
    // - 函数在某个寄存器（如R4）
    // - 参数使用exp2NextReg分配，可能不连续（如R2, R3）
    // - CALL 4 2 1 会从R5读取参数，但参数实际在R2
    //
    // 修复策略：
    // 1. 确保函数在base寄存器
    // 2. 保存当前freereg，然后设置freereg=base+1
    // 3. 对每个参数调用exp2NextReg，自动连续分配到base+1, base+2, ...
    // 4. 生成CALL指令
    // 5. 重置freereg=base+1（保留返回值寄存器）

    i32 base;  // 函数所在的寄存器（也是调用帧的基址）
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
        // SELF A B C: R(A+1) := R(B); R(A) := R(B)[RK(C)]
        // SELF会将对象放入base+1，方法放入base
        luaK_self(obj, key);
        base = obj.u.s.info;

        // 参数数量+1（包含隐式的self参数，已经在base+1）
        nargs++;
    } else {
        // 普通函数调用
        ExprDesc func;
        expr(*e.func, func);

        // 确保函数表达式在寄存器中（NonRelocatable）
        base = exp2AnyReg(func);
    }

    // ⭐ 关键修复：确保参数连续分配在base+1之后
    // 参考官方实现：lua_c_analysis/src/lparser.c:3394-3396
    //
    // 官方代码：
    //   if (args.k != VVOID)
    //       luaK_exp2nextreg(fs, &args);
    //   nparams = fs->freereg - (base+1);
    //
    // 这里的关键是：
    // 1. 函数已经在base寄存器，占用了一个寄存器位置
    // 2. 调用exp2NextReg时，会自动分配到freereg位置
    // 3. 如果freereg > base+1，说明中间有其他寄存器被占用
    // 4. 需要确保freereg == base+1，这样参数才能紧跟函数

    // 保存当前的freereg（可能有其他表达式占用的寄存器）
    i32 savedFreeReg = freereg_;

    // 强制freereg = base + 1，确保参数从base+1开始分配
    // 这样第一个参数会分配到base+1，第二个到base+2，依此类推
    freereg_ = base + 1;
    checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_

    // 计算每个参数，exp2NextReg会将它们连续分配到base+1, base+2, ...
    for (const auto& arg : e.args) {
        ExprDesc argDesc;
        expr(*arg, argDesc);
        exp2NextReg(argDesc);  // 现在会连续分配到base+1, base+2, base+3, ...
    }

    // 此时freereg应该等于base+1+nargs
    // 验证参数数量（调试用）
    i32 actualNargs = freereg_ - (base + 1);
    if (actualNargs != nargs) {
        // 理论上不应该发生，除非有多返回值表达式
        // 暂时使用实际计算的参数数量
        nargs = actualNargs;
    }

    // 生成CALL指令
    // CALL A B C: 调用R(A)，B-1个参数（在R(A+1)...R(A+B-1)），C-1个返回值
    // - A = base（函数寄存器）
    // - B = nargs + 1（参数数量+1）
    // - C = 2（期望1个返回值，C-1=1）
    i32 callPC = codeABC(OpCode::CALL, base, nargs + 1, 2);

    // ⭐ 关键修复：调用后重置freereg
    // 参考官方实现：lua_c_analysis/src/lparser.c:3400
    //   fs->freereg = base+1;
    //
    // 原因：
    // 1. CALL指令执行后，返回值会存储在base寄存器
    // 2. 参数寄存器（base+1...base+nargs）不再需要，可以释放
    // 3. 设置freereg=base+1，保留base寄存器（存放返回值）
    // 4. 下一个表达式可以从base+1开始分配新寄存器
    freereg_ = base + 1;
    checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_

    // ⭐ P0修复：ExprKind::Call需要同时存储CALL指令的PC和函数所在的寄存器
    // 参考：lua_c_analysis/src/lcode.c luaK_exp2nextreg() VCALL分支
    // info: 函数所在的寄存器（返回值也在这里，用于exp2AnyReg）
    // aux: CALL指令的PC（用于LocalStmt修改指令）
    desc.kind = ExprKind::Call;
    desc.u.s.info = base;     // 存储函数所在的寄存器（返回值也在这里）
    desc.u.s.aux = callPC;    // 存储CALL指令的PC
}

/**
 * @brief 表构造器代码生成
 *
 * 参考 lua_c_analysis/src/lparser.c constructor() 函数实现。
 * 生成 NEWTABLE 指令创建表，然后：
 * - 对于数组字段（key == nullptr）：累积值到连续寄存器，用 SETLIST 批量存储
 * - 对于哈希字段（key != nullptr）：用 SETTABLE 逐个设置
 * 最后回补 NEWTABLE 指令的数组/哈希大小参数。
 */
void CodeGenerator::tableExpr(const TableExpr& table, ExprDesc& desc) {
    // 1. 生成 NEWTABLE 指令，B=0, C=0（后续回补实际大小）
    i32 pc = codeABC(OpCode::NEWTABLE, 0, 0, 0);

    // 2. 标记为 Relocatable，exp2NextReg 会将其固定到寄存器
    desc.kind = ExprKind::Relocatable;
    desc.u.s.info = pc;
    desc.t = NO_JUMP;
    desc.f = NO_JUMP;

    // 3. 将表固定到一个寄存器中
    exp2NextReg(desc);
    i32 tableReg = desc.u.s.info;

    // 4. 遍历字段，分别处理数组和哈希部分
    i32 na = 0;       // 数组元素总数
    i32 nh = 0;       // 哈希元素总数
    i32 tostore = 0;  // 待批量存储的数组元素数
    ExprDesc lastArrayExpr;  // 保存最后一个数组元素的表达式
    bool hasLastArrayExpr = false;

    for (usize i = 0; i < table.fields.size(); i++) {
        const auto& field = table.fields[i];
        bool isLastField = (i == table.fields.size() - 1);

        if (field.key) {
            // === 哈希字段：[key] = value 或 name = value ===
            i32 savedFreereg = freereg_;

            ExprDesc key;
            expr(*field.key, key);
            i32 rkKey = exp2RK(key);

            ExprDesc val;
            expr(*field.value, val);
            i32 rkVal = exp2RK(val);

            codeABC(OpCode::SETTABLE, tableReg, rkKey, rkVal);
            freereg_ = savedFreereg;  // 恢复寄存器状态
            checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
            nh++;
        } else {
            // === 数组字段：值按顺序累积到 R(tableReg+1), R(tableReg+2), ... ===
            na++;
            tostore++;

            ExprDesc val;
            expr(*field.value, val);

            // ⭐ 关键修复：检查是否为最后一个数组元素且为多返回值表达式
            // 参考：lua_c_analysis/src/lparser.c lastlistfield()
            if (isLastField && (val.kind == ExprKind::Vararg || val.kind == ExprKind::Call)) {
                // 最后一个元素是多返回值表达式，保存它稍后特殊处理
                lastArrayExpr = val;
                hasLastArrayExpr = true;
                // 不调用 exp2NextReg，保持为 Vararg/Call 状态
            } else {
                exp2NextReg(val);
            }

            // 达到批量阈值时发射 SETLIST
            if (tostore == LFIELDS_PER_FLUSH) {
                i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
                codeABC(OpCode::SETLIST, tableReg, LFIELDS_PER_FLUSH, c);
                freereg_ = tableReg + 1;  // 释放批量寄存器
                checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
                tostore = 0;
            }
        }
    }

    // 5. 发射剩余数组元素的 SETLIST
    if (tostore > 0) {
        if (hasLastArrayExpr) {
            // ⭐ 最后一个元素是多返回值表达式（Vararg/Call）
            // 参考：lua_c_analysis/src/lparser.c lastlistfield()
            // 设置多返回值模式：VARARG/CALL B=0（返回所有值）
            i32 pc = lastArrayExpr.u.s.info;
            Instruction inst = proto_->getInstruction(pc);

            // ⭐ 关键修复：设置 A 参数为 tableReg + 1（数组元素起始位置）
            SETARG_A(inst, tableReg + 1);

            if (lastArrayExpr.kind == ExprKind::Vararg) {
                // VARARG A B：B=0 表示复制所有可变参数
                SETARG_B(inst, 0);
            } else {
                // CALL A B C：C=0 表示返回所有值
                SETARG_C(inst, 0);
            }
            proto_->setInstruction(pc, inst);

            // SETLIST B=0 表示到栈顶（LUA_MULTRET）
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, 0, c);
            freereg_ = tableReg + 1;
            checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_

            // 调整 na 计数（因为实际元素数量未知）
            // na--;  // 注意：Lua 5.1 会减1，但我们保持不变以简化
        } else {
            // 普通情况：固定数量的元素
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, tostore, c);
            freereg_ = tableReg + 1;
            checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
        }
    }

    // 6. 回补 NEWTABLE 指令的大小参数 B（数组）和 C（哈希）
    Instruction inst = proto_->getInstruction(pc);
    SETARG_B(inst, na);
    SETARG_C(inst, nh);
    proto_->setInstruction(pc, inst);
}

void CodeGenerator::functionStmt(const FunctionStmt& s) {
    // 计算函数定义的行号范围
    i32 linedefined = s.line;
    i32 lastlinedefined = getLastLineOfBlock(s.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;  // 空函数体的情况
    }

    // 编译函数体
    Proto* funcProto = compileFunction(s.params, s.isVararg, s.body, linedefined, lastlinedefined);

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

    // 进入可break的代码块（在添加循环变量之前）
    enterBlock(true);  // isbreakable = true

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

    // 离开代码块，修补所有break跳转，并移除循环变量
    leaveBlock();

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
        checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_
    } else {
        // 不是函数调用，这是错误的
        throw std::runtime_error("CodeGenerator: for-in loop iterator must be a function call");
    }

    // 进入可break的代码块（在添加循环变量之前）
    enterBlock(true);  // isbreakable = true

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
    patchtohere(jmpToTfor);

    // 生成TFORLOOP指令
    codeABC(OpCode::TFORLOOP, base, 0, nvars);

    // 生成JMP回循环开始
    codeAsBx(OpCode::JMP, 0, loopStart - getLabel() - 1);

    // 离开代码块，修补所有break跳转，并移除循环变量
    leaveBlock();

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

// =====================================================================
// 代码块管理
// =====================================================================

void CodeGenerator::enterBlock(bool isbreakable) {
    // ⭐ P0修复：参考lua_c_analysis/src/lparser.c:1770-1778 enterblock实现
    // 创建新的代码块并链接到当前块
    BlockInfo* newBlock = new BlockInfo(currentBlock_, nactvar_, isbreakable);
    currentBlock_ = newBlock;
}

void CodeGenerator::leaveBlock() {
    // ⭐ P0修复：参考lua_c_analysis/src/lparser.c:1852-1862 leaveblock实现
    if (currentBlock_ == nullptr) {
        throw std::runtime_error("No block to leave");
    }

    BlockInfo* bl = currentBlock_;

    // 恢复父级代码块
    currentBlock_ = bl->previous;

    // 移除当前代码块中声明的局部变量
    removeLocalVars(bl->nactvar);

    // 恢复寄存器分配状态
    freereg_ = nactvar_;
    checkStack(0);  // ⭐ P0修复：确保maxStackSize >= freereg_

    // 修补所有break跳转到当前位置
    patchtohere(bl->breaklist);

    // 释放代码块
    delete bl;
}

}  // namespace Lua

