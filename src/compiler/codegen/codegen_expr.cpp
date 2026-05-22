/**
 * @file codegen_expr.cpp
 * @brief CodeGenerator expression, condition, call, and lvalue lowering.
 */

#include "compiler/codegen.hpp"

#include <stdexcept>
#include <utility>

namespace Lua {

// 辅助函数：获取语句块的最后一行号
static i32 getLastLineOfBlock(const Vec<StmtPtr>& body) {
    if (body.empty()) {
        return 0;
    }
    return body.back()->getLine();
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
    i32 previousLine = state_.currentLine;
    i32 exprLine = e.getLine();
    if (exprLine > 0) {
        state_.currentLine = exprLine;
    }

    ValueResult result = ExprVisitor<CodeGenerator, ValueResult>::visit(e);

    state_.currentLine = previousLine;
    return result;
}

ValueResult CodeGenerator::visitNode(const NilExpr&) {
    ValueResult result;
    result.kind = ValueResult::Kind::Immediate;
    result.immediate = ValueResult::ImmediateKind::Nil;
    return result;
}

ValueResult CodeGenerator::visitNode(const BoolExpr& e) {
    ValueResult result;
    result.kind = ValueResult::Kind::Immediate;
    result.immediate = ValueResult::ImmediateKind::Boolean;
    result.boolValue = e.value;
    return result;
}

ValueResult CodeGenerator::visitNode(const NumberExpr& e) {
    ValueResult result;
    result.kind = ValueResult::Kind::Immediate;
    result.immediate = ValueResult::ImmediateKind::Number;
    result.numberValue = e.value;
    return result;
}

ValueResult CodeGenerator::visitNode(const StringExpr& e) {
    ValueResult result;
    i32 k = stringConstant(e.value);
    result.kind = ValueResult::Kind::Constant;
    result.constIndex = k;
    return result;
}

ValueResult CodeGenerator::visitNode(const VarargExpr&) {
    CallResultInfo info = emitVarargExpr();

    ValueResult result;
    result.kind = ValueResult::Kind::MultiRet;
    result.access = ValueResult::AccessKind::Vararg;
    result.instructionPc = info.instructionPc;
    result.isMultiResult = true;
    result.isSingleValue = false;
    return result;
}

ValueResult CodeGenerator::visitNode(const NameExpr& e) {
    SymbolRef sym = resolve(e.name);
    return symbolToValue(sym);
}

ValueResult CodeGenerator::visitNode(const BinaryExpr& e) {
    return emitValueBinary(e);
}

ValueResult CodeGenerator::visitNode(const UnaryExpr& e) {
    return emitValueUnary(e);
}

ValueResult CodeGenerator::visitNode(const TableExpr& e) {
    return emitValueTable(e);
}

ValueResult CodeGenerator::visitNode(const CallExpr& e) {
    CallResultInfo info = emitCallExpr(e);

    ValueResult result;
    result.kind = ValueResult::Kind::MultiRet;
    result.access = ValueResult::AccessKind::Call;
    result.reg = info.baseReg;
    result.instructionPc = info.instructionPc;
    result.isMultiResult = true;
    result.isSingleValue = false;
    return result;
}

ValueResult CodeGenerator::visitNode(const IndexExpr& e) {
    return emitValueIndex(e);
}

ValueResult CodeGenerator::visitNode(const MemberExpr& e) {
    return emitValueMember(e);
}

ValueResult CodeGenerator::visitNode(const FunctionExpr& e) {
    i32 linedefined = e.line;
    i32 lastlinedefined = getLastLineOfBlock(e.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;
    }

    Vec<UpvalueCapture> childUpvalues;
    Proto* funcProto = compileFunction(e.params, e.isVararg,
                                       e.body, linedefined, lastlinedefined,
                                       &childUpvalues);
    i32 protoIdx = state_.bytecode.addSubProto(funcProto);
    i32 reg = allocReg();
    codeABx(OpCode::CLOSURE, reg, protoIdx);
    emitClosureUpvalues(childUpvalues);

    ValueResult result;
    result.kind = ValueResult::Kind::Register;
    result.reg = reg;
    result.ownsRegister = true;
    return result;
}

ValueResult CodeGenerator::visitNode(const ParenExpr& e) {
    ValueResult inner = emitValue(*e.expression);
    inner = forceSingleValue(inner);
    i32 reg = valueToAnyReg(inner);

    ValueResult result;
    result.kind = ValueResult::Kind::Register;
    result.reg = reg;
    result.ownsRegister = true;
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
            Instruction inst = state_.bytecode.instruction(val.instructionPc);
            SETARG_A(inst, reg);
            state_.bytecode.replaceInstruction(val.instructionPc, inst);
            break;
        }
        case ValueResult::Kind::MultiRet: {
            if (val.access == ValueResult::AccessKind::Call) {
                // Call: 返回值在 baseReg，不能直接重写 A
                Instruction inst = state_.bytecode.instruction(val.instructionPc);
                i32 callBase = GETARG_A(inst);
                SETARG_C(inst, 2);  // 固定为 1 个返回值
                state_.bytecode.replaceInstruction(val.instructionPc, inst);
                if (callBase != reg) {
                    codeABC(OpCode::MOVE, reg, callBase, 0);
                }
            } else if (val.access == ValueResult::AccessKind::Vararg) {
                Instruction inst = state_.bytecode.instruction(val.instructionPc);
                SETARG_A(inst, reg);
                SETARG_B(inst, 2);  // 固定为 1 个值
                state_.bytecode.replaceInstruction(val.instructionPc, inst);
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
        Instruction inst = state_.bytecode.instruction(val.instructionPc);
        SETARG_C(inst, 2);
        state_.bytecode.replaceInstruction(val.instructionPc, inst);
        return GETARG_A(inst);
    }
    // 否则分配寄存器并物化
    i32 reg = allocReg();
    materializeValue(val, reg);
    return reg;
}

void CodeGenerator::valueToNextReg(const ValueResult& val) {
    ValueResult v = forceSingleValue(val);
    if (v.kind == ValueResult::Kind::Register && v.reg == state_.regs.current() - 1) {
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
        Instruction inst = state_.bytecode.instruction(val.instructionPc);
        SETARG_B(inst, 2);  // B=2 → 1 个值
        state_.bytecode.replaceInstruction(val.instructionPc, inst);
        ValueResult result;
        result.kind = ValueResult::Kind::Relocatable;
        result.instructionPc = val.instructionPc;
        return result;
    }
    if (val.access == ValueResult::AccessKind::Call) {
        Instruction inst = state_.bytecode.instruction(val.instructionPc);
        SETARG_C(inst, 2);  // C=2 → 1 个返回值
        state_.bytecode.replaceInstruction(val.instructionPc, inst);
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
        Instruction inst = state_.bytecode.instruction(pc);
        SETARG_A(inst, tableReg);
        state_.bytecode.replaceInstruction(pc, inst);
    }

    i32 na = 0, nh = 0, tostore = 0;
    CallResultInfo lastCallResult;
    bool hasLastCallResult = false;

    for (usize i = 0; i < table.fields.size(); i++) {
        const auto& field = table.fields[i];
        bool isLastField = (i == table.fields.size() - 1);

        if (field.key) {
            // 哈希字段: SETTABLE
            i32 savedFreereg = state_.regs.current();
            ValueResult keyVal = emitValue(*field.key);
            i32 rkKey = valueToRK(keyVal);
            ValueResult valVal = emitValue(*field.value);
            i32 rkVal = valueToRK(valVal);
            codeABC(OpCode::SETTABLE, tableReg, rkKey, rkVal);
            state_.regs.restore(savedFreereg);
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
                state_.regs.setFreeReg(tableReg + 1);
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
                Instruction inst = state_.bytecode.instruction(lastCallResult.instructionPc);
                i32 callBase = GETARG_A(inst);
                if (callBase != targetBase) {
                    throw std::runtime_error("CALL base mismatch in table multret field");
                }
                setOpenMultiRet(lastCallResult);
            } else {
                Instruction inst = state_.bytecode.instruction(lastCallResult.instructionPc);
                SETARG_A(inst, targetBase);
                state_.bytecode.replaceInstruction(lastCallResult.instructionPc, inst);
                setOpenMultiRet(lastCallResult);
            }
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, 0, c);
            state_.regs.setFreeReg(tableReg + 1);
            checkStack(0);
            na--;
        } else {
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, tostore, c);
            state_.regs.setFreeReg(tableReg + 1);
            checkStack(0);
        }
    }

    // 回填 NEWTABLE 的 B/C (na, nh)
    {
        Instruction inst = state_.bytecode.instruction(pc);
        SETARG_B(inst, na);
        SETARG_C(inst, nh);
        state_.bytecode.replaceInstruction(pc, inst);
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
    i32 previousLine = state_.currentLine;
    if (e.line > 0) {
        state_.currentLine = e.line;
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
        base = state_.regs.current();
        state_.regs.reserve(2);
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

    i32 savedFreeReg = state_.regs.current();

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
    state_.regs.setFreeReg(firstArgReg);
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
                Instruction inst = state_.bytecode.instruction(innerInfo.instructionPc);
                SETARG_A(inst, targetReg);
                SETARG_B(inst, 0);  // B=0 → 全部 vararg
                state_.bytecode.replaceInstruction(innerInfo.instructionPc, inst);
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

        state_.regs.ensureAtLeast(targetReg + 1);
        argIndex++;
    }

    i32 nargs = explicitArgCount + (hasImplicitSelf ? 1 : 0);

    // B: 参数数量+1；如果最后实参是 multret，B=0
    i32 bArg = lastArgIsMultiRet ? 0 : (nargs + 1);
    // C=2: 默认期望 1 个返回值（上层按需修改）
    i32 callPC = codeABC(OpCode::CALL, base, bArg, 2);

    state_.regs.setFreeReg((savedFreeReg > (base + 1)) ? savedFreeReg : (base + 1));
    checkStack(0);

    state_.currentLine = previousLine;

    CallResultInfo result;
    result.kind = CallResultInfo::Kind::Call;
    result.baseReg = base;
    result.instructionPc = callPC;
    return result;
}

CallResultInfo CodeGenerator::emitVarargExpr() {
    if (!state_.proto->isVararg()) {
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
    Instruction inst = state_.bytecode.instruction(info.instructionPc);
    if (info.kind == CallResultInfo::Kind::Call) {
        SETARG_C(inst, 0);  // C=0 → 返回所有值
    } else if (info.kind == CallResultInfo::Kind::Vararg) {
        SETARG_B(inst, 0);  // B=0 → 复制所有 vararg
    }
    state_.bytecode.replaceInstruction(info.instructionPc, inst);
    info.openMultiRet = true;
}

void CodeGenerator::setWantedResults(CallResultInfo& info, i32 wanted) {
    Instruction inst = state_.bytecode.instruction(info.instructionPc);
    if (info.kind == CallResultInfo::Kind::Call) {
        SETARG_C(inst, wanted + 1);  // C = wanted+1
    } else if (info.kind == CallResultInfo::Kind::Vararg) {
        SETARG_B(inst, wanted + 1);  // B = wanted+1
    }
    state_.bytecode.replaceInstruction(info.instructionPc, inst);
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

}  // namespace Lua
