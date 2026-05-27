/**
 * @file statement_emitter.cpp
 * @brief StatementEmitter implementation.
 */

#include "compiler/codegen/statement_emitter.hpp"
#include "compiler/codegen/codegen.hpp"

#include <stdexcept>

namespace Lua {

StatementEmitter::StatementEmitter(CodeGenerator& owner) noexcept
    : owner_(owner)
    , state_(owner.state_)
    , ops_(owner.ops_)
    , jumps_(owner.jumps_)
    , scopes_(owner.scopes_)
    , binder_(owner.binder_)
    , expressions_(owner.expressions_) {}

i32 StatementEmitter::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    return ops_.codeABC(op, a, b, c);
}

i32 StatementEmitter::codeABx(OpCode op, i32 a, i32 bx) {
    return ops_.codeABx(op, a, bx);
}

i32 StatementEmitter::codeAsBx(OpCode op, i32 a, i32 sbx) {
    return ops_.codeAsBx(op, a, sbx);
}

i32 StatementEmitter::allocReg() {
    return ops_.allocReg();
}

void StatementEmitter::freeReg(i32 reg) {
    ops_.freeReg(reg, scopes_.activeLocalCount());
}

void StatementEmitter::checkStack(i32 n) {
    ops_.checkStack(n);
}

i32 StatementEmitter::numberConstant(f64 value) {
    return ops_.numberConstant(value);
}

i32 StatementEmitter::stringConstant(const Str& value) {
    return ops_.stringConstant(value);
}

i32 StatementEmitter::addLocalVar(const Str& name) {
    return scopes_.addLocalVar(name);
}

void StatementEmitter::adjustLocalVars(i32 nvars) {
    scopes_.adjustLocalVars(nvars);
}

void StatementEmitter::removeLocalVars(i32 tolevel) {
    scopes_.removeLocalVars(tolevel);
}

SymbolRef StatementEmitter::resolve(const Str& name) {
    return binder_.resolve(name);
}

ValueResult StatementEmitter::symbolToValue(const SymbolRef& sym) {
    return binder_.symbolToValue(sym);
}

CondResult StatementEmitter::emitCondResult(const Expr& e) {
    return expressions_.emitCondResult(e);
}

ValueResult StatementEmitter::emitValue(const Expr& e) {
    return expressions_.emitValue(e);
}

void StatementEmitter::materializeValue(const ValueResult& val, i32 reg) {
    expressions_.materializeValue(val, reg);
}

i32 StatementEmitter::valueToAnyReg(const ValueResult& val) {
    return expressions_.valueToAnyReg(val);
}

void StatementEmitter::valueToNextReg(const ValueResult& val) {
    expressions_.valueToNextReg(val);
}

ValueResult StatementEmitter::forceSingleValue(const ValueResult& val) {
    return expressions_.forceSingleValue(val);
}

CallResultInfo StatementEmitter::emitCallExpr(const CallExpr& e, i32 targetBase) {
    return expressions_.emitCallExpr(e, targetBase);
}

CallResultInfo StatementEmitter::emitVarargExpr() {
    return expressions_.emitVarargExpr();
}

void StatementEmitter::setOpenMultiRet(CallResultInfo& info) {
    expressions_.setOpenMultiRet(info);
}

void StatementEmitter::setWantedResults(CallResultInfo& info, i32 wanted) {
    expressions_.setWantedResults(info, wanted);
}

LValueRef StatementEmitter::emitLValue(const Expr& e) {
    return expressions_.emitLValue(e);
}

void StatementEmitter::emitStore(const LValueRef& target, const ValueResult& val) {
    expressions_.emitStore(target, val);
}

i32 StatementEmitter::jump() {
    return jumps_.emitJump();
}

void StatementEmitter::patchList(i32 list, i32 target) {
    jumps_.patchList(list, target);
}

void StatementEmitter::patchList(const PatchList& list, i32 target) {
    jumps_.patchList(list, target);
}

i32 StatementEmitter::getLabel() {
    return jumps_.getLabel();
}

void StatementEmitter::patchtohere(i32 list) {
    jumps_.patchToHere(list);
}

void StatementEmitter::patchtohere(const PatchList& list) {
    jumps_.patchToHere(list);
}

void StatementEmitter::fixjump(i32 pc, i32 dest) {
    jumps_.fixJump(pc, dest);
}

void StatementEmitter::enterBlock(bool isbreakable) {
    scopes_.enterBlock(isbreakable);
}

void StatementEmitter::leaveBlock() {
    scopes_.leaveBlock();
}

void StatementEmitter::closeScopeUpvalues(i32 level) {
    scopes_.closeScopeUpvalues(level);
}

Proto* StatementEmitter::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                         i32 linedefined, i32 lastlinedefined,
                                         Vec<UpvalueCapture>* outUpvalues) {
    return owner_.compileFunction(params, isVararg, body, linedefined, lastlinedefined, outUpvalues);
}

void StatementEmitter::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    owner_.emitClosureUpvalues(upvalues);
}

static i32 getLastLineOfBlock(const Vec<StmtPtr>& body) {
    if (body.empty()) {
        return 0;
    }
    return body.back()->getLine();
}

void StatementEmitter::visitNode(const EmptyStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const AssignStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const LocalStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const ReturnStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const IfStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const WhileStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const DoStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const ForNumStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const ForInStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const FunctionStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const CallStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const BreakStmt& s) {
    emitStmt(s);
}

void StatementEmitter::visitNode(const RepeatStmt& s) {
    emitStmt(s);
}

void StatementEmitter::statement(const Stmt& s) {
    LineGuard line(state_, s.getLine());
    StmtVisitor<StatementEmitter, void>::visit(s);
    ops_.resetToLocals(scopes_.activeLocalCount());
}

void StatementEmitter::emitStmt(const EmptyStmt&) {
    // Empty statement: no bytecode.
}

void StatementEmitter::emitStmt(const AssignStmt& s) {
    i32 nvars = static_cast<i32>(s.targets.size());
    i32 nexps = static_cast<i32>(s.values.size());

    auto freezeRegister = [&](i32 reg) -> i32 {
        i32 stableReg = ops_.currentReg();
        ops_.reserveRegsAndCheck(1);
        if (stableReg != reg) {
            codeABC(OpCode::MOVE, stableReg, reg, 0);
        }
        return stableReg;
    };

    auto freezeLValue = [&](const Expr& targetExpr) -> LValueRef {
        LValueRef target = emitLValue(targetExpr);
        if (target.kind == LValueRef::Kind::Indexed) {
            target.tableReg = freezeRegister(target.tableReg);
            if (!ISK(target.key)) {
                target.key = freezeRegister(target.key);
            }
        }
        return target;
    };

    Vec<LValueRef> targets;
    targets.reserve(static_cast<usize>(nvars));
    for (const auto& targetExpr : s.targets) {
        targets.push_back(freezeLValue(*targetExpr));
    }

    i32 valueBase = ops_.currentReg();
    i32 assignedValues = 0;

    auto discardExpression = [&](const Expr& expr) {
        i32 scratchReg = valueBase + nvars;
        ops_.setFreeRegAndCheck(scratchReg);

        if (auto* callExpr = std::get_if<CallExpr>(&expr.variant)) {
            CallResultInfo callResult = emitCallExpr(*callExpr, scratchReg);
            setWantedResults(callResult, 0);
        } else if (std::holds_alternative<VarargExpr>(expr.variant)) {
            CallResultInfo varargResult = emitVarargExpr();
            ops_.patchArgsAB(varargResult.instructionPc, scratchReg, 1);
        } else {
            ValueResult val = emitValue(expr);
            val = forceSingleValue(val);
            materializeValue(val, scratchReg);
        }

        ops_.setFreeRegAndCheck(scratchReg);
    };

    i32 directValues = nvars < nexps ? nvars : nexps;
    for (i32 i = 0; i < directValues; i++) {
        const Expr& expr = *s.values[i];
        bool isLastExpr = (i == nexps - 1);
        i32 targetReg = valueBase + i;

        if (isLastExpr && nexps <= nvars) {
            i32 wanted = nvars - i;
            if (auto* callExpr = std::get_if<CallExpr>(&expr.variant)) {
                CallResultInfo callResult = emitCallExpr(*callExpr, targetReg);
                setWantedResults(callResult, wanted);
                ops_.setFreeRegAndCheck(targetReg + wanted);
                assignedValues = nvars;
                break;
            }
            if (std::holds_alternative<VarargExpr>(expr.variant)) {
                CallResultInfo varargResult = emitVarargExpr();
                ops_.patchArgsAB(varargResult.instructionPc, targetReg, wanted + 1);
                ops_.setFreeRegAndCheck(targetReg + wanted);
                assignedValues = nvars;
                break;
            }
        }

        ops_.setFreeRegAndCheck(targetReg);
        ValueResult val = emitValue(expr);
        val = forceSingleValue(val);
        materializeValue(val, targetReg);
        ops_.setFreeRegAndCheck(targetReg + 1);
        assignedValues = i + 1;
    }

    for (i32 i = directValues; i < nexps; i++) {
        discardExpression(*s.values[i]);
    }

    for (i32 i = 0; i < nvars; i++) {
        if (i < assignedValues) {
            emitStore(targets[i], ValueResult::makeRegister(valueBase + i, false));
        } else {
            emitStore(targets[i], ValueResult::makeNil());
        }
    }
}

void StatementEmitter::emitStmt(const LocalStmt& s) {
    i32 nvars = static_cast<i32>(s.names.size());
    i32 nexps = static_cast<i32>(s.values.size());

    i32 base = state_.localScope.activeVarCount_;
    RegisterGuard registers(state_);

    ops_.setFreeReg(base);

    bool allVarsInitialized = false;
    if (nexps > 0) {
        for (i32 i = 0; i < nexps - 1 && i < nvars; i++) {
            ValueResult val = emitValue(*s.values[i]);
            val = forceSingleValue(val);
            materializeValue(val, base + i);
            ops_.setFreeRegAndCheck(base + i + 1);
        }

        if (nexps <= nvars) {
            const Expr& lastExpr = *s.values[nexps - 1];
            i32 wanted = nvars - (nexps - 1);
            i32 targetReg = base + (nexps - 1);

            if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
                CallResultInfo callResult = emitCallExpr(*callExpr);
                setWantedResults(callResult, wanted);

                i32 callBase = callResult.baseReg;
                if (targetReg != callBase) {
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
                ops_.setFreeRegAndCheck(base + nvars);
                allVarsInitialized = true;
            } else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
                CallResultInfo callResult = emitVarargExpr();
                ops_.patchArgsAB(callResult.instructionPc, targetReg, wanted + 1);
                ops_.setFreeRegAndCheck(base + nvars);
                allVarsInitialized = true;
            } else {
                ValueResult val = emitValue(lastExpr);
                val = forceSingleValue(val);
                materializeValue(val, base + (nexps - 1));
                ops_.setFreeRegAndCheck(base + nexps);
            }
        }
    }

    if (nexps < nvars && !allVarsInitialized) {
        codeABC(OpCode::LOADNIL, base + nexps, base + nvars - 1, 0);
    }

    ops_.setFreeReg(base);
    for (i32 i = 0; i < nvars; i++) {
        addLocalVar(s.names[i]);
    }

    registers.restoreNow();
    adjustLocalVars(nvars);
}

void StatementEmitter::emitStmt(const ReturnStmt& s) {
    i32 nret = static_cast<i32>(s.values.size());
    if (nret == 0) {
        codeABC(OpCode::RETURN, 0, 1, 0);
    } else {
        i32 base = state_.localScope.activeVarCount_;
        RegisterGuard registers(state_);
        ops_.setFreeReg(base);
        checkStack(nret);

        for (i32 i = 0; i < nret - 1; i++) {
            ValueResult val = emitValue(*s.values[i]);
            val = forceSingleValue(val);
            materializeValue(val, base + i);
        }

        ops_.setFreeReg(base + (nret - 1));

        const Expr& lastExpr = *s.values[nret - 1];
        if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
            CallResultInfo info = emitCallExpr(*callExpr, base + (nret - 1));
            setOpenMultiRet(info);
            if (info.baseReg == base + (nret - 1)) {
                if (nret == 1) {
                    Instruction inst = ops_.instruction(info.instructionPc);
                    ops_.patchToABC(info.instructionPc, OpCode::TAILCALL, GETARG_A(inst), GETARG_B(inst), 0);
                }
                codeABC(OpCode::RETURN, base, 0, 0);
            } else {
                ops_.patchArgC(info.instructionPc, 2);
                codeABC(OpCode::MOVE, base + (nret - 1), info.baseReg, 0);
                codeABC(OpCode::RETURN, base, nret + 1, 0);
            }
            return;
        } else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
            CallResultInfo info = emitVarargExpr();
            ops_.patchArgsAB(info.instructionPc, base + (nret - 1), 0);
            codeABC(OpCode::RETURN, base, 0, 0);
            return;
        } else {
            ValueResult val = emitValue(lastExpr);
            val = forceSingleValue(val);
            materializeValue(val, base + (nret - 1));
        }

        codeABC(OpCode::RETURN, base, nret + 1, 0);
    }
}

void StatementEmitter::emitStmt(const IfStmt& s) {
    if (s.branches.empty()) {
        return;
    }

    PatchList escapelist;
    PatchList flist;

    {
        const auto& branch = s.branches[0];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    for (size_t i = 1; i < s.branches.size(); i++) {
        escapelist.append(jump());
        patchtohere(flist);

        const auto& branch = s.branches[i];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    if (!s.elseBranch.empty()) {
        escapelist.append(jump());
        patchtohere(flist);
        block(s.elseBranch);
    } else {
        escapelist.append(flist);
    }

    patchtohere(escapelist);
}

void StatementEmitter::emitStmt(const WhileStmt& s) {
    i32 whileinit = getLabel();

    CondResult cond = emitCondResult(*s.condition);

    enterBlock(true);

    block(s.body);

    patchList(jump(), whileinit);

    leaveBlock();

    patchtohere(cond.falseList);
}

void StatementEmitter::emitStmt(const DoStmt& s) {
    block(s.body);
}

void StatementEmitter::emitStmt(const CallStmt& s) {
    const Expr& callExpr = *s.call;
    if (auto* ce = std::get_if<CallExpr>(&callExpr.variant)) {
        CallResultInfo info = emitCallExpr(*ce);
        setWantedResults(info, 0);
        freeReg(info.baseReg);
    } else {
        emitValue(callExpr);
    }

    ops_.resetToLocals(scopes_.activeLocalCount());
}

void StatementEmitter::emitStmt(const BreakStmt&) {
    BlockInfo* bl = scopes_.findBreakableBlock();

    if (!bl) {
        throw std::runtime_error("no loop to break");
    }

    closeScopeUpvalues(bl->activeVarCount);

    scopes_.appendBreakJump(*bl, jump());
}

void StatementEmitter::emitStmt(const RepeatStmt& s) {
    i32 repeat_init = getLabel();

    enterBlock(true);

    i32 bodyActiveVarCount = scopes_.activeLocalCount();

    for (const auto& stmt : s.body) {
        statement(*stmt);
    }

    CondResult cond = emitCondResult(*s.condition);

    removeLocalVars(bodyActiveVarCount);

    patchList(cond.falseList, repeat_init);

    leaveBlock();
}

void StatementEmitter::emitStmt(const FunctionStmt& s) {
    i32 linedefined = s.line;
    i32 lastlinedefined = getLastLineOfBlock(s.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;
    }

    i32 localReg = -1;
    if (s.isLocal) {
        localReg = addLocalVar(s.name);
    }

    Vec<UpvalueCapture> childUpvalues;
    Proto* funcProto = compileFunction(s.params, s.isVararg, s.body, linedefined, lastlinedefined, &childUpvalues);

    i32 protoIdx = state_.bytecode.addSubProto(funcProto);

    if (s.isLocal) {
        codeABx(OpCode::CLOSURE, localReg, protoIdx);
        emitClosureUpvalues(childUpvalues);

        adjustLocalVars(1);
    } else {
        {
            RegisterGuard registers(state_);

            if (s.tablePath.empty()) {
                i32 reg = allocReg();
                codeABx(OpCode::CLOSURE, reg, protoIdx);
                emitClosureUpvalues(childUpvalues);

                i32 k = stringConstant(s.name);
                codeABx(OpCode::SETGLOBAL, reg, k);
            } else {
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

                i32 tableReg = loadNameToReg(s.tablePath[0]);

                for (usize i = 1; i < s.tablePath.size(); i++) {
                    i32 nextReg = allocReg();
                    i32 k = stringConstant(s.tablePath[i]);
                    codeABC(OpCode::GETTABLE, nextReg, tableReg, RKASK(k));
                    tableReg = nextReg;
                }

                i32 reg = allocReg();
                codeABx(OpCode::CLOSURE, reg, protoIdx);
                emitClosureUpvalues(childUpvalues);

                i32 rkKey = RKASK(stringConstant(s.name));
                codeABC(OpCode::SETTABLE, tableReg, rkKey, reg);
            }
        }
        checkStack(0);
    }
}

void StatementEmitter::emitStmt(const ForNumStmt& s) {
    i32 base = ops_.currentReg();

    ValueResult initVal = emitValue(*s.init);
    initVal = forceSingleValue(initVal);
    valueToNextReg(initVal);

    ValueResult limitVal = emitValue(*s.limit);
    limitVal = forceSingleValue(limitVal);
    valueToNextReg(limitVal);

    if (s.step) {
        ValueResult stepVal = emitValue(*s.step);
        stepVal = forceSingleValue(stepVal);
        valueToNextReg(stepVal);
    } else {
        i32 stepReg = allocReg();
        codeABx(OpCode::LOADK, stepReg, numberConstant(1.0));
    }

    enterBlock(true);

    RegisterFrame loopRegs(ops_, base);
    addLocalVar("(for index)");
    addLocalVar("(for limit)");
    addLocalVar("(for step)");
    addLocalVar(s.var);
    adjustLocalVars(4);

    loopRegs.setTop(4);

    i32 prep = codeAsBx(OpCode::FORPREP, base, 0);

    i32 bodyStart = getLabel();
    block(s.body);

    i32 loop = codeAsBx(OpCode::FORLOOP, base, bodyStart - getLabel() - 1);

    fixjump(prep, loop);

    leaveBlock();
}

void StatementEmitter::emitStmt(const ForInStmt& s) {
    i32 base = ops_.currentReg();
    i32 nvars = static_cast<i32>(s.vars.size());

    if (s.iterators.empty()) {
        throw std::runtime_error("CodeGenerator: for-in loop requires iterator expression");
    }

    RegisterFrame iteratorRegs(ops_, base);
    i32 filled = 0;
    i32 nexps = static_cast<i32>(s.iterators.size());
    for (i32 i = 0; i < nexps; i++) {
        const Expr& iteratorExpr = *s.iterators[i];
        bool isLast = (i == nexps - 1);

        if (filled < 3) {
            i32 targetReg = base + filled;
            i32 wanted = 3 - filled;

            if (isLast) {
                if (auto* callExpr = std::get_if<CallExpr>(&iteratorExpr.variant)) {
                    CallResultInfo info = emitCallExpr(*callExpr, targetReg);
                    setWantedResults(info, wanted);
                    filled = 3;
                    iteratorRegs.setTop(3);
                    break;
                }
                if (std::holds_alternative<VarargExpr>(iteratorExpr.variant)) {
                    CallResultInfo info = emitVarargExpr();
                    ops_.patchArgsAB(info.instructionPc, targetReg, wanted + 1);
                    filled = 3;
                    iteratorRegs.setTop(3);
                    break;
                }
            }

            ValueResult val = emitValue(iteratorExpr);
            val = forceSingleValue(val);
            materializeValue(val, targetReg);
            filled++;
            iteratorRegs.setTop(filled);
        } else {
            ValueResult val = emitValue(iteratorExpr);
            val = forceSingleValue(val);
            i32 discardReg = valueToAnyReg(val);
            freeReg(discardReg);
        }
    }

    while (filled < 3) {
        ValueResult nilVal = ValueResult::makeNil();
        materializeValue(nilVal, base + filled);
        filled++;
    }
    iteratorRegs.setTop(3);

    enterBlock(true);

    iteratorRegs.setTopUnchecked(0);
    addLocalVar("(for generator)");
    addLocalVar("(for state)");
    addLocalVar("(for control)");

    for (const Str& var : s.vars) {
        addLocalVar(var);
    }
    adjustLocalVars(3 + nvars);

    iteratorRegs.setTop(3 + nvars);

    i32 jmpToTfor = jump();

    i32 loopStart = getLabel();
    block(s.body);

    patchtohere(jmpToTfor);

    codeABC(OpCode::TFORLOOP, base, 0, nvars);

    codeAsBx(OpCode::JMP, 0, loopStart - getLabel() - 1);

    leaveBlock();
}

void StatementEmitter::block(const Vec<StmtPtr>& stmts) {
    i32 oldActiveVarCount = scopes_.activeLocalCount();

    for (const auto& stmt : stmts) {
        statement(*stmt);
    }

    removeLocalVars(oldActiveVarCount);
}

}  // namespace Lua
