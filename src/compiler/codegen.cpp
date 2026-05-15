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

// Forward declaration of static helper
static i32 getLastLineOfBlock(const Vec<StmtPtr>& body);

// =====================================================================
// 构造和析构
// =====================================================================

CodeGenerator::CodeGenerator(StringPool* pool)
    : pool_(pool)
    , parent_(nullptr)
    , proto_(nullptr)
    , regs_()
    , locals_()
    , blocks_()
    , upvalueCtx_()
    , pc_(0)
    , currentLine_(0)
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

Proto* CodeGenerator::generate(const Chunk& chunk, StrView sourceName) {
    // 创建新的Proto对象
    proto_ = new Proto();
    regs_.bind(proto_);
    proto_->setMaxStackSize(2);  // 最小栈大小
    proto_->setVararg(true);     // 主函数（chunk）默认是可变参数的
    if (!sourceName.empty()) {
        proto_->setSource(pool_->intern(sourceName));
    }

    // 重置状态
    regs_.setFreeReg(0);
    locals_.nactvar_ = 0;
    locals_.localVars_.clear();
    upvalueCtx_.upvalues_.clear();
    pc_ = 0;
    currentLine_ = 0;
    
    // 生成语句块
    block(chunk.statements);
    
    // 添加RETURN指令（如果最后一条指令不是RETURN）
    if (proto_->getInstructionCount() == 0 || 
        GET_OPCODE(proto_->getInstruction(proto_->getInstructionCount() - 1)) != OpCode::RETURN) {
        codeABC(OpCode::RETURN, 0, 1, 0);  // return (no values)
    }

    attachDebugMetadata();
    
    return proto_;
}

// =====================================================================
// 指令生成
// =====================================================================

i32 CodeGenerator::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    // 在生成指令前刷新所有待处理跳转
    flushPendingJumps();

    Instruction inst = CREATE_ABC(op, a, b, c);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(currentLine_);
    return pc;
}

i32 CodeGenerator::codeABx(OpCode op, i32 a, i32 bx) {
    // 在生成指令前刷新待处理跳转
    flushPendingJumps();

    Instruction inst = CREATE_ABx(op, a, bx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(currentLine_);
    return pc;
}

i32 CodeGenerator::codeAsBx(OpCode op, i32 a, i32 sbx) {
    // 在生成指令前刷新待处理跳转
    flushPendingJumps();

    Instruction inst = CREATE_AsBx(op, a, sbx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(currentLine_);
    return pc;
}

// =====================================================================
// 寄存器管理
// =====================================================================

i32 CodeGenerator::allocReg() {
    return regs_.alloc();
}

void CodeGenerator::freeReg(i32 reg) {
    regs_.freeReg(reg, locals_.nactvar_);
}

void CodeGenerator::freeRegs(i32 n) {
    regs_.freeRegs(n);
}

void CodeGenerator::checkStack(i32 n) {
    regs_.checkStack(n);
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
    i32 reg = regs_.current();
    locals_.localVars_.emplace_back(name, reg, static_cast<i32>(proto_->getInstructionCount()));
    regs_.reserve(1);
    checkStack(0);
    return reg;
}

i32 CodeGenerator::findLocalVar(const Str& name) {
    return locals_.findLocal(name);
}

void CodeGenerator::adjustLocalVars(i32 nvars) {
    locals_.nactvar_ += nvars;
    regs_.resetToLocals(locals_.nactvar_);
    regs_.checkStack(0);
}

void CodeGenerator::removeLocalVars(i32 tolevel) {
    closeScopeUpvalues(tolevel);
    i32 pc = static_cast<i32>(proto_->getInstructionCount());
    locals_.closeLocals(tolevel, pc);
    regs_.resetToLocals(locals_.nactvar_);
    regs_.checkStack(0);
}

// =====================================================================
// 跳转管理
// =====================================================================

i32 CodeGenerator::jump() {
    i32 jpc = blocks_.jpc_;  // 保存当前待处理跳转列表
    blocks_.jpc_ = NO_JUMP;  // 清空blocks_.jpc_
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);  // 生成JMP指令
    concatJumpList(j, jpc);  // 将jpc链表连接到j后面
    return j;
}

void CodeGenerator::patchList(i32 list, i32 target) {
    while (list != NO_JUMP) {
        i32 next = getjump(list);
        fixjump(list, target);
        list = next;
    }
}

void CodeGenerator::patchList(const PatchList& list, i32 target) {
    for (i32 pc : list.pcs) {
        fixjump(pc, target);
    }
}

void CodeGenerator::flushPendingJumps() {
    // 将所有待处理跳转修补到当前指令位置
    i32 target = static_cast<i32>(proto_->getInstructionCount());
    patchList(blocks_.jpc_, target);
    blocks_.jpc_ = NO_JUMP;
}

i32 CodeGenerator::getLabel() {
    return static_cast<i32>(proto_->getInstructionCount());
}

// =====================================================================
// 条件代码生成（emitCond通道）
// =====================================================================

i32 CodeGenerator::emitCond(const Expr& e) {
    return emitCondResult(e).falseList.front();
}

CondResult CodeGenerator::emitCondResult(const Expr& e) {
    CondResult result;

    if (const auto* binary = std::get_if<BinaryExpr>(&e.variant)) {
        switch (binary->op) {
            case BinaryExpr::Op::And: {
                CondResult left = emitCondResult(*binary->left);
                CondResult right = emitCondResult(*binary->right);
                result.falseList = PatchList::merge(left.falseList, right.falseList);
                return result;
            }

            case BinaryExpr::Op::Or: {
                CondResult left = emitCondResultTrue(*binary->left);
                CondResult right = emitCondResult(*binary->right);
                patchtohere(left.trueList);
                result.falseList = right.falseList;
                return result;
            }

            case BinaryExpr::Op::Eq:
            case BinaryExpr::Op::Ne:
            case BinaryExpr::Op::Lt:
            case BinaryExpr::Op::Le:
            case BinaryExpr::Op::Gt:
            case BinaryExpr::Op::Ge:
                result.falseList = emitComparisonJump(*binary, false);
                return result;

            default:
                break;
        }
    }

    if (const auto* unary = std::get_if<UnaryExpr>(&e.variant)) {
        if (unary->op == UnaryExpr::Op::Not) {
            CondResult inner = emitCondResultTrue(*unary->operand);
            result.falseList = inner.trueList;
            return result;
        }
    }

    // PR-6: native ValueResult fallback
    ValueResult val = emitValue(e);
    val = forceSingleValue(val);

    if (val.kind == ValueResult::Kind::Immediate) {
        if (val.immediate == ValueResult::ImmediateKind::Nil ||
            (val.immediate == ValueResult::ImmediateKind::Boolean && !val.boolValue)) {
            // 常假值 — 无条件跳转到 falseList
            result.falseList.append(jump());
        }
        // 常真值（true / number）— 无条件通过，falseList 为空
    }
    else if (val.kind == ValueResult::Kind::Constant) {
        // 字符串常量始终为真
    }
    else {
        i32 reg = valueToAnyReg(val);
        // TEST reg 0 0: truthy → skip JMP (fall through = true), falsy → exec JMP (→ falseList)
        codeABC(OpCode::TEST, reg, 0, 0);
        result.falseList.append(jump());
        freeReg(reg);
    }

    return result;
}

CondResult CodeGenerator::emitCondResultTrue(const Expr& e) {
    CondResult result;

    if (const auto* binary = std::get_if<BinaryExpr>(&e.variant)) {
        switch (binary->op) {
            case BinaryExpr::Op::And: {
                CondResult left = emitCondResult(*binary->left);
                CondResult right = emitCondResultTrue(*binary->right);
                patchtohere(left.falseList);
                result.trueList = right.trueList;
                return result;
            }

            case BinaryExpr::Op::Or: {
                CondResult left = emitCondResultTrue(*binary->left);
                CondResult right = emitCondResultTrue(*binary->right);
                result.trueList = PatchList::merge(left.trueList, right.trueList);
                return result;
            }

            case BinaryExpr::Op::Eq:
            case BinaryExpr::Op::Ne:
            case BinaryExpr::Op::Lt:
            case BinaryExpr::Op::Le:
            case BinaryExpr::Op::Gt:
            case BinaryExpr::Op::Ge:
                result.trueList = emitComparisonJump(*binary, true);
                return result;

            default:
                break;
        }
    }

    if (const auto* unary = std::get_if<UnaryExpr>(&e.variant)) {
        if (unary->op == UnaryExpr::Op::Not) {
            CondResult inner = emitCondResult(*unary->operand);
            result.trueList = inner.falseList;
            return result;
        }
    }

    // PR-6: native ValueResult fallback
    ValueResult val = emitValue(e);
    val = forceSingleValue(val);

    if (val.kind == ValueResult::Kind::Immediate) {
        if (val.immediate == ValueResult::ImmediateKind::Boolean && val.boolValue) {
            // true — 无条件跳转到 trueList
            result.trueList.append(jump());
        }
        if (val.immediate == ValueResult::ImmediateKind::Number) {
            // number 始终为真
            result.trueList.append(jump());
        }
        // nil / false — 无条件通过，trueList 为空
    }
    else if (val.kind == ValueResult::Kind::Constant) {
        // 字符串常量始终为真
        result.trueList.append(jump());
    }
    else {
        i32 reg = valueToAnyReg(val);
        // TEST reg 0 1: falsy → skip JMP (fall through = false), truthy → exec JMP (→ trueList)
        codeABC(OpCode::TEST, reg, 0, 1);
        result.trueList.append(jump());
        freeReg(reg);
    }

    return result;
}

// =====================================================================
// 值通道（PR-4 emitValue pipeline）
// =====================================================================

ValueResult CodeGenerator::emitValue(const Expr& e) {
    i32 previousLine = currentLine_;
    i32 exprLine = e.getLine();
    if (exprLine > 0) {
        currentLine_ = exprLine;
    }

    ValueResult result;

    if (auto* nil = std::get_if<NilExpr>(&e.variant)) {
        result.kind = ValueResult::Kind::Immediate;
        result.immediate = ValueResult::ImmediateKind::Nil;
    }
    else if (auto* boolExpr = std::get_if<BoolExpr>(&e.variant)) {
        result.kind = ValueResult::Kind::Immediate;
        result.immediate = ValueResult::ImmediateKind::Boolean;
        result.boolValue = boolExpr->value;
    }
    else if (auto* numExpr = std::get_if<NumberExpr>(&e.variant)) {
        result.kind = ValueResult::Kind::Immediate;
        result.immediate = ValueResult::ImmediateKind::Number;
        result.numberValue = numExpr->value;
    }
    else if (auto* strExpr = std::get_if<StringExpr>(&e.variant)) {
        i32 k = stringConstant(strExpr->value);
        result.kind = ValueResult::Kind::Constant;
        result.constIndex = k;
    }
    else if (auto* nameExpr = std::get_if<NameExpr>(&e.variant)) {
        SymbolRef sym = resolve(nameExpr->name);
        result = symbolToValue(sym);
    }
    else if (auto* parenExpr = std::get_if<ParenExpr>(&e.variant)) {
        // Lua 5.1 语义：括号表达式将 multret 收敛为单值
        ValueResult inner = emitValue(*parenExpr->expression);
        inner = forceSingleValue(inner);
        i32 reg = valueToAnyReg(inner);
        result.kind = ValueResult::Kind::Register;
        result.reg = reg;
        result.ownsRegister = true;
    }
    else if (auto* funcExpr = std::get_if<FunctionExpr>(&e.variant)) {
        i32 linedefined = funcExpr->line;
        i32 lastlinedefined = getLastLineOfBlock(funcExpr->body);
        if (lastlinedefined < linedefined) {
            lastlinedefined = linedefined;
        }
        Vec<UpvalueCapture> childUpvalues;
        Proto* funcProto = compileFunction(funcExpr->params, funcExpr->isVararg,
                                           funcExpr->body, linedefined, lastlinedefined,
                                           &childUpvalues);
        i32 protoIdx = static_cast<i32>(proto_->addProto(funcProto));
        i32 reg = allocReg();
        codeABx(OpCode::CLOSURE, reg, protoIdx);
        emitClosureUpvalues(childUpvalues);
        result.kind = ValueResult::Kind::Register;
        result.reg = reg;
        result.ownsRegister = true;
    }
    else if (auto* indexExpr = std::get_if<IndexExpr>(&e.variant)) {
        // PR-6: table[key] 读路径 — 原生 ValueResult 通道
        result = emitValueIndex(*indexExpr);
    }
    else if (auto* memberExpr = std::get_if<MemberExpr>(&e.variant)) {
        // PR-6: table.member 读路径 — 原生 ValueResult 通道
        result = emitValueMember(*memberExpr);
    }
    else if (auto* callExpr = std::get_if<CallExpr>(&e.variant)) {
        // 函数调用 — PR-5: 直接使用 emitCallExpr 通道
        CallResultInfo info = emitCallExpr(*callExpr);
        result.kind = ValueResult::Kind::MultiRet;
        result.access = ValueResult::AccessKind::Call;
        result.reg = info.baseReg;
        result.instructionPc = info.instructionPc;
        result.isMultiResult = true;
        result.isSingleValue = false;
    }
    else if (auto* varargExpr = std::get_if<VarargExpr>(&e.variant)) {
        // vararg — PR-5: 直接使用 emitVarargExpr 通道
        CallResultInfo info = emitVarargExpr();
        result.kind = ValueResult::Kind::MultiRet;
        result.access = ValueResult::AccessKind::Vararg;
        result.instructionPc = info.instructionPc;
        result.isMultiResult = true;
        result.isSingleValue = false;
    }
    else if (auto* binaryExpr = std::get_if<BinaryExpr>(&e.variant)) {
        // PR-6: 二元表达式 — 原生 ValueResult 通道
        result = emitValueBinary(*binaryExpr);
    }
    else if (auto* unaryExpr = std::get_if<UnaryExpr>(&e.variant)) {
        // PR-6: 一元表达式 — 原生 ValueResult 通道
        result = emitValueUnary(*unaryExpr);
    }
    else if (auto* tableExpr = std::get_if<TableExpr>(&e.variant)) {
        // PR-6: 表构造器 — 原生 ValueResult 通道
        result = emitValueTable(*tableExpr);
    }
    else {
        throw std::runtime_error("emitValue: unsupported expression type");
    }

    currentLine_ = previousLine;
    return result;
}

void CodeGenerator::materializeValue(const ValueResult& val, i32 reg) {
    switch (val.kind) {
        case ValueResult::Kind::Immediate: {
            switch (val.immediate) {
                case ValueResult::ImmediateKind::Nil:
                    codeABC(OpCode::LOADNIL, reg, reg, 0);
                    break;
                case ValueResult::ImmediateKind::Boolean:
                    codeABC(OpCode::LOADBOOL, reg, val.boolValue ? 1 : 0, 0);
                    break;
                case ValueResult::ImmediateKind::Number: {
                    i32 k = numberConstant(val.numberValue);
                    codeABx(OpCode::LOADK, reg, k);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case ValueResult::Kind::Constant:
            codeABx(OpCode::LOADK, reg, val.constIndex);
            break;
        case ValueResult::Kind::Register:
            if (val.reg != reg) {
                codeABC(OpCode::MOVE, reg, val.reg, 0);
            }
            break;
        case ValueResult::Kind::PendingLoad: {
            switch (val.access) {
                case ValueResult::AccessKind::Global:
                    codeABx(OpCode::GETGLOBAL, reg, val.constIndex);
                    break;
                case ValueResult::AccessKind::Upvalue:
                    codeABC(OpCode::GETUPVAL, reg, val.aux, 0);
                    break;
                case ValueResult::AccessKind::Indexed:
                    codeABC(OpCode::GETTABLE, reg, val.reg, val.aux);
                    break;
                default:
                    break;
            }
            break;
        }
        case ValueResult::Kind::Relocatable: {
            Instruction inst = proto_->getInstruction(val.instructionPc);
            SETARG_A(inst, reg);
            proto_->setInstruction(val.instructionPc, inst);
            break;
        }
        case ValueResult::Kind::MultiRet: {
            if (val.access == ValueResult::AccessKind::Call) {
                // Call: 返回值在 baseReg，不能直接重写 A
                Instruction inst = proto_->getInstruction(val.instructionPc);
                i32 callBase = GETARG_A(inst);
                SETARG_C(inst, 2);  // 固定为 1 个返回值
                proto_->setInstruction(val.instructionPc, inst);
                if (callBase != reg) {
                    codeABC(OpCode::MOVE, reg, callBase, 0);
                }
            } else if (val.access == ValueResult::AccessKind::Vararg) {
                Instruction inst = proto_->getInstruction(val.instructionPc);
                SETARG_A(inst, reg);
                SETARG_B(inst, 2);  // 固定为 1 个值
                proto_->setInstruction(val.instructionPc, inst);
            }
            break;
        }
        case ValueResult::Kind::PendingJump: {
            // 比较表达式物化为布尔值
            i32 trueJump = val.instructionPc;
            codeABC(OpCode::LOADBOOL, reg, 0, 1);
            i32 trueLabel = getLabel();
            fixjump(trueJump, trueLabel);
            codeABC(OpCode::LOADBOOL, reg, 1, 0);
            break;
        }
        default:
            break;
    }
}

i32 CodeGenerator::valueToRK(const ValueResult& val) {
    // 常量可直接编码为 RK 操作数
    if (val.kind == ValueResult::Kind::Immediate && val.immediate == ValueResult::ImmediateKind::Number) {
        i32 k = numberConstant(val.numberValue);
        if (k <= MAXINDEXRK) {
            return RKASK(k);
        }
    }
    if (val.kind == ValueResult::Kind::Constant) {
        if (val.constIndex <= MAXINDEXRK) {
            return RKASK(val.constIndex);
        }
    }
    // 否则落到寄存器
    return valueToAnyReg(val);
}

i32 CodeGenerator::valueToAnyReg(const ValueResult& val) {
    // 已经在寄存器中则直接返回
    if (val.kind == ValueResult::Kind::Register) {
        return val.reg;
    }
    // MultiRet(Call): 返回值已在 baseReg
    if (val.kind == ValueResult::Kind::MultiRet && val.access == ValueResult::AccessKind::Call) {
        // 先固定为单值
        Instruction inst = proto_->getInstruction(val.instructionPc);
        SETARG_C(inst, 2);
        proto_->setInstruction(val.instructionPc, inst);
        return GETARG_A(inst);
    }
    // 否则分配寄存器并物化
    i32 reg = allocReg();
    materializeValue(val, reg);
    return reg;
}

void CodeGenerator::valueToNextReg(const ValueResult& val) {
    ValueResult v = forceSingleValue(val);
    if (v.kind == ValueResult::Kind::Register && v.reg == regs_.current() - 1) {
        return;  // 已在下一个位置
    }
    i32 reg = allocReg();
    materializeValue(v, reg);
}

ValueResult CodeGenerator::forceSingleValue(const ValueResult& val) {
    if (val.kind != ValueResult::Kind::MultiRet) {
        return val;
    }
    // 将 CALL/VARARG 固定为单返回值并转为 Relocatable/Register
    if (val.access == ValueResult::AccessKind::Vararg) {
        Instruction inst = proto_->getInstruction(val.instructionPc);
        SETARG_B(inst, 2);  // B=2 → 1 个值
        proto_->setInstruction(val.instructionPc, inst);
        ValueResult result;
        result.kind = ValueResult::Kind::Relocatable;
        result.instructionPc = val.instructionPc;
        return result;
    }
    if (val.access == ValueResult::AccessKind::Call) {
        Instruction inst = proto_->getInstruction(val.instructionPc);
        SETARG_C(inst, 2);  // C=2 → 1 个返回值
        proto_->setInstruction(val.instructionPc, inst);
        ValueResult result;
        result.kind = ValueResult::Kind::Register;
        result.reg = GETARG_A(inst);
        result.ownsRegister = false;
        return result;
    }
    return val;
}

// =====================================================================
// 复合表达式原生通道（PR-6 Composite Expressions Cleanup）
// =====================================================================

ValueResult CodeGenerator::emitValueBinary(const BinaryExpr& e) {
    ValueResult result;
    BinaryExpr::Op op = e.op;

    // === 比较表达式 → 条件通道 + 物化 ===
    if (op == BinaryExpr::Op::Eq || op == BinaryExpr::Op::Ne ||
        op == BinaryExpr::Op::Lt || op == BinaryExpr::Op::Le ||
        op == BinaryExpr::Op::Gt || op == BinaryExpr::Op::Ge) {
        CondResult cond;
        cond.trueList = emitComparisonJump(e, true);
        i32 resultReg = allocReg();
        materializeCondResult(cond, resultReg, false);
        result.kind = ValueResult::Kind::Register;
        result.reg = resultReg;
        result.ownsRegister = true;
        return result;
    }

    // === And/Or: 短路求值 ===
    if (op == BinaryExpr::Op::And || op == BinaryExpr::Op::Or) {
        ValueResult left = emitValue(*e.left);
        left = forceSingleValue(left);
        i32 resultReg = allocReg();
        materializeValue(left, resultReg);

        i32 testCond = (op == BinaryExpr::Op::And) ? 0 : 1;
        codeABC(OpCode::TEST, resultReg, 0, testCond);
        i32 skipRight = codeAsBx(OpCode::JMP, 0, NO_JUMP);

        ValueResult right = emitValue(*e.right);
        right = forceSingleValue(right);
        materializeValue(right, resultReg);

        fixjump(skipRight, getLabel());

        result.kind = ValueResult::Kind::Register;
        result.reg = resultReg;
        result.ownsRegister = true;
        return result;
    }

    // === Concat ===
    if (op == BinaryExpr::Op::Concat) {
        ValueResult left = emitValue(*e.left);
        left = forceSingleValue(left);
        i32 regLeft = allocReg();
        materializeValue(left, regLeft);

        ValueResult right = emitValue(*e.right);
        right = forceSingleValue(right);
        i32 regRight = allocReg();
        materializeValue(right, regRight);

        freeReg(regRight);
        freeReg(regLeft);

        i32 pc = codeABC(OpCode::CONCAT, 0, regLeft, regRight);
        result.kind = ValueResult::Kind::Relocatable;
        result.instructionPc = pc;
        return result;
    }

    // === 算术表达式: Add/Sub/Mul/Div/Mod/Pow ===
    OpCode arithOp;
    switch (op) {
        case BinaryExpr::Op::Add: arithOp = OpCode::ADD; break;
        case BinaryExpr::Op::Sub: arithOp = OpCode::SUB; break;
        case BinaryExpr::Op::Mul: arithOp = OpCode::MUL; break;
        case BinaryExpr::Op::Div: arithOp = OpCode::DIV; break;
        case BinaryExpr::Op::Mod: arithOp = OpCode::MOD; break;
        case BinaryExpr::Op::Pow: arithOp = OpCode::POW; break;
        default:
            throw std::runtime_error("emitValueBinary: unsupported binary operator");
    }

    ValueResult left = emitValue(*e.left);
    i32 rkLeft = valueToRK(left);
    ValueResult right = emitValue(*e.right);
    i32 rkRight = valueToRK(right);

    if (rkLeft > rkRight) { freeReg(rkLeft); freeReg(rkRight); }
    else                  { freeReg(rkRight); freeReg(rkLeft); }

    i32 pc = codeABC(arithOp, 0, rkLeft, rkRight);
    result.kind = ValueResult::Kind::Relocatable;
    result.instructionPc = pc;
    return result;
}

ValueResult CodeGenerator::emitValueUnary(const UnaryExpr& e) {
    ValueResult result;

    // === Not: 条件通道 + 物化 ===
    if (e.op == UnaryExpr::Op::Not) {
        CondResult cond;
        cond.trueList = emitCondResult(*e.operand).falseList;
        i32 resultReg = allocReg();
        materializeCondResult(cond, resultReg, false);
        result.kind = ValueResult::Kind::Register;
        result.reg = resultReg;
        result.ownsRegister = true;
        return result;
    }

    // === Neg: 常量折叠 ===
    if (e.op == UnaryExpr::Op::Neg) {
        ValueResult operand = emitValue(*e.operand);
        if (operand.kind == ValueResult::Kind::Immediate &&
            operand.immediate == ValueResult::ImmediateKind::Number) {
            result.kind = ValueResult::Kind::Immediate;
            result.immediate = ValueResult::ImmediateKind::Number;
            result.numberValue = -operand.numberValue;
            return result;
        }
        // 非常量: 物化到寄存器后生成 UNM
        i32 opReg = valueToAnyReg(operand);
        freeReg(opReg);
        i32 pc = codeABC(OpCode::UNM, 0, opReg, 0);
        result.kind = ValueResult::Kind::Relocatable;
        result.instructionPc = pc;
        return result;
    }

    // === Len ===
    if (e.op == UnaryExpr::Op::Len) {
        ValueResult operand = emitValue(*e.operand);
        i32 opReg = valueToAnyReg(operand);
        freeReg(opReg);
        i32 pc = codeABC(OpCode::LEN, 0, opReg, 0);
        result.kind = ValueResult::Kind::Relocatable;
        result.instructionPc = pc;
        return result;
    }

    throw std::runtime_error("emitValueUnary: unsupported unary operator");
}

ValueResult CodeGenerator::emitValueIndex(const IndexExpr& e) {
    ValueResult table = emitValue(*e.table);
    i32 tableReg = valueToAnyReg(table);

    ValueResult key = emitValue(*e.index);
    i32 rkKey = valueToRK(key);

    ValueResult result;
    result.kind = ValueResult::Kind::PendingLoad;
    result.access = ValueResult::AccessKind::Indexed;
    result.reg = tableReg;
    result.aux = rkKey;
    return result;
}

ValueResult CodeGenerator::emitValueMember(const MemberExpr& e) {
    ValueResult table = emitValue(*e.table);
    i32 tableReg = valueToAnyReg(table);

    i32 k = stringConstant(e.member);
    i32 rkKey;
    if (k <= MAXINDEXRK) {
        rkKey = RKASK(k);
    } else {
        ValueResult keyVal;
        keyVal.kind = ValueResult::Kind::Constant;
        keyVal.constIndex = k;
        rkKey = valueToAnyReg(keyVal);
    }

    ValueResult result;
    result.kind = ValueResult::Kind::PendingLoad;
    result.access = ValueResult::AccessKind::Indexed;
    result.reg = tableReg;
    result.aux = rkKey;
    return result;
}

ValueResult CodeGenerator::emitValueTable(const TableExpr& table) {
    i32 pc = codeABC(OpCode::NEWTABLE, 0, 0, 0);
    i32 tableReg = allocReg();
    // 设置 NEWTABLE 的 A 字段为 tableReg
    {
        Instruction inst = proto_->getInstruction(pc);
        SETARG_A(inst, tableReg);
        proto_->setInstruction(pc, inst);
    }

    i32 na = 0, nh = 0, tostore = 0;
    CallResultInfo lastCallResult;
    bool hasLastCallResult = false;

    for (usize i = 0; i < table.fields.size(); i++) {
        const auto& field = table.fields[i];
        bool isLastField = (i == table.fields.size() - 1);

        if (field.key) {
            // 哈希字段: SETTABLE
            i32 savedFreereg = regs_.current();
            ValueResult keyVal = emitValue(*field.key);
            i32 rkKey = valueToRK(keyVal);
            ValueResult valVal = emitValue(*field.value);
            i32 rkVal = valueToRK(valVal);
            codeABC(OpCode::SETTABLE, tableReg, rkKey, rkVal);
            regs_.restore(savedFreereg);
            checkStack(0);
            nh++;
        } else {
            // 数组字段
            na++;
            tostore++;

            if (isLastField) {
                // 最后一个数组字段: 检查是否为 Call/Vararg multret
                if (auto* callExpr = std::get_if<CallExpr>(&field.value->variant)) {
                    i32 targetBase = tableReg + tostore;
                    CallResultInfo info = emitCallExpr(*callExpr, targetBase);
                    lastCallResult = info;
                    hasLastCallResult = true;
                    continue;
                }
                else if (std::holds_alternative<VarargExpr>(field.value->variant)) {
                    CallResultInfo info = emitVarargExpr();
                    lastCallResult = info;
                    hasLastCallResult = true;
                    continue;
                }
            }

            ValueResult val = emitValue(*field.value);
            val = forceSingleValue(val);
            valueToNextReg(val);

            if (!hasLastCallResult && tostore == LFIELDS_PER_FLUSH) {
                i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
                codeABC(OpCode::SETLIST, tableReg, LFIELDS_PER_FLUSH, c);
                regs_.setFreeReg(tableReg + 1);
                checkStack(0);
                tostore = 0;
            }
        }
    }

    // 刷新剩余数组元素
    if (tostore > 0) {
        if (hasLastCallResult) {
            i32 targetBase = tableReg + tostore;
            if (lastCallResult.kind == CallResultInfo::Kind::Call) {
                Instruction inst = proto_->getInstruction(lastCallResult.instructionPc);
                i32 callBase = GETARG_A(inst);
                if (callBase != targetBase) {
                    throw std::runtime_error("CALL base mismatch in table multret field");
                }
                setOpenMultiRet(lastCallResult);
            } else {
                Instruction inst = proto_->getInstruction(lastCallResult.instructionPc);
                SETARG_A(inst, targetBase);
                proto_->setInstruction(lastCallResult.instructionPc, inst);
                setOpenMultiRet(lastCallResult);
            }
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, 0, c);
            regs_.setFreeReg(tableReg + 1);
            checkStack(0);
            na--;
        } else {
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, tostore, c);
            regs_.setFreeReg(tableReg + 1);
            checkStack(0);
        }
    }

    // 回填 NEWTABLE 的 B/C (na, nh)
    {
        Instruction inst = proto_->getInstruction(pc);
        SETARG_B(inst, na);
        SETARG_C(inst, nh);
        proto_->setInstruction(pc, inst);
    }

    ValueResult result;
    result.kind = ValueResult::Kind::Register;
    result.reg = tableReg;
    result.ownsRegister = true;
    return result;
}

// =====================================================================
// 调用/多返回值通道（PR-5 Call/Vararg/MultiRet pipeline）
// =====================================================================

CallResultInfo CodeGenerator::emitCallExpr(const CallExpr& e, i32 targetBase) {
    i32 previousLine = currentLine_;
    if (e.line > 0) {
        currentLine_ = e.line;
    }

    i32 base;  // 函数所在的寄存器（调用帧的基址）
    i32 explicitArgCount = static_cast<i32>(e.args.size());
    bool hasImplicitSelf = false;

    // 检查是否为方法调用（obj:method(args)）
    if (e.isMethodCall) {
        const MemberExpr* memberExpr = std::get_if<MemberExpr>(&e.func->variant);
        if (!memberExpr) {
            throw std::runtime_error("Method call must have MemberExpr as func");
        }

        // PR-6: native ValueResult pipeline for SELF
        ValueResult obj = emitValue(*memberExpr->table);
        i32 objReg = valueToAnyReg(obj);

        i32 methodKey = stringConstant(memberExpr->member);
        i32 rkKey = RKASK(methodKey);

        // 释放 obj 寄存器（SELF 会将其复制到 base+1）
        freeReg(objReg);

        // 分配 2 个连续寄存器：func 和 self
        base = regs_.current();
        regs_.reserve(2);
        checkStack(0);

        // SELF base objReg RK(method)
        // R(base+1) = R(objReg); R(base) = R(objReg)[RK(method)]
        codeABC(OpCode::SELF, base, objReg, rkKey);
        hasImplicitSelf = true;
    }
    else {
        ValueResult funcVal = emitValue(*e.func);
        base = valueToAnyReg(funcVal);
    }

    i32 savedFreeReg = regs_.current();

    auto moveRegRange = [this](i32 dst, i32 src, i32 count) {
        if (count <= 0 || dst == src) return;
        if (dst < src) {
            for (i32 i = 0; i < count; i++)
                codeABC(OpCode::MOVE, dst + i, src + i, 0);
        } else {
            for (i32 i = count - 1; i >= 0; i--)
                codeABC(OpCode::MOVE, dst + i, src + i, 0);
        }
    };

    if (targetBase >= 0) {
        // 表构造器等场景明确指定基址
        moveRegRange(targetBase, base, hasImplicitSelf ? 2 : 1);
        base = targetBase;
    }
    else if (base < savedFreeReg) {
        i32 newBase = savedFreeReg;
        moveRegRange(newBase, base, hasImplicitSelf ? 2 : 1);
        base = newBase;
    }

    i32 firstArgReg = hasImplicitSelf ? (base + 2) : (base + 1);
    regs_.setFreeReg(firstArgReg);
    checkStack(0);
    checkStack(explicitArgCount);

    // 编译所有实参。最后一个实参如果是 Call/Vararg 则保持 multret 打开。
    bool lastArgIsMultiRet = false;
    i32 argIndex = 0;
    for (const auto& arg : e.args) {
        i32 targetReg = firstArgReg + argIndex;
        bool isLastArg = (argIndex == explicitArgCount - 1);

        if (isLastArg) {
            // 最后一个实参：检查是否为 Call 或 Vararg
            if (auto* innerCall = std::get_if<CallExpr>(&arg->variant)) {
                // g(f()) — 最后一个实参是函数调用，保持 multret
                CallResultInfo innerInfo = emitCallExpr(*innerCall, targetReg);
                setOpenMultiRet(innerInfo);
                lastArgIsMultiRet = true;
            }
            else if (std::holds_alternative<VarargExpr>(arg->variant)) {
                // g(...) — 最后一个实参是 vararg，保持 multret
                CallResultInfo innerInfo = emitVarargExpr();
                // 设置 VARARG A 到 targetReg
                Instruction inst = proto_->getInstruction(innerInfo.instructionPc);
                SETARG_A(inst, targetReg);
                SETARG_B(inst, 0);  // B=0 → 全部 vararg
                proto_->setInstruction(innerInfo.instructionPc, inst);
                lastArgIsMultiRet = true;
            }
            else {
                // 普通最后实参
                ValueResult argVal = emitValue(*arg);
                argVal = forceSingleValue(argVal);
                materializeValue(argVal, targetReg);
            }
        }
        else {
            // 非最后实参：固定为单值
            ValueResult argVal = emitValue(*arg);
            argVal = forceSingleValue(argVal);
            materializeValue(argVal, targetReg);
        }

        regs_.ensureAtLeast(targetReg + 1);
        argIndex++;
    }

    i32 nargs = explicitArgCount + (hasImplicitSelf ? 1 : 0);

    // B: 参数数量+1；如果最后实参是 multret，B=0
    i32 bArg = lastArgIsMultiRet ? 0 : (nargs + 1);
    // C=2: 默认期望 1 个返回值（上层按需修改）
    i32 callPC = codeABC(OpCode::CALL, base, bArg, 2);

    regs_.setFreeReg((savedFreeReg > (base + 1)) ? savedFreeReg : (base + 1));
    checkStack(0);

    currentLine_ = previousLine;

    CallResultInfo result;
    result.kind = CallResultInfo::Kind::Call;
    result.baseReg = base;
    result.instructionPc = callPC;
    return result;
}

CallResultInfo CodeGenerator::emitVarargExpr() {
    if (!proto_->isVararg()) {
        throw std::runtime_error("CodeGenerator: cannot use '...' outside a vararg function");
    }
    i32 pc = codeABC(OpCode::VARARG, 0, 1, 0);

    CallResultInfo result;
    result.kind = CallResultInfo::Kind::Vararg;
    result.baseReg = -1;
    result.instructionPc = pc;
    return result;
}

void CodeGenerator::setOpenMultiRet(CallResultInfo& info) {
    Instruction inst = proto_->getInstruction(info.instructionPc);
    if (info.kind == CallResultInfo::Kind::Call) {
        SETARG_C(inst, 0);  // C=0 → 返回所有值
    } else if (info.kind == CallResultInfo::Kind::Vararg) {
        SETARG_B(inst, 0);  // B=0 → 复制所有 vararg
    }
    proto_->setInstruction(info.instructionPc, inst);
    info.openMultiRet = true;
}

void CodeGenerator::setWantedResults(CallResultInfo& info, i32 wanted) {
    Instruction inst = proto_->getInstruction(info.instructionPc);
    if (info.kind == CallResultInfo::Kind::Call) {
        SETARG_C(inst, wanted + 1);  // C = wanted+1
    } else if (info.kind == CallResultInfo::Kind::Vararg) {
        SETARG_B(inst, wanted + 1);  // B = wanted+1
    }
    proto_->setInstruction(info.instructionPc, inst);
}











// 辅助函数：获取语句块的最后一行号
static i32 getLastLineOfBlock(const Vec<StmtPtr>& body) {
    if (body.empty()) {
        return 0;
    }
    return body.back()->getLine();
}







i32 CodeGenerator::findUpvalue(const Str& name) {
    return upvalueCtx_.find(name);
}

i32 CodeGenerator::addUpvalue(const Str& name, bool inStack, i32 index) {
    return upvalueCtx_.add(name, inStack, index);
}

i32 CodeGenerator::resolveUpvalue(const Str& name) {
    if (parent_ == nullptr) {
        return -1;
    }

    // 优先在直接父函数的局部变量中查找
    i32 local = parent_->findLocalVar(name);
    if (local >= 0) {
        return addUpvalue(name, true, local);
    }

    // 否则递归在更外层查找，并在父函数中建立中转upvalue
    i32 parentUp = parent_->resolveUpvalue(name);
    if (parentUp >= 0) {
        return addUpvalue(name, false, parentUp);
    }

    return -1;
}

// =====================================================================
// 符号绑定（PR-8 Symbol Binding）
// =====================================================================

SymbolRef CodeGenerator::resolve(const Str& name) {
    SymbolRef result;
    result.name = name;

    i32 reg = findLocalVar(name);
    if (reg >= 0) {
        result.kind = SymbolRef::Kind::Local;
        result.index = reg;
        return result;
    }

    i32 up = resolveUpvalue(name);
    if (up >= 0) {
        result.kind = SymbolRef::Kind::Upvalue;
        result.index = up;
        return result;
    }

    result.kind = SymbolRef::Kind::Global;
    result.index = stringConstant(name);
    return result;
}

ValueResult CodeGenerator::symbolToValue(const SymbolRef& sym) {
    ValueResult result;

    switch (sym.kind) {
        case SymbolRef::Kind::Local:
            result.kind = ValueResult::Kind::Register;
            result.access = ValueResult::AccessKind::Local;
            result.reg = sym.index;
            result.ownsRegister = false;
            break;

        case SymbolRef::Kind::Upvalue:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Upvalue;
            result.aux = sym.index;
            break;

        case SymbolRef::Kind::Global:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Global;
            result.constIndex = sym.index;
            break;

        default:
            break;
    }

    return result;
}

LValueRef CodeGenerator::symbolToLValue(const SymbolRef& sym) {
    LValueRef result;

    switch (sym.kind) {
        case SymbolRef::Kind::Local:
            result.kind = LValueRef::Kind::Local;
            result.slot = sym.index;
            break;

        case SymbolRef::Kind::Upvalue:
            result.kind = LValueRef::Kind::Upvalue;
            result.slot = sym.index;
            break;

        case SymbolRef::Kind::Global:
            result.kind = LValueRef::Kind::Global;
            result.slot = sym.index;
            break;

        default:
            break;
    }

    return result;
}


// =====================================================================
// 语句代码生成（简化版）
// =====================================================================

void CodeGenerator::statement(const Stmt& s) {
    i32 previousLine = currentLine_;
    i32 stmtLine = s.getLine();
    if (stmtLine > 0) {
        currentLine_ = stmtLine;
    }
    std::visit([this](auto&& arg) {
        emitStmt(arg);
    }, s.variant);
    currentLine_ = previousLine;
}

void CodeGenerator::emitStmt(const EmptyStmt&) {
    // 空语句，不生成代码
}

void CodeGenerator::emitStmt(const AssignStmt& s) {
    i32 nvars = static_cast<i32>(s.targets.size());
    i32 nexps = static_cast<i32>(s.values.size());

    // 先处理除最后一个之外的右值表达式（每个表达式固定对应一个左值）
    for (i32 i = 0; i < nexps - 1 && i < nvars; i++) {
        ValueResult val = emitValue(*s.values[i]);
        val = forceSingleValue(val);
        LValueRef target = emitLValue(*s.targets[i]);
        emitStore(target, val);
    }

    // 处理最后一个右值表达式（可能是多返回值表达式）
    if (nexps > 0 && nexps <= nvars) {
        i32 targetIndex = nexps - 1;
        const Expr& lastExpr = *s.values[targetIndex];
        i32 wanted = nvars - targetIndex;

        if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
            CallResultInfo callResult = emitCallExpr(*callExpr);
            setWantedResults(callResult, wanted);
            i32 valueBase = callResult.baseReg;

            for (i32 j = 0; j < wanted; j++) {
                LValueRef target = emitLValue(*s.targets[targetIndex + j]);
                ValueResult tmp;
                tmp.kind = ValueResult::Kind::Register;
                tmp.reg = valueBase + j;
                emitStore(target, tmp);
            }
            return;
        }
        else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
            CallResultInfo callResult = emitVarargExpr();
            Instruction inst = proto_->getInstruction(callResult.instructionPc);
            SETARG_B(inst, wanted + 1);
            proto_->setInstruction(callResult.instructionPc, inst);
            i32 valueBase = GETARG_A(inst);

            for (i32 j = 0; j < wanted; j++) {
                LValueRef target = emitLValue(*s.targets[targetIndex + j]);
                ValueResult tmp;
                tmp.kind = ValueResult::Kind::Register;
                tmp.reg = valueBase + j;
                emitStore(target, tmp);
            }
            return;
        }
        else {
            ValueResult val = emitValue(lastExpr);
            val = forceSingleValue(val);
            LValueRef target = emitLValue(*s.targets[targetIndex]);
            emitStore(target, val);
        }
    }

    // 如果变量多于值，剩余变量赋值为 nil
    for (i32 i = nexps; i < nvars; i++) {
        LValueRef target = emitLValue(*s.targets[i]);
        ValueResult nilVal;
        nilVal.kind = ValueResult::Kind::Immediate;
        nilVal.immediate = ValueResult::ImmediateKind::Nil;
        emitStore(target, nilVal);
    }
}

void CodeGenerator::emitStmt(const LocalStmt& s) {
    i32 nvars = static_cast<i32>(s.names.size());
    i32 nexps = static_cast<i32>(s.values.size());

    // ⭐ 关键修复：保存 locals_.nactvar_ 的初始值（第一个变量的寄存器索引）
    i32 base = locals_.nactvar_;

    //std::fprintf(stderr, "DEBUG LocalStmt: nvars=%d, nexps=%d, base=%d, freereg=%d\n",
    //             nvars, nexps, base, regs_.current());

    // 保存当前寄存器状态，表达式求值将从 base 开始分配寄存器
    i32 savedFreereg = regs_.current();

    // 将临时分配指针对齐到第一个局部变量槽位。
    regs_.setFreeReg(base);

    // 为每个变量分配寄存器。
    for (i32 i = 0; i < nvars; i++) {
        addLocalVar(s.names[i]);
    }

    //std::fprintf(stderr, "DEBUG LocalStmt: after addLocalVar, freereg=%d\n", regs_.current());

    // 重置寄存器基址为 base，确保后续分配从 base 开始
    regs_.setFreeReg(base);

    //std::fprintf(stderr, "DEBUG LocalStmt: reset freereg to %d\n", regs_.current());

    // 生成初始化代码
    bool allVarsInitialized = false;  // 标记是否所有变量都已初始化
    if (nexps > 0) {
        // 处理前 nexps-1 个表达式（每个表达式对应一个变量）
        for (i32 i = 0; i < nexps - 1 && i < nvars; i++) {
            ValueResult val = emitValue(*s.values[i]);
            val = forceSingleValue(val);
            materializeValue(val, base + i);
        }

        // 处理最后一个表达式（可能是多返回值表达式）
        if (nexps <= nvars) {
            const Expr& lastExpr = *s.values[nexps - 1];
            i32 wanted = nvars - (nexps - 1);
            i32 targetReg = base + (nexps - 1);

            // PR-5: 从 AST 直接检测 Call/Vararg，使用原生 CallResultInfo 通道
            if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
                CallResultInfo callResult = emitCallExpr(*callExpr);
                setWantedResults(callResult, wanted);

                i32 callBase = callResult.baseReg;
                if (targetReg != callBase) {
                    // 将返回值从 callBase... 搬到 targetReg...
                    // 注意重叠区：target 在右侧时需逆序拷贝，避免覆盖源值。
                    if (targetReg > callBase) {
                        for (i32 j = wanted - 1; j >= 0; --j) {
                            codeABC(OpCode::MOVE, targetReg + j, callBase + j, 0);
                        }
                    } else {
                        for (i32 j = 0; j < wanted; ++j) {
                            codeABC(OpCode::MOVE, targetReg + j, callBase + j, 0);
                        }
                    }
                }
                allVarsInitialized = true;
            }
            else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
                CallResultInfo callResult = emitVarargExpr();
                Instruction inst = proto_->getInstruction(callResult.instructionPc);
                SETARG_A(inst, targetReg);
                SETARG_B(inst, wanted + 1);
                proto_->setInstruction(callResult.instructionPc, inst);
                allVarsInitialized = true;
            }
            else {
                // 普通表达式
                ValueResult val = emitValue(lastExpr);
                val = forceSingleValue(val);
                materializeValue(val, base + (nexps - 1));
            }
        }
    }

    // 未初始化的变量设为nil
    // ⭐ 关键修复：如果最后一个表达式是多返回值表达式（Vararg/Call），
    // 它已经初始化了所有剩余变量，不需要再生成 LOADNIL
    if (nexps < nvars && !allVarsInitialized) {
        codeABC(OpCode::LOADNIL, base + nexps, base + nvars - 1, 0);  // ⭐ 使用 base 而不是 locals_.nactvar_
    }

    // 恢复寄存器状态
    regs_.restore(savedFreereg);

    adjustLocalVars(nvars);
}

void CodeGenerator::emitStmt(const ReturnStmt& s) {
    // 返回语句 — PR-5: 支持 return f() / return ... 的开放 multret 传播
    i32 nret = static_cast<i32>(s.values.size());
    if (nret == 0) {
        codeABC(OpCode::RETURN, 0, 1, 0);
    } else {
        i32 base = locals_.nactvar_;
        i32 savedFreereg = regs_.current();
        regs_.setFreeReg(base);
        checkStack(nret);

        // 处理前 nret-1 个值（每个固定为单值）
        for (i32 i = 0; i < nret - 1; i++) {
            ValueResult val = emitValue(*s.values[i]);
            val = forceSingleValue(val);
            materializeValue(val, base + i);
        }

        // 确保下一个空闲寄存器指向最后一个值应落的位置。
        regs_.setFreeReg(base + (nret - 1));

        // 处理最后一个值 — 可能是 Call/Vararg 开放多返回
        const Expr& lastExpr = *s.values[nret - 1];
        if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
            // return ..., f() — 保持 multret 传播
            CallResultInfo info = emitCallExpr(*callExpr, base + (nret - 1));
            setOpenMultiRet(info);
            // emitCallExpr 保证 callBase >= base + nret - 1
            // 如果 callBase 恰好等于 base + (nret - 1)，完美对齐
            if (info.baseReg == base + (nret - 1)) {
                codeABC(OpCode::RETURN, base, 0, 0);  // B=0 → 从 base 到栈顶
            } else {
                // callBase 在更高位置（嵌套调用保护发生了搬移），
                // 无法 MOVE multret，退化为单值
                Instruction inst = proto_->getInstruction(info.instructionPc);
                SETARG_C(inst, 2);  // 恢复为单值 C=2
                proto_->setInstruction(info.instructionPc, inst);
                codeABC(OpCode::MOVE, base + (nret - 1), info.baseReg, 0);
                codeABC(OpCode::RETURN, base, nret + 1, 0);
            }
            regs_.restore(savedFreereg);
            return;
        }
        else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
            // return ..., ... — 保持 multret 传播
            CallResultInfo info = emitVarargExpr();
            Instruction inst = proto_->getInstruction(info.instructionPc);
            SETARG_A(inst, base + (nret - 1));
            SETARG_B(inst, 0);  // B=0 → 复制全部 vararg
            proto_->setInstruction(info.instructionPc, inst);
            codeABC(OpCode::RETURN, base, 0, 0);  // B=0 → 返回到栈顶
            regs_.restore(savedFreereg);
            return;
        }
        else {
            // 普通最后一个值
            ValueResult val = emitValue(lastExpr);
            val = forceSingleValue(val);
            materializeValue(val, base + (nret - 1));
        }

        codeABC(OpCode::RETURN, base, nret + 1, 0);
        regs_.restore(savedFreereg);
    }
}

void CodeGenerator::emitStmt(const IfStmt& s) {
    if (s.branches.empty()) {
        return;
    }

    PatchList escapelist;
    PatchList flist;

    // first if branch
    {
        const auto& branch = s.branches[0];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    // elseif branches
    for (size_t i = 1; i < s.branches.size(); i++) {
        escapelist.append(jump());
        patchtohere(flist);

        const auto& branch = s.branches[i];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    // else block
    if (!s.elseBranch.empty()) {
        escapelist.append(jump());
        patchtohere(flist);
        block(s.elseBranch);
    } else {
        escapelist.append(flist);
    }

    patchtohere(escapelist);
}

void CodeGenerator::emitStmt(const WhileStmt& s) {
    i32 whileinit = getLabel();

    // 生成条件表达式，返回假值跳转列表
    CondResult cond = emitCondResult(*s.condition);

    // 进入可break的代码块
    enterBlock(true);

    block(s.body);

    // 生成回跳到循环开始
    patchList(jump(), whileinit);

    // 离开代码块，修补所有break跳转
    leaveBlock();

    // 修补条件为假时的跳转到循环出口
    patchtohere(cond.falseList);
}

void CodeGenerator::emitStmt(const DoStmt& s) {
    // do块
    block(s.body);
}

void CodeGenerator::emitStmt(const CallStmt& s) {
    // PR-5: 直接使用 emitCallExpr，设置 C=1 丢弃所有返回值
    const Expr& callExpr = *s.call;
    if (auto* ce = std::get_if<CallExpr>(&callExpr.variant)) {
        CallResultInfo info = emitCallExpr(*ce);
        setWantedResults(info, 0);  // C=1 → 0 个返回值
        freeReg(info.baseReg);
    } else {
        // 回退：非 CallExpr（理论上 parser 不会生成此情况）
        emitValue(callExpr);
    }

    // 语句级函数调用不会跨语句保留临时寄存器。
    regs_.resetToLocals(locals_.nactvar_);
}

void CodeGenerator::emitStmt(const BreakStmt&) {
    // 查找最近的可 break 代码块
    BlockInfo* bl = blocks_.currentBlock_;
    while (bl && !bl->isbreakable) {
        bl = bl->previous;
    }

    // 如果没有找到可break的代码块，报错
    if (!bl) {
        throw std::runtime_error("no loop to break");
    }

    closeScopeUpvalues(bl->nactvar);

    // 生成跳转指令并添加到break列表
    concatJumpList(bl->breaklist, jump());
}

void CodeGenerator::emitStmt(const RepeatStmt& s) {
    // repeat body until condition
    //
    // 关键语义：body 中声明的局部变量在 until 条件中仍然可见，
    // 因此不能使用 block()（它会在结束时移除局部变量）。
    // Lua 5.1 使用两层 block（loop + scope）处理此问题；
    // 当前实现用单层 loop block + 手动延迟 removeLocalVars 替代。
    // 后续补全 CLOSE 指令支持时再引入 scope block 处理 upvalue 关闭。

    i32 repeat_init = getLabel();

    // 进入可 break 的循环块
    enterBlock(true);  // isbreakable = true

    // 记录循环体前的局部变量数量
    i32 body_nactvar = locals_.nactvar_;

    // 生成循环体（不使用 block()，避免提前移除局部变量）
    for (const auto& stmt : s.body) {
        statement(*stmt);
    }

    // 生成条件表达式（此时 body 局部变量仍然可见）
    CondResult cond = emitCondResult(*s.condition);

    // 条件求值完毕，移除 body 中声明的局部变量
    removeLocalVars(body_nactvar);

    // 条件为假 → 跳回循环开始
    patchList(cond.falseList, repeat_init);

    // 离开循环块，修补所有 break 跳转到当前位置
    leaveBlock();
}

// =====================================================================
// 二元和一元表达式代码生成
// =====================================================================


// =====================================================================


void CodeGenerator::concatJumpList(i32& l1, i32 l2) {
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

// invertJump / jumponcond are no longer used

i32 CodeGenerator::condjump(OpCode op, i32 a, i32 b, i32 c) {
    // 当 TESTSET 的 A 参数为 NO_REG 时转换为 TEST：
    // NO_REG(255) 是无效寄存器索引，直接使用会导致 VM 运行时错误
    if (op == OpCode::TESTSET && a == NO_REG) {
        // 转换为TEST指令：TEST A B C
        // TESTSET的B参数变为TEST的A参数（被测试的寄存器）
        op = OpCode::TEST;
        a = b;
        b = 0;
    }

    codeABC(op, a, b, c);
    i32 jpc = blocks_.jpc_;
    blocks_.jpc_ = NO_JUMP;
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    concatJumpList(j, jpc);  // ⭐ 将jpc链表连接到j后面
    return j;
}

void CodeGenerator::patchtohere(i32 list) {
    pc_ = static_cast<i32>(proto_->getInstructionCount());
    concatJumpList(blocks_.jpc_, list);
}

void CodeGenerator::patchtohere(const PatchList& list) {
    pc_ = static_cast<i32>(proto_->getInstructionCount());
    patchList(list, pc_);
}

void CodeGenerator::syncPC() {
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

PatchList CodeGenerator::collectPatchList(i32 list) {
    PatchList result;

    while (list != NO_JUMP) {
        result.append(list);
        list = getjump(list);
    }

    return result;
}


PatchList CodeGenerator::emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue) {
    OpCode op = OpCode::EQ;
    i32 cond = jumpOnTrue ? 1 : 0;
    bool swapOperands = false;

    switch (e.op) {
        case BinaryExpr::Op::Eq:
            op = OpCode::EQ;
            cond = jumpOnTrue ? 1 : 0;
            break;
        case BinaryExpr::Op::Ne:
            op = OpCode::EQ;
            cond = jumpOnTrue ? 0 : 1;
            break;
        case BinaryExpr::Op::Lt:
            op = OpCode::LT;
            cond = jumpOnTrue ? 1 : 0;
            break;
        case BinaryExpr::Op::Le:
            op = OpCode::LE;
            cond = jumpOnTrue ? 1 : 0;
            break;
        case BinaryExpr::Op::Gt:
            op = OpCode::LT;
            cond = jumpOnTrue ? 1 : 0;
            swapOperands = true;
            break;
        case BinaryExpr::Op::Ge:
            op = OpCode::LE;
            cond = jumpOnTrue ? 1 : 0;
            swapOperands = true;
            break;
        default:
            throw std::runtime_error("emitComparisonJump requires comparison operator");
    }

    // PR-6: native ValueResult pipeline
    ValueResult left = emitValue(*e.left);
    ValueResult right = emitValue(*e.right);

    if (swapOperands) {
        std::swap(left, right);
    }

    i32 o1 = valueToRK(left);
    i32 o2 = valueToRK(right);
    freeReg(o1);
    freeReg(o2);

    codeABC(op, cond, o1, o2);

    PatchList result;
    result.append(jump());
    return result;
}

void CodeGenerator::materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue) {
    if (fallthroughOnTrue) {
        codeABC(OpCode::LOADBOOL, reg, 1, 1);
        i32 falseLabel = getLabel();
        patchList(cond.falseList, falseLabel);
        codeABC(OpCode::LOADBOOL, reg, 0, 0);
        return;
    }

    codeABC(OpCode::LOADBOOL, reg, 0, 1);
    i32 trueLabel = getLabel();
    patchList(cond.trueList, trueLabel);
    codeABC(OpCode::LOADBOOL, reg, 1, 0);
}

// =====================================================================
// 函数定义和调用
// =====================================================================
Proto* CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                     i32 linedefined, i32 lastlinedefined,
                                     Vec<UpvalueCapture>* outUpvalues) {
    // 使用独立子生成器编译子函数，保留父函数上下文以解析upvalue
    CodeGenerator child(pool_);
    child.parent_ = this;

    Proto* newProto = new Proto();
    newProto->setNumParams(static_cast<u8>(params.size()));
    newProto->setVararg(isVararg);
    newProto->setLineDefined(linedefined);
    newProto->setLastLineDefined(lastlinedefined);

    // 继承父Proto的源文件名
    if (proto_ != nullptr) {
        newProto->setSource(proto_->getSource());
    }

    child.proto_ = newProto;
    child.regs_.bind(newProto);
    child.regs_.setFreeReg(0);
    child.locals_.nactvar_ = 0;
    child.locals_.localVars_.clear();
    child.upvalueCtx_.upvalues_.clear();
    child.pc_ = 0;
    child.blocks_.jpc_ = NO_JUMP;
    child.blocks_.currentBlock_ = nullptr;
    child.currentLine_ = linedefined;

    // 添加参数作为局部变量
    for (const Str& param : params) {
        child.addLocalVar(param);
    }
    child.adjustLocalVars(static_cast<i32>(params.size()));

    // 编译函数体
    child.block(body);

    // 添加隐式return（如果函数体没有显式return）
    if (newProto->getInstructionCount() == 0 ||
        GET_OPCODE(newProto->getInstruction(newProto->getInstructionCount() - 1)) != OpCode::RETURN) {
        child.codeABC(OpCode::RETURN, 0, 1, 0);
    }

    // 写入upvalue元信息（数量 + 名称）
    newProto->setNumUpvalues(static_cast<u8>(child.upvalueCtx_.upvalues_.size()));
    for (const UpvalueCapture& uv : child.upvalueCtx_.upvalues_) {
        newProto->addUpvalueName(pool_->intern(uv.name));
    }

    child.attachDebugMetadata();

    // 设置最大栈大小（只增不减）
    if (static_cast<i32>(newProto->getMaxStackSize()) < child.regs_.current()) {
        newProto->setMaxStackSize(static_cast<u8>(child.regs_.current()));
    }

    if (outUpvalues != nullptr) {
        *outUpvalues = child.upvalueCtx_.upvalues_;
    }

    return newProto;
}

void CodeGenerator::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    // Lua 5.1约定：CLOSURE后紧跟nups条伪指令（MOVE或GETUPVAL）
    for (const UpvalueCapture& uv : upvalues) {
        if (uv.inStack) {
            codeABC(OpCode::MOVE, 0, uv.index, 0);
        } else {
            codeABC(OpCode::GETUPVAL, 0, uv.index, 0);
        }
    }
}


void CodeGenerator::emitStmt(const FunctionStmt& s) {
    // 计算函数定义的行号范围
    i32 linedefined = s.line;
    i32 lastlinedefined = getLastLineOfBlock(s.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;  // 空函数体的情况
    }

    // 编译函数体
    Vec<UpvalueCapture> childUpvalues;
    Proto* funcProto = compileFunction(s.params, s.isVararg, s.body, linedefined, lastlinedefined, &childUpvalues);

    // 将Proto添加到当前Proto的子函数列表
    i32 protoIdx = static_cast<i32>(proto_->addProto(funcProto));

    if (s.isLocal) {
        // 局部函数：local function name() end
        // 先添加局部变量，并使用该变量的真实寄存器承载闭包。
        i32 reg = addLocalVar(s.name);

        // 生成CLOSURE指令
        codeABx(OpCode::CLOSURE, reg, protoIdx);
        emitClosureUpvalues(childUpvalues);

        // 激活局部变量
        adjustLocalVars(1);
    } else {
        // 全局/表成员函数：function name() end / function t.a.b:name() end
        i32 savedFreereg = regs_.current();

        if (s.tablePath.empty()) {
            // 生成CLOSURE指令到临时寄存器
            i32 reg = allocReg();
            codeABx(OpCode::CLOSURE, reg, protoIdx);
            emitClosureUpvalues(childUpvalues);

            // 简单全局函数：_G[name] = closure
            i32 k = stringConstant(s.name);
            codeABx(OpCode::SETGLOBAL, reg, k);
        } else {
            // 表成员函数：tablePath.name = closure
            // 例如：
            // - function t.foo() end      -> t["foo"] = closure
            // - function t.a.b:c() end    -> t["a"]["b"]["c"] = closure（参数已含self）
            auto loadNameToReg = [this](const Str& name) -> i32 {
                SymbolRef sym = resolve(name);
                if (sym.kind == SymbolRef::Kind::Local) {
                    return sym.index;
                }
                i32 reg = allocReg();
                ValueResult val = symbolToValue(sym);
                materializeValue(val, reg);
                return reg;
            };

            // 先取到最外层表（tablePath[0]）
            i32 tableReg = loadNameToReg(s.tablePath[0]);

            // 逐层读取中间字段，定位到最终赋值目标表
            for (usize i = 1; i < s.tablePath.size(); i++) {
                i32 nextReg = allocReg();
                i32 k = stringConstant(s.tablePath[i]);
                codeABC(OpCode::GETTABLE, nextReg, tableReg, RKASK(k));
                tableReg = nextReg;
            }

            // 生成CLOSURE（必须先于upvalue伪指令）
            i32 reg = allocReg();
            codeABx(OpCode::CLOSURE, reg, protoIdx);
            emitClosureUpvalues(childUpvalues);

            // 设置最终字段：tableReg[s.name] = closure
            i32 rkKey = RKASK(stringConstant(s.name));
            codeABC(OpCode::SETTABLE, tableReg, rkKey, reg);
        }

        // 释放本语句使用的临时寄存器（包含closure和路径求值临时寄存器）
        regs_.restore(savedFreereg);
        checkStack(0);
    }
}

void CodeGenerator::emitStmt(const ForNumStmt& s) {
    // 数值for循环的字节码模式：
    // R(base) = init
    // R(base+1) = limit
    // R(base+2) = step
    // FORPREP base sBx    ; R(base) -= step, pc += sBx
    // <loop body>
    // FORLOOP base sBx    ; R(base) += step, if R(base) <= limit then pc += sBx

    i32 base = regs_.current();  // 循环变量的基址

    // 计算init, limit, step并存储到R(base), R(base+1), R(base+2)
    ValueResult initVal = emitValue(*s.init);
    initVal = forceSingleValue(initVal);
    valueToNextReg(initVal);  // R(base)

    ValueResult limitVal = emitValue(*s.limit);
    limitVal = forceSingleValue(limitVal);
    valueToNextReg(limitVal);  // R(base+1)

    if (s.step) {
        ValueResult stepVal = emitValue(*s.step);
        stepVal = forceSingleValue(stepVal);
        valueToNextReg(stepVal);  // R(base+2)
    } else {
        // 默认步长为1
        i32 stepReg = allocReg();
        codeABx(OpCode::LOADK, stepReg, numberConstant(1.0));
    }

    // 进入可break的代码块（在添加循环变量之前）
    enterBlock(true);  // isbreakable = true

    // 注册 3 个内部控制变量和可见循环变量为局部变量（与 Lua 5.1 C 一致）。
    // 这确保 locals_.nactvar_ 包含它们，防止后续语句把临时寄存器指针
    // 重置到控制寄存器区域。init/limit/step 已经落在 R(base)..R(base+2)，
    // 这里先回退分配指针，让 addLocalVar 映射到正确槽位。
    regs_.setFreeReg(base);
    addLocalVar("(for index)");   // R(base)
    addLocalVar("(for limit)");   // R(base+1)
    addLocalVar("(for step)");    // R(base+2)
    addLocalVar(s.var);           // R(base+3) — 可见循环变量
    adjustLocalVars(4);

    // 确保临时寄存器从所有保留寄存器之后开始。
    regs_.setFreeReg(base + 4);
    checkStack(0);

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

void CodeGenerator::emitStmt(const ForInStmt& s) {
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

    i32 base = regs_.current();  // 迭代器变量的基址
    i32 nvars = static_cast<i32>(s.vars.size());  // 循环变量数量

    // 计算迭代器表达式（应该返回3个值：func, state, var）
    // 例如：for k, v in pairs(t) do ... end
    // pairs(t) 返回 (next, t, nil)

    if (s.iterators.size() != 1) {
        throw std::runtime_error("CodeGenerator: for-in loop requires exactly 1 iterator expression");
    }

    // 计算迭代器表达式。当前实现要求唯一的迭代表达式直接提供
    // generator/state/control 三元组，因此这里显式消费 Call/Vararg 多返回值通道。
    const Expr& iteratorExpr = *s.iterators[0];
    if (auto* callExpr = std::get_if<CallExpr>(&iteratorExpr.variant)) {
        CallResultInfo info = emitCallExpr(*callExpr, base);
        setWantedResults(info, 3);
        regs_.setFreeReg(base + 3);
        checkStack(0);
    } else if (std::holds_alternative<VarargExpr>(iteratorExpr.variant)) {
        CallResultInfo info = emitVarargExpr();
        Instruction inst = proto_->getInstruction(info.instructionPc);
        SETARG_A(inst, base);
        SETARG_B(inst, 4);  // B=4 -> 3 个结果
        proto_->setInstruction(info.instructionPc, inst);
        regs_.setFreeReg(base + 3);
        checkStack(0);
    } else {
        throw std::runtime_error("CodeGenerator: for-in loop iterator must be a function call or vararg");
    }

    // 进入可break的代码块（在添加循环变量之前）
    enterBlock(true);  // isbreakable = true

    // 注册 3 个内部控制变量为局部变量（与 Lua 5.1 C 一致）。
    // 类似数值 for，先回退分配指针以映射到 R(base)..R(base+2)。
    regs_.setFreeReg(base);
    addLocalVar("(for generator)");  // R(base)
    addLocalVar("(for state)");      // R(base+1)
    addLocalVar("(for control)");    // R(base+2)

    // 添加循环变量作为局部变量（R(base+3), R(base+4), ...）
    for (const Str& var : s.vars) {
        addLocalVar(var);
    }
    adjustLocalVars(3 + nvars);

    // 泛型 for 需要保留：
    // - R(base)   = iterator function
    // - R(base+1) = state
    // - R(base+2) = control variable
    // - R(base+3)... = visible loop variables
    // 循环体临时寄存器从保留区之后开始分配。
    regs_.setFreeReg(base + 3 + nvars);
    checkStack(0);

    // 跳转到TFORLOOP（跳过循环体）
    i32 jmpToTfor = jump();  // 跳到 TFORLOOP，稍后回填

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
    i32 oldnactvar = locals_.nactvar_;

    for (const auto& stmt : stmts) {
        statement(*stmt);
    }

    removeLocalVars(oldnactvar);
}

void CodeGenerator::attachDebugMetadata() {
    if (proto_ == nullptr) {
        return;
    }

    for (const LocalVar& local : locals_.localVars_) {
        i32 endpc = local.endpc >= 0
            ? local.endpc
            : static_cast<i32>(proto_->getInstructionCount());
        proto_->addLocVar(pool_->intern(local.name), local.startpc, endpc, local.reg);
    }
}

// =====================================================================
// LValue 通道（PR-3）
// =====================================================================

LValueRef CodeGenerator::emitLValue(const Expr& e) {
    LValueRef result;

    if (auto* name = std::get_if<NameExpr>(&e.variant)) {
        SymbolRef sym = resolve(name->name);
        result = symbolToLValue(sym);
        return result;
    }

    if (auto* idx = std::get_if<IndexExpr>(&e.variant)) {
        // table[key] — 需要求值表和键
        ValueResult tableVal = emitValue(*idx->table);
        i32 tableReg = valueToAnyReg(tableVal);

        ValueResult keyVal = emitValue(*idx->index);
        i32 rkKey = valueToRK(keyVal);

        result.kind = LValueRef::Kind::Indexed;
        result.tableReg = tableReg;
        result.key = rkKey;
        return result;
    }

    if (auto* mem = std::get_if<MemberExpr>(&e.variant)) {
        // table.member — 需要求值表
        ValueResult tableVal = emitValue(*mem->table);
        i32 tableReg = valueToAnyReg(tableVal);

        i32 constIdx = stringConstant(mem->member);
        ValueResult keyVal;
        keyVal.kind = ValueResult::Kind::Constant;
        keyVal.constIndex = constIdx;
        i32 rkKey = valueToRK(keyVal);

        result.kind = LValueRef::Kind::Indexed;
        result.tableReg = tableReg;
        result.key = rkKey;
        return result;
    }

    throw std::runtime_error("Expression is not a valid lvalue");
}

void CodeGenerator::emitStore(const LValueRef& target, const ValueResult& val) {
    switch (target.kind) {
        case LValueRef::Kind::Local: {
            // 局部变量：直接存储到指定寄存器
            if (val.kind == ValueResult::Kind::Register && val.ownsRegister) {
                freeReg(val.reg);
            }
            materializeValue(val, target.slot);
            return;
        }

        case LValueRef::Kind::Upvalue: {
            // Upvalue：生成 SETUPVAL 指令
            ValueResult v = forceSingleValue(val);
            i32 reg = valueToAnyReg(v);
            codeABC(OpCode::SETUPVAL, reg, target.slot, 0);
            if (v.kind == ValueResult::Kind::Register && v.ownsRegister) {
                freeReg(reg);
            }
            break;
        }

        case LValueRef::Kind::Global: {
            // 全局变量：生成 SETGLOBAL 指令
            ValueResult v = forceSingleValue(val);
            i32 reg = valueToAnyReg(v);
            codeABx(OpCode::SETGLOBAL, reg, target.slot);
            if (v.kind == ValueResult::Kind::Register && v.ownsRegister) {
                freeReg(reg);
            }
            break;
        }

        case LValueRef::Kind::Indexed: {
            // 表索引：生成 SETTABLE 指令
            ValueResult v = forceSingleValue(val);
            i32 rk = valueToRK(v);
            codeABC(OpCode::SETTABLE, target.tableReg, target.key, rk);
            if (v.kind == ValueResult::Kind::Register && v.ownsRegister) {
                freeReg(rk);
            }
            break;
        }

        case LValueRef::Kind::None:
        default:
            throw std::runtime_error("Invalid variable type for assignment");
    }
}

// =====================================================================
// 代码块管理
// =====================================================================

void CodeGenerator::enterBlock(bool isbreakable) {
    blocks_.enterBlock(isbreakable, locals_.nactvar_);
}

void CodeGenerator::closeScopeUpvalues(i32 level) {
    if (locals_.nactvar_ <= level) {
        return;
    }

    if (proto_->getInstructionCount() > 0) {
        Instruction last = proto_->getInstruction(proto_->getInstructionCount() - 1);
        if (GET_OPCODE(last) == OpCode::RETURN) {
            return;
        }
    }

    codeABC(OpCode::CLOSE, level, 0, 0);
}

void CodeGenerator::leaveBlock() {
    if (blocks_.currentBlock_ == nullptr) {
        throw std::runtime_error("No block to leave");
    }

    BlockInfo* bl = blocks_.currentBlock_;
    blocks_.currentBlock_ = bl->previous;

    removeLocalVars(bl->nactvar);
    patchtohere(bl->breaklist);

    delete bl;
}

}  // namespace Lua

