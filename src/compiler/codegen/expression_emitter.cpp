/**
 * @file expression_emitter.cpp
 * @brief ExpressionEmitter implementation.
 */

#include "compiler/codegen/expression_emitter.hpp"
#include "compiler/codegen/codegen.hpp"

#include <stdexcept>
#include <utility>

namespace Lua {

namespace {

enum class PayloadTruthiness {
    Falsy,
    Truthy,
    Runtime
};

PayloadTruthiness constantTruthiness(const ValueResult& value) {
    return value.visit(ValueResultVisitor{
        [](const ValueResult::Immediate& immediate) noexcept -> PayloadTruthiness {
            switch (immediate.kind) {
                case ValueResult::ImmediateKind::Nil:
                    return PayloadTruthiness::Falsy;
                case ValueResult::ImmediateKind::Boolean:
                    return immediate.boolValue ? PayloadTruthiness::Truthy : PayloadTruthiness::Falsy;
                case ValueResult::ImmediateKind::Number:
                    return PayloadTruthiness::Truthy;
                case ValueResult::ImmediateKind::None:
                default:
                    return PayloadTruthiness::Runtime;
            }
        },
        [](const ValueResult::ConstantRef&) noexcept -> PayloadTruthiness {
            return PayloadTruthiness::Truthy;
        },
        [](const auto&) noexcept -> PayloadTruthiness {
            return PayloadTruthiness::Runtime;
        },
    });
}

Opt<f64> immediateNumber(const ValueResult& value) {
    return value.visit(ValueResultVisitor{
        [](const ValueResult::Immediate& immediate) -> Opt<f64> {
            if (immediate.kind == ValueResult::ImmediateKind::Number) {
                return immediate.numberValue;
            }
            return std::nullopt;
        },
        [](const auto&) -> Opt<f64> {
            return std::nullopt;
        },
    });
}

Opt<i32> ownedRegister(const ValueResult& value) {
    return value.visit(ValueResultVisitor{
        [](const ValueResult::RegisterRef& reg) -> Opt<i32> {
            if (reg.ownsRegister) {
                return reg.reg;
            }
            return std::nullopt;
        },
        [](const auto&) -> Opt<i32> {
            return std::nullopt;
        },
    });
}

// 辅助函数：获取语句块的最后一行号
i32 getLastLineOfBlock(const Vec<StmtPtr>& body) {
    if (body.empty()) {
        return 0;
    }
    return body.back()->getLine();
}

}  // namespace

ExpressionEmitter::ExpressionEmitter(CodeGenerator& owner) noexcept
    : owner_(owner)
    , state_(owner.state_)
    , ops_(owner.ops_)
    , jumps_(owner.jumps_)
    , scopes_(owner.scopes_)
    , binder_(owner.binder_) {}

i32 ExpressionEmitter::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    return ops_.codeABC(op, a, b, c);
}

i32 ExpressionEmitter::codeABx(OpCode op, i32 a, i32 bx) {
    return ops_.codeABx(op, a, bx);
}

i32 ExpressionEmitter::codeAsBx(OpCode op, i32 a, i32 sbx) {
    return ops_.codeAsBx(op, a, sbx);
}

i32 ExpressionEmitter::allocReg() {
    return ops_.allocReg();
}

void ExpressionEmitter::freeReg(i32 reg) {
    ops_.freeReg(reg, scopes_.activeLocalCount());
}

void ExpressionEmitter::checkStack(i32 n) {
    ops_.checkStack(n);
}

i32 ExpressionEmitter::numberConstant(f64 value) {
    return ops_.numberConstant(value);
}

i32 ExpressionEmitter::stringConstant(const Str& value) {
    return ops_.stringConstant(value);
}

SymbolRef ExpressionEmitter::resolve(const Str& name) {
    return binder_.resolve(name);
}

ValueResult ExpressionEmitter::symbolToValue(const SymbolRef& sym) {
    return binder_.symbolToValue(sym);
}

LValueRef ExpressionEmitter::symbolToLValue(const SymbolRef& sym) {
    return binder_.symbolToLValue(sym);
}

i32 ExpressionEmitter::jump() {
    return jumps_.emitJump();
}

void ExpressionEmitter::patchList(const PatchList& list, i32 target) {
    jumps_.patchList(list, target);
}

i32 ExpressionEmitter::getLabel() {
    return jumps_.getLabel();
}

void ExpressionEmitter::patchtohere(const PatchList& list) {
    jumps_.patchToHere(list);
}

void ExpressionEmitter::fixjump(i32 pc, i32 dest) {
    jumps_.fixJump(pc, dest);
}

Proto* ExpressionEmitter::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                          i32 linedefined, i32 lastlinedefined,
                                          Vec<UpvalueCapture>* outUpvalues) {
    return owner_.compileFunction(params, isVararg, body, linedefined, lastlinedefined, outUpvalues);
}

void ExpressionEmitter::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    owner_.emitClosureUpvalues(upvalues);
}

// =====================================================================
// 条件代码生成（emitCond通道）
// =====================================================================

i32 ExpressionEmitter::emitCond(const Expr& e) {
    return emitCondResult(e).falseList.front();
}

CondResult ExpressionEmitter::emitCondResult(const Expr& e) {
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

    switch (constantTruthiness(val)) {
        case PayloadTruthiness::Falsy:
            // 常假值 — 无条件跳转到 falseList
            result.falseList.append(jump());
            break;
        case PayloadTruthiness::Truthy:
            // 常真值（true / number / constant）— 无条件通过，falseList 为空
            break;
        case PayloadTruthiness::Runtime: {
            i32 reg = valueToAnyReg(val);
            // TEST reg 0 0: truthy → skip JMP (fall through = true), falsy → exec JMP (→ falseList)
            codeABC(OpCode::TEST, reg, 0, 0);
            result.falseList.append(jump());
            freeReg(reg);
            break;
        }
    }

    return result;
}

CondResult ExpressionEmitter::emitCondResultTrue(const Expr& e) {
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

    switch (constantTruthiness(val)) {
        case PayloadTruthiness::Truthy:
            // 常真值 — 无条件跳转到 trueList
            result.trueList.append(jump());
            break;
        case PayloadTruthiness::Falsy:
            // nil / false — 无条件通过，trueList 为空
            break;
        case PayloadTruthiness::Runtime: {
            i32 reg = valueToAnyReg(val);
            // TEST reg 0 1: falsy → skip JMP (fall through = false), truthy → exec JMP (→ trueList)
            codeABC(OpCode::TEST, reg, 0, 1);
            result.trueList.append(jump());
            freeReg(reg);
            break;
        }
    }

    return result;
}

// =====================================================================
// 值通道（PR-4 emitValue pipeline）
// =====================================================================

ValueResult ExpressionEmitter::emitValue(const Expr& e) {
    LineGuard line(state_, e.getLine());
    return ExprVisitor<ExpressionEmitter, ValueResult>::visit(e);
}

ValueResult ExpressionEmitter::visitNode(const NilExpr&) {
    return ValueResult::makeNil();
}

ValueResult ExpressionEmitter::visitNode(const BoolExpr& e) {
    return ValueResult::makeBoolean(e.value);
}

ValueResult ExpressionEmitter::visitNode(const NumberExpr& e) {
    return ValueResult::makeNumber(e.value);
}

ValueResult ExpressionEmitter::visitNode(const StringExpr& e) {
    i32 k = stringConstant(e.value);
    return ValueResult::makeConstant(k);
}

ValueResult ExpressionEmitter::visitNode(const VarargExpr&) {
    CallResultInfo info = emitVarargExpr();

    return ValueResult::makeMultiRet(ValueResult::AccessKind::Vararg, info.baseReg, info.instructionPc);
}

ValueResult ExpressionEmitter::visitNode(const NameExpr& e) {
    SymbolRef sym = resolve(e.name);
    return symbolToValue(sym);
}

ValueResult ExpressionEmitter::visitNode(const BinaryExpr& e) {
    return emitValueBinary(e);
}

ValueResult ExpressionEmitter::visitNode(const UnaryExpr& e) {
    return emitValueUnary(e);
}

ValueResult ExpressionEmitter::visitNode(const TableExpr& e) {
    return emitValueTable(e);
}

ValueResult ExpressionEmitter::visitNode(const CallExpr& e) {
    CallResultInfo info = emitCallExpr(e);

    return ValueResult::makeMultiRet(ValueResult::AccessKind::Call, info.baseReg, info.instructionPc);
}

ValueResult ExpressionEmitter::visitNode(const IndexExpr& e) {
    return emitValueIndex(e);
}

ValueResult ExpressionEmitter::visitNode(const MemberExpr& e) {
    return emitValueMember(e);
}

ValueResult ExpressionEmitter::visitNode(const FunctionExpr& e) {
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

    return ValueResult::makeRegister(reg, true);
}

ValueResult ExpressionEmitter::visitNode(const ParenExpr& e) {
    ValueResult inner = emitValue(*e.expression);
    inner = forceSingleValue(inner);
    i32 reg = valueToAnyReg(inner);

    return ValueResult::makeRegister(reg, true);
}

void ExpressionEmitter::materializeValue(const ValueResult& val, i32 reg) {
    val.visit(ValueResultVisitor{
        [](const ValueResult::None&) {},
        [&](const ValueResult::Immediate& immediate) {
            switch (immediate.kind) {
                case ValueResult::ImmediateKind::Nil:
                    codeABC(OpCode::LOADNIL, reg, reg, 0);
                    break;
                case ValueResult::ImmediateKind::Boolean:
                    codeABC(OpCode::LOADBOOL, reg, immediate.boolValue ? 1 : 0, 0);
                    break;
                case ValueResult::ImmediateKind::Number: {
                    i32 k = numberConstant(immediate.numberValue);
                    codeABx(OpCode::LOADK, reg, k);
                    break;
                }
                case ValueResult::ImmediateKind::None:
                default:
                    break;
            }
        },
        [&](const ValueResult::ConstantRef& constant) {
            codeABx(OpCode::LOADK, reg, constant.constIndex);
        },
        [&](const ValueResult::RegisterRef& source) {
            if (source.reg != reg) {
                codeABC(OpCode::MOVE, reg, source.reg, 0);
            }
        },
        [&](const ValueResult::PendingLoad& pending) {
            switch (pending.access) {
                case ValueResult::AccessKind::Global:
                    codeABx(OpCode::GETGLOBAL, reg, pending.constIndex);
                    break;
                case ValueResult::AccessKind::Upvalue:
                    codeABC(OpCode::GETUPVAL, reg, pending.aux, 0);
                    break;
                case ValueResult::AccessKind::Indexed:
                    codeABC(OpCode::GETTABLE, reg, pending.reg, pending.aux);
                    break;
                default:
                    break;
            }
        },
        [&](const ValueResult::Relocatable& relocatable) {
            Instruction inst = ops_.instruction(relocatable.instructionPc);
            SETARG_A(inst, reg);
            ops_.replaceInstruction(relocatable.instructionPc, inst);
        },
        [&](const ValueResult::MultiRet& multi) {
            if (multi.access == ValueResult::AccessKind::Call) {
                // Call: 返回值在 baseReg，不能直接重写 A
                Instruction inst = ops_.instruction(multi.instructionPc);
                i32 callBase = GETARG_A(inst);
                SETARG_C(inst, 2);  // 固定为 1 个返回值
                ops_.replaceInstruction(multi.instructionPc, inst);
                if (callBase != reg) {
                    codeABC(OpCode::MOVE, reg, callBase, 0);
                }
            } else if (multi.access == ValueResult::AccessKind::Vararg) {
                Instruction inst = ops_.instruction(multi.instructionPc);
                SETARG_A(inst, reg);
                SETARG_B(inst, 2);  // 固定为 1 个值
                ops_.replaceInstruction(multi.instructionPc, inst);
            }
        },
        [&](const ValueResult::PendingJump& pending) {
            // 比较表达式物化为布尔值
            i32 trueJump = pending.instructionPc;
            codeABC(OpCode::LOADBOOL, reg, 0, 1);
            i32 trueLabel = getLabel();
            fixjump(trueJump, trueLabel);
            codeABC(OpCode::LOADBOOL, reg, 1, 0);
        },
    });
}

i32 ExpressionEmitter::valueToRK(const ValueResult& val) {
    Opt<i32> encoded = val.visit(ValueResultVisitor{
        [&](const ValueResult::Immediate& immediate) -> Opt<i32> {
            if (immediate.kind != ValueResult::ImmediateKind::Number) {
                return std::nullopt;
            }
            i32 k = numberConstant(immediate.numberValue);
            if (k <= MAXINDEXRK) {
                return RKASK(k);
            }
            return std::nullopt;
        },
        [](const ValueResult::ConstantRef& constant) -> Opt<i32> {
            if (constant.constIndex <= MAXINDEXRK) {
                return RKASK(constant.constIndex);
            }
            return std::nullopt;
        },
        [](const auto&) -> Opt<i32> {
            return std::nullopt;
        },
    });
    if (encoded.has_value()) {
        return *encoded;
    }

    // 否则落到寄存器
    return valueToAnyReg(val);
}

i32 ExpressionEmitter::valueToAnyReg(const ValueResult& val) {
    Opt<i32> existingReg = val.visit(ValueResultVisitor{
        [](const ValueResult::RegisterRef& source) -> Opt<i32> {
            return source.reg;
        },
        [&](const ValueResult::MultiRet& multi) -> Opt<i32> {
            if (multi.access != ValueResult::AccessKind::Call) {
                return std::nullopt;
            }

            // MultiRet(Call): 返回值已在 baseReg，先固定为单值
            Instruction inst = ops_.instruction(multi.instructionPc);
            SETARG_C(inst, 2);
            ops_.replaceInstruction(multi.instructionPc, inst);
            return GETARG_A(inst);
        },
        [](const auto&) -> Opt<i32> {
            return std::nullopt;
        },
    });
    if (existingReg.has_value()) {
        return *existingReg;
    }

    // 否则分配寄存器并物化
    i32 reg = allocReg();
    materializeValue(val, reg);
    return reg;
}

void ExpressionEmitter::valueToNextReg(const ValueResult& val) {
    ValueResult v = forceSingleValue(val);
    bool alreadyAtNextReg = v.visit(ValueResultVisitor{
        [&](const ValueResult::RegisterRef& source) {
            return source.reg == ops_.currentReg() - 1;
        },
        [](const auto&) {
            return false;
        },
    });
    if (alreadyAtNextReg) {
        return;  // 已在下一个位置
    }
    i32 reg = allocReg();
    materializeValue(v, reg);
}

ValueResult ExpressionEmitter::forceSingleValue(const ValueResult& val) {
    return val.visit(ValueResultVisitor{
        [&](const ValueResult::MultiRet& multi) -> ValueResult {
            // 将 CALL/VARARG 固定为单返回值并转为 Relocatable/Register
            if (multi.access == ValueResult::AccessKind::Vararg) {
                Instruction inst = ops_.instruction(multi.instructionPc);
                SETARG_B(inst, 2);  // B=2 → 1 个值
                ops_.replaceInstruction(multi.instructionPc, inst);
                return ValueResult::makeRelocatable(multi.instructionPc);
            }
            if (multi.access == ValueResult::AccessKind::Call) {
                Instruction inst = ops_.instruction(multi.instructionPc);
                SETARG_C(inst, 2);  // C=2 → 1 个返回值
                ops_.replaceInstruction(multi.instructionPc, inst);
                return ValueResult::makeRegister(GETARG_A(inst), false);
            }
            return val;
        },
        [&](const auto&) -> ValueResult {
            return val;
        },
    });
}

// =====================================================================
// 复合表达式原生通道（PR-6 Composite Expressions Cleanup）
// =====================================================================

ValueResult ExpressionEmitter::emitValueBinary(const BinaryExpr& e) {
    BinaryExpr::Op op = e.op;

    // === 比较表达式 → 条件通道 + 物化 ===
    if (op == BinaryExpr::Op::Eq || op == BinaryExpr::Op::Ne ||
        op == BinaryExpr::Op::Lt || op == BinaryExpr::Op::Le ||
        op == BinaryExpr::Op::Gt || op == BinaryExpr::Op::Ge) {
        CondResult cond;
        cond.trueList = emitComparisonJump(e, true);
        i32 resultReg = allocReg();
        materializeCondResult(cond, resultReg, false);
        return ValueResult::makeRegister(resultReg, true);
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

        return ValueResult::makeRegister(resultReg, true);
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
        return ValueResult::makeRelocatable(pc);
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
    return ValueResult::makeRelocatable(pc);
}

ValueResult ExpressionEmitter::emitValueUnary(const UnaryExpr& e) {
    // === Not: 条件通道 + 物化 ===
    if (e.op == UnaryExpr::Op::Not) {
        CondResult cond;
        cond.trueList = emitCondResult(*e.operand).falseList;
        i32 resultReg = allocReg();
        materializeCondResult(cond, resultReg, false);
        return ValueResult::makeRegister(resultReg, true);
    }

    // === Neg: 常量折叠 ===
    if (e.op == UnaryExpr::Op::Neg) {
        ValueResult operand = emitValue(*e.operand);
        Opt<f64> number = immediateNumber(operand);
        if (number.has_value()) {
            return ValueResult::makeNumber(-*number);
        }
        // 非常量: 物化到寄存器后生成 UNM
        i32 opReg = valueToAnyReg(operand);
        freeReg(opReg);
        i32 pc = codeABC(OpCode::UNM, 0, opReg, 0);
        return ValueResult::makeRelocatable(pc);
    }

    // === Len ===
    if (e.op == UnaryExpr::Op::Len) {
        ValueResult operand = emitValue(*e.operand);
        i32 opReg = valueToAnyReg(operand);
        freeReg(opReg);
        i32 pc = codeABC(OpCode::LEN, 0, opReg, 0);
        return ValueResult::makeRelocatable(pc);
    }

    throw std::runtime_error("emitValueUnary: unsupported unary operator");
}

ValueResult ExpressionEmitter::emitValueIndex(const IndexExpr& e) {
    ValueResult table = emitValue(*e.table);
    i32 tableReg = valueToAnyReg(table);

    ValueResult key = emitValue(*e.index);
    i32 rkKey = valueToRK(key);

    return ValueResult::makePendingLoad(ValueResult::AccessKind::Indexed, tableReg, -1, rkKey);
}

ValueResult ExpressionEmitter::emitValueMember(const MemberExpr& e) {
    ValueResult table = emitValue(*e.table);
    i32 tableReg = valueToAnyReg(table);

    i32 k = stringConstant(e.member);
    i32 rkKey;
    if (k <= MAXINDEXRK) {
        rkKey = RKASK(k);
    } else {
        ValueResult keyVal = ValueResult::makeConstant(k);
        rkKey = valueToAnyReg(keyVal);
    }

    return ValueResult::makePendingLoad(ValueResult::AccessKind::Indexed, tableReg, -1, rkKey);
}

ValueResult ExpressionEmitter::emitValueTable(const TableExpr& table) {
    i32 pc = codeABC(OpCode::NEWTABLE, 0, 0, 0);
    i32 tableReg = allocReg();
    ops_.patchArgA(pc, tableReg);

    i32 na = 0, nh = 0, tostore = 0;
    CallResultInfo lastCallResult;
    bool hasLastCallResult = false;

    for (usize i = 0; i < table.fields.size(); i++) {
        const auto& field = table.fields[i];
        bool isLastField = (i == table.fields.size() - 1);

        if (field.key) {
            // 哈希字段: SETTABLE
            {
                RegisterGuard registers(state_);
                ValueResult keyVal = emitValue(*field.key);
                i32 rkKey = valueToRK(keyVal);
                ValueResult valVal = emitValue(*field.value);
                i32 rkVal = valueToRK(valVal);
                codeABC(OpCode::SETTABLE, tableReg, rkKey, rkVal);
            }
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

            i32 targetReg = tableReg + tostore;
            ValueResult val = emitValue(*field.value);
            val = forceSingleValue(val);
            materializeValue(val, targetReg);
            ops_.setFreeRegAndCheck(targetReg + 1);

            if (!hasLastCallResult && tostore == LFIELDS_PER_FLUSH) {
                i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
                codeABC(OpCode::SETLIST, tableReg, LFIELDS_PER_FLUSH, c);
                ops_.setFreeRegAndCheck(tableReg + 1);
                tostore = 0;
            }
        }
    }

    // 刷新剩余数组元素
    if (tostore > 0) {
        if (hasLastCallResult) {
            i32 targetBase = tableReg + tostore;
            if (lastCallResult.kind == CallResultInfo::Kind::Call) {
                Instruction inst = ops_.instruction(lastCallResult.instructionPc);
                i32 callBase = GETARG_A(inst);
                if (callBase != targetBase) {
                    throw std::runtime_error("CALL base mismatch in table multret field");
                }
                setOpenMultiRet(lastCallResult);
            } else {
                ops_.patchArgA(lastCallResult.instructionPc, targetBase);
                setOpenMultiRet(lastCallResult);
            }
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, 0, c);
            ops_.setFreeRegAndCheck(tableReg + 1);
            na--;
        } else {
            i32 c = (na - 1) / LFIELDS_PER_FLUSH + 1;
            codeABC(OpCode::SETLIST, tableReg, tostore, c);
            ops_.setFreeRegAndCheck(tableReg + 1);
        }
    }

    ops_.patchArgsBC(pc, na, nh);

    return ValueResult::makeRegister(tableReg, true);
}

// =====================================================================
// 调用/多返回值通道（PR-5 Call/Vararg/MultiRet pipeline）
// =====================================================================

CallResultInfo ExpressionEmitter::emitCallExpr(const CallExpr& e, i32 targetBase) {
    LineGuard line(state_, e.line);

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
        base = ops_.currentReg();
        ops_.reserveRegsAndCheck(2);

        // SELF base objReg RK(method)
        // R(base+1) = R(objReg); R(base) = R(objReg)[RK(method)]
        codeABC(OpCode::SELF, base, objReg, rkKey);
        hasImplicitSelf = true;
    }
    else {
        ValueResult funcVal = emitValue(*e.func);
        base = valueToAnyReg(funcVal);
    }

    i32 savedFreeReg = ops_.currentReg();

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
    ops_.setFreeRegAndCheck(firstArgReg);
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
                ops_.patchArgsAB(innerInfo.instructionPc, targetReg, 0);
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

        ops_.ensureRegAtLeast(targetReg + 1);
        argIndex++;
    }

    i32 nargs = explicitArgCount + (hasImplicitSelf ? 1 : 0);

    // B: 参数数量+1；如果最后实参是 multret，B=0
    i32 bArg = lastArgIsMultiRet ? 0 : (nargs + 1);
    // C=2: 默认期望 1 个返回值（上层按需修改）
    i32 callPC = codeABC(OpCode::CALL, base, bArg, 2);

    ops_.setFreeRegAndCheck((savedFreeReg > (base + 1)) ? savedFreeReg : (base + 1));

    CallResultInfo result;
    result.kind = CallResultInfo::Kind::Call;
    result.baseReg = base;
    result.instructionPc = callPC;
    return result;
}

CallResultInfo ExpressionEmitter::emitVarargExpr() {
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

void ExpressionEmitter::setOpenMultiRet(CallResultInfo& info) {
    if (info.kind == CallResultInfo::Kind::Call) {
        ops_.patchArgC(info.instructionPc, 0);  // C=0 → 返回所有值
    } else if (info.kind == CallResultInfo::Kind::Vararg) {
        ops_.patchArgB(info.instructionPc, 0);  // B=0 → 复制所有 vararg
    }
    info.openMultiRet = true;
}

void ExpressionEmitter::setWantedResults(CallResultInfo& info, i32 wanted) {
    if (info.kind == CallResultInfo::Kind::Call) {
        ops_.patchArgC(info.instructionPc, wanted + 1);  // C = wanted+1
    } else if (info.kind == CallResultInfo::Kind::Vararg) {
        ops_.patchArgB(info.instructionPc, wanted + 1);  // B = wanted+1
    }
}

PatchList ExpressionEmitter::emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue) {
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

    ValueResult left = emitValue(*e.left);
    i32 o1 = valueToRK(left);

    ValueResult right = emitValue(*e.right);
    i32 o2 = valueToRK(right);

    if (swapOperands) {
        std::swap(o1, o2);
    }

    if (o1 > o2) { freeReg(o1); freeReg(o2); }
    else         { freeReg(o2); freeReg(o1); }

    codeABC(op, cond, o1, o2);

    PatchList result;
    result.append(jump());
    return result;
}

void ExpressionEmitter::materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue) {
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
// LValue 通道（PR-3）
// =====================================================================

LValueRef ExpressionEmitter::emitLValue(const Expr& e) {
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
        ValueResult keyVal = ValueResult::makeConstant(constIdx);
        i32 rkKey = valueToRK(keyVal);

        result.kind = LValueRef::Kind::Indexed;
        result.tableReg = tableReg;
        result.key = rkKey;
        return result;
    }

    throw std::runtime_error("Expression is not a valid lvalue");
}

void ExpressionEmitter::emitStore(const LValueRef& target, const ValueResult& val) {
    switch (target.kind) {
        case LValueRef::Kind::Local: {
            // 局部变量：直接存储到指定寄存器
            if (Opt<i32> reg = ownedRegister(val); reg.has_value()) {
                freeReg(*reg);
            }
            materializeValue(val, target.slot);
            return;
        }

        case LValueRef::Kind::Upvalue: {
            // Upvalue：生成 SETUPVAL 指令
            ValueResult v = forceSingleValue(val);
            i32 reg = valueToAnyReg(v);
            codeABC(OpCode::SETUPVAL, reg, target.slot, 0);
            if (ownedRegister(v).has_value()) {
                freeReg(reg);
            }
            break;
        }

        case LValueRef::Kind::Global: {
            // 全局变量：生成 SETGLOBAL 指令
            ValueResult v = forceSingleValue(val);
            i32 reg = valueToAnyReg(v);
            codeABx(OpCode::SETGLOBAL, reg, target.slot);
            if (ownedRegister(v).has_value()) {
                freeReg(reg);
            }
            break;
        }

        case LValueRef::Kind::Indexed: {
            // 表索引：生成 SETTABLE 指令
            ValueResult v = forceSingleValue(val);
            i32 rk = valueToRK(v);
            codeABC(OpCode::SETTABLE, target.tableReg, target.key, rk);
            if (ownedRegister(v).has_value()) {
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
