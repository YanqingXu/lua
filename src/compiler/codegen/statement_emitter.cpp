/**
 * @file statement_emitter.cpp
 * @brief StatementEmitter implementation.
 */

#include "compiler/codegen/statement_emitter.hpp"
#include "compiler/codegen/codegen.hpp"

#include <stdexcept>
#include <type_traits>

namespace Lua {

namespace {

bool allInitializersAreNilLiterals(const LocalStmt& stmt) {
    if (stmt.values.empty()) {
        return false;
    }

    for (const auto& value : stmt.values) {
        if (!std::holds_alternative<NilExpr>(value->variant)) {
            return false;
        }
    }
    return true;
}

void collectStmtReads(const Stmt& stmt, HashSet<Str>& reads);

void collectStmtListReads(const Vec<StmtPtr>& stmts, HashSet<Str>& reads) {
    for (const auto& stmt : stmts) {
        collectStmtReads(*stmt, reads);
    }
}

bool expressionHasCall(const Expr& e) {
    return std::visit(
        [&](const auto& node) {
            using Node = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<Node, NameExpr> || std::is_same_v<Node, NilExpr> ||
                          std::is_same_v<Node, BoolExpr> || std::is_same_v<Node, NumberExpr> ||
                          std::is_same_v<Node, StringExpr> || std::is_same_v<Node, VarargExpr>) {
                return false;
            } else if constexpr (std::is_same_v<Node, BinaryExpr>) {
                return expressionHasCall(*node.left) || expressionHasCall(*node.right);
            } else if constexpr (std::is_same_v<Node, UnaryExpr>) {
                return expressionHasCall(*node.operand);
            } else if constexpr (std::is_same_v<Node, CallExpr>) {
                if (expressionHasCall(*node.func)) {
                    return true;
                }
                for (const auto& arg : node.args) {
                    if (expressionHasCall(*arg)) {
                        return true;
                    }
                }
                return true;
            } else if constexpr (std::is_same_v<Node, TableExpr>) {
                for (const TableField& field : node.fields) {
                    if (field.key != nullptr && expressionHasCall(*field.key)) {
                        return true;
                    }
                    if (expressionHasCall(*field.value)) {
                        return true;
                    }
                }
                return false;
            } else if constexpr (std::is_same_v<Node, IndexExpr>) {
                return expressionHasCall(*node.table) || expressionHasCall(*node.index);
            } else if constexpr (std::is_same_v<Node, MemberExpr>) {
                return expressionHasCall(*node.table);
            } else if constexpr (std::is_same_v<Node, FunctionExpr>) {
                return false;
            } else if constexpr (std::is_same_v<Node, ParenExpr>) {
                return expressionHasCall(*node.expression);
            } else {
                return false;
            }
        },
        e.variant);
}

bool receiverTableName(const Expr& targetExpr, const Str*& name) {
    if (const auto* idx = std::get_if<IndexExpr>(&targetExpr.variant)) {
        if (const auto* tableName = std::get_if<NameExpr>(&idx->table->variant)) {
            name = &tableName->name;
            return true;
        }
    }
    if (const auto* mem = std::get_if<MemberExpr>(&targetExpr.variant)) {
        if (const auto* tableName = std::get_if<NameExpr>(&mem->table->variant)) {
            name = &tableName->name;
            return true;
        }
    }
    return false;
}

void collectExprReads(const Expr& expr, HashSet<Str>& reads) {
    std::visit([&](const auto& node) {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, NameExpr>) {
            reads.insert(node.name);
        } else if constexpr (std::is_same_v<Node, BinaryExpr>) {
            collectExprReads(*node.left, reads);
            collectExprReads(*node.right, reads);
        } else if constexpr (std::is_same_v<Node, UnaryExpr>) {
            collectExprReads(*node.operand, reads);
        } else if constexpr (std::is_same_v<Node, TableExpr>) {
            for (const TableField& field : node.fields) {
                if (field.key != nullptr) {
                    collectExprReads(*field.key, reads);
                }
                collectExprReads(*field.value, reads);
            }
        } else if constexpr (std::is_same_v<Node, CallExpr>) {
            collectExprReads(*node.func, reads);
            for (const auto& arg : node.args) {
                collectExprReads(*arg, reads);
            }
        } else if constexpr (std::is_same_v<Node, IndexExpr>) {
            collectExprReads(*node.table, reads);
            collectExprReads(*node.index, reads);
        } else if constexpr (std::is_same_v<Node, MemberExpr>) {
            collectExprReads(*node.table, reads);
        } else if constexpr (std::is_same_v<Node, FunctionExpr>) {
            collectStmtListReads(node.body, reads);
        } else if constexpr (std::is_same_v<Node, ParenExpr>) {
            collectExprReads(*node.expression, reads);
        }
    }, expr.variant);
}

void collectAssignTargetReads(const Expr& target, HashSet<Str>& reads) {
    if (std::holds_alternative<NameExpr>(target.variant)) {
        return;
    }
    collectExprReads(target, reads);
}

void collectStmtReads(const Stmt& stmt, HashSet<Str>& reads) {
    std::visit([&](const auto& node) {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, AssignStmt>) {
            for (const auto& target : node.targets) {
                collectAssignTargetReads(*target, reads);
            }
            for (const auto& value : node.values) {
                collectExprReads(*value, reads);
            }
        } else if constexpr (std::is_same_v<Node, LocalStmt>) {
            for (const auto& value : node.values) {
                collectExprReads(*value, reads);
            }
        } else if constexpr (std::is_same_v<Node, CallStmt>) {
            collectExprReads(*node.call, reads);
        } else if constexpr (std::is_same_v<Node, IfStmt>) {
            for (const auto& branch : node.branches) {
                collectExprReads(*branch.condition, reads);
                collectStmtListReads(branch.body, reads);
            }
            collectStmtListReads(node.elseBranch, reads);
        } else if constexpr (std::is_same_v<Node, WhileStmt>) {
            collectExprReads(*node.condition, reads);
            collectStmtListReads(node.body, reads);
        } else if constexpr (std::is_same_v<Node, RepeatStmt>) {
            collectStmtListReads(node.body, reads);
            collectExprReads(*node.condition, reads);
        } else if constexpr (std::is_same_v<Node, ForNumStmt>) {
            collectExprReads(*node.init, reads);
            collectExprReads(*node.limit, reads);
            if (node.step != nullptr) {
                collectExprReads(*node.step, reads);
            }
            collectStmtListReads(node.body, reads);
        } else if constexpr (std::is_same_v<Node, ForInStmt>) {
            for (const auto& iterator : node.iterators) {
                collectExprReads(*iterator, reads);
            }
            collectStmtListReads(node.body, reads);
        } else if constexpr (std::is_same_v<Node, FunctionStmt>) {
            if (!node.isLocal) {
                reads.insert(node.name);
            }
            collectStmtListReads(node.body, reads);
        } else if constexpr (std::is_same_v<Node, ReturnStmt>) {
            for (const auto& value : node.values) {
                collectExprReads(*value, reads);
            }
        } else if constexpr (std::is_same_v<Node, DoStmt>) {
            collectStmtListReads(node.body, reads);
        }
    }, stmt.variant);
}

bool isFutureRead(const HashSet<Str>* reads, const Str& name) {
    return reads != nullptr && reads->find(name) != reads->end();
}

}  // namespace

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

CompiledFunction StatementEmitter::compileFunction(const Vec<Str>& params, bool isVararg,
                                                   const Vec<StmtPtr>& body,
                                                   i32 linedefined, i32 lastlinedefined) {
    return owner_.compileFunction(params, isVararg, body, linedefined, lastlinedefined);
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

    if (nvars == 1 && nexps == 1) {
        const auto* targetName = std::get_if<NameExpr>(&s.targets[0]->variant);
        const auto* valueName = std::get_if<NameExpr>(&s.values[0]->variant);
        if (targetName != nullptr && valueName != nullptr && targetName->name == valueName->name &&
            scopes_.findLocalVar(targetName->name) >= 0) {
            return;
        }

        const bool assignsNil = std::holds_alternative<NilExpr>(s.values[0]->variant);
        if (targetName != nullptr && assignsNil && scopes_.currentBlock() == nullptr) {
            const i32 targetReg = scopes_.findLocalVar(targetName->name);
            const bool valueCanBeObserved = isFutureRead(futureReads_, targetName->name) ||
                                            (targetReg >= 0 && scopes_.isLocalCaptured(targetReg));
            if (targetReg >= 0 && !valueCanBeObserved) {
                return;
            }
        }
    }

    auto tryEmitLocalNameAssignment = [&]() -> bool {
        if (nvars == 0 || nvars != nexps) {
            return false;
        }

        Vec<i32> targetRegs;
        Vec<i32> sourceRegs;
        targetRegs.reserve(static_cast<usize>(nvars));
        sourceRegs.reserve(static_cast<usize>(nvars));

        for (i32 i = 0; i < nvars; ++i) {
            const auto* targetName = std::get_if<NameExpr>(&s.targets[static_cast<usize>(i)]->variant);
            const auto* valueName = std::get_if<NameExpr>(&s.values[static_cast<usize>(i)]->variant);
            if (targetName == nullptr || valueName == nullptr) {
                return false;
            }

            i32 targetReg = scopes_.findLocalVar(targetName->name);
            i32 sourceReg = scopes_.findLocalVar(valueName->name);
            if (targetReg < 0 || sourceReg < 0) {
                return false;
            }

            targetRegs.push_back(targetReg);
            sourceRegs.push_back(sourceReg);
        }

        bool changed = false;
        for (i32 i = 0; i < nvars; ++i) {
            changed = changed || targetRegs[static_cast<usize>(i)] != sourceRegs[static_cast<usize>(i)];
        }
        if (!changed) {
            return true;
        }

        if (nvars == 2 && targetRegs[0] == sourceRegs[1] && targetRegs[1] == sourceRegs[0]) {
            i32 tempReg = ops_.currentReg();
            ops_.reserveRegsAndCheck(1);
            codeABC(OpCode::MOVE, tempReg, sourceRegs[0], 0);
            codeABC(OpCode::MOVE, targetRegs[0], sourceRegs[1], 0);
            codeABC(OpCode::MOVE, targetRegs[1], tempReg, 0);
            ops_.setFreeRegAndCheck(tempReg);
            return true;
        }

        for (i32 i = 0; i < nvars; ++i) {
            for (i32 j = i + 1; j < nvars; ++j) {
                if (targetRegs[static_cast<usize>(i)] == sourceRegs[static_cast<usize>(j)]) {
                    return false;
                }
            }
        }

        for (i32 i = 0; i < nvars; ++i) {
            i32 targetReg = targetRegs[static_cast<usize>(i)];
            i32 sourceReg = sourceRegs[static_cast<usize>(i)];
            if (targetReg != sourceReg) {
                codeABC(OpCode::MOVE, targetReg, sourceReg, 0);
            }
        }
        return true;
    };

    if (tryEmitLocalNameAssignment()) {
        return;
    }

    auto freezeRegister = [&](i32 reg) -> i32 {
        i32 stableReg = ops_.currentReg();
        ops_.reserveRegsAndCheck(1);
        if (stableReg != reg) {
            codeABC(OpCode::MOVE, stableReg, reg, 0);
        }
        return stableReg;
    };

    bool rhsHasCall = false;
    for (const auto& value : s.values) {
        if (expressionHasCall(*value)) {
            rhsHasCall = true;
            break;
        }
    }

    auto freezeLValue = [&](const Expr& targetExpr) -> LValueRef {
        LValueRef target = emitLValue(targetExpr);
        if (target.kind == LValueRef::Kind::Indexed) {
            const Str* receiverName = nullptr;
            const bool simpleReceiver = receiverTableName(targetExpr, receiverName);
            (void)receiverName;
            const bool multiAssignNeedsFrozenTarget = nvars > 1;
            if (multiAssignNeedsFrozenTarget || rhsHasCall || !simpleReceiver) {
                target.tableReg = freezeRegister(target.tableReg);
            }
            if ((multiAssignNeedsFrozenTarget || rhsHasCall) && !ISK(target.key)) {
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
    Vec<ValueResult> valuesForStore;
    valuesForStore.reserve(static_cast<usize>(nvars));

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
    const bool assignmentNeedsFrozenValues = nvars > 1 || nexps > 1;
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
        if (assignmentNeedsFrozenValues) {
            materializeValue(val, targetReg);
            valuesForStore.push_back(ValueResult::makeRegister(targetReg, false));
        } else {
            valuesForStore.push_back(val);
        }
        ops_.setFreeRegAndCheck(targetReg + 1);
        assignedValues = i + 1;
    }

    for (i32 i = directValues; i < nexps; i++) {
        discardExpression(*s.values[i]);
    }

    for (i32 i = 0; i < nvars; i++) {
        if (i < assignedValues) {
            if (static_cast<usize>(i) < valuesForStore.size()) {
                emitStore(targets[i], valuesForStore[static_cast<usize>(i)]);
            } else {
                emitStore(targets[i], ValueResult::makeRegister(valueBase + i, false));
            }
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
    if (futureReads_ != nullptr && nexps == 0 && nvars > 0) {
        allVarsInitialized = true;
        for (const Str& name : s.names) {
            if (isFutureRead(futureReads_, name)) {
                allVarsInitialized = false;
                break;
            }
        }
    }

    if (nexps > 0) {
        if (allInitializersAreNilLiterals(s)) {
            if (nvars > 0) {
                codeABC(OpCode::LOADNIL, base, base + nvars - 1, 0);
            }
            ops_.setFreeRegAndCheck(base + nvars);
            allVarsInitialized = true;
        } else {
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
    }

    if (nexps < nvars && !allVarsInitialized) {
        if (nexps == 0) {
            i32 savedLine = state_.currentLine;
            state_.currentLine = 0;
            codeABC(OpCode::LOADNIL, base, base + nvars - 1, 0);
            state_.currentLine = savedLine;
        } else {
            codeABC(OpCode::LOADNIL, base + nexps, base + nvars - 1, 0);
        }
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
        auto directLocalReturnBase = [&]() -> i32 {
            i32 base = -1;
            for (i32 i = 0; i < nret; i++) {
                const auto* name = std::get_if<NameExpr>(&s.values[static_cast<usize>(i)]->variant);
                if (name == nullptr) {
                    return -1;
                }

                i32 reg = scopes_.findLocalVar(name->name);
                if (reg < 0) {
                    return -1;
                }

                if (i == 0) {
                    base = reg;
                } else if (reg != base + i) {
                    return -1;
                }
            }
            return base;
        };

        if (i32 directBase = directLocalReturnBase(); directBase >= 0) {
            codeABC(OpCode::RETURN, directBase, nret + 1, 0);
            return;
        }

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
        {
            LineGuard endLine(state_, s.endLine);
            escapelist.append(jump());
        }
        patchtohere(flist);

        const auto& branch = s.branches[i];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    if (!s.elseBranch.empty()) {
        {
            LineGuard endLine(state_, s.endLine);
            escapelist.append(jump());
        }
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

    i32 savedLine = state_.currentLine;
    state_.currentLine = 0;
    patchList(jump(), whileinit);
    state_.currentLine = savedLine;

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

    ValueResult cond = emitValue(*s.condition);
    cond = forceSingleValue(cond);
    i32 condReg = valueToAnyReg(cond);

    removeLocalVars(bodyActiveVarCount);

    {
        LineGuard conditionLine(state_, s.condition->getLine());
        codeABC(OpCode::TEST, condReg, 0, 0);
        patchList(jump(), repeat_init);
    }

    leaveBlock();
}

void StatementEmitter::emitStmt(const FunctionStmt& s) {
    i32 linedefined = s.line;
    i32 lastlinedefined = s.endLine > 0 ? s.endLine : getLastLineOfBlock(s.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;
    }

    i32 localReg = -1;
    if (s.isLocal) {
        localReg = addLocalVar(s.name);
    }

    CompiledFunction function = compileFunction(s.params, s.isVararg, s.body, linedefined, lastlinedefined);

    if (s.isLocal) {
        codeABx(OpCode::CLOSURE, localReg, function.protoIndex);
        emitClosureUpvalues(function.upvalues);

        adjustLocalVars(1);
    } else {
        {
            RegisterGuard registers(state_);

            if (s.tablePath.empty()) {
                SymbolRef sym = resolve(s.name);
                LValueRef target = binder_.symbolToLValue(sym);

                i32 reg = allocReg();
                codeABx(OpCode::CLOSURE, reg, function.protoIndex);
                emitClosureUpvalues(function.upvalues);
                emitStore(target, ValueResult::makeRegister(reg, false));
            } else {
                auto stringKeyToRK = [this](const Str& key) -> i32 {
                    i32 constIdx = stringConstant(key);
                    return expressions_.valueToRK(ValueResult::makeConstant(constIdx));
                };

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
                    i32 rkKey = stringKeyToRK(s.tablePath[i]);
                    codeABC(OpCode::GETTABLE, nextReg, tableReg, rkKey);
                    if (!ISK(rkKey)) {
                        freeReg(rkKey);
                    }
                    tableReg = nextReg;
                }

                i32 reg = allocReg();
                codeABx(OpCode::CLOSURE, reg, function.protoIndex);
                emitClosureUpvalues(function.upvalues);

                i32 rkKey = stringKeyToRK(s.name);
                codeABC(OpCode::SETTABLE, tableReg, rkKey, reg);
                if (!ISK(rkKey)) {
                    freeReg(rkKey);
                }
            }
        }
        checkStack(0);
    }
}

void StatementEmitter::emitStmt(const ForNumStmt& s) {
    i32 base = ops_.currentReg();

    ValueResult initVal = emitValue(*s.init);
    initVal = forceSingleValue(initVal);
    materializeValue(initVal, base);
    ops_.setFreeRegAndCheck(base + 1);

    ValueResult limitVal = emitValue(*s.limit);
    limitVal = forceSingleValue(limitVal);
    materializeValue(limitVal, base + 1);
    ops_.setFreeRegAndCheck(base + 2);

    if (s.step) {
        ValueResult stepVal = emitValue(*s.step);
        stepVal = forceSingleValue(stepVal);
        materializeValue(stepVal, base + 2);
    } else {
        codeABx(OpCode::LOADK, base + 2, numberConstant(1.0));
    }
    ops_.setFreeRegAndCheck(base + 3);

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

    if (!s.body.empty()) {
        codeABC(OpCode::CLOSE, base + 3, 0, 0);
    }
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

    if (nvars > 0) {
        codeABC(OpCode::CLOSE, base + 3, 0, 0);
    }

    patchtohere(jmpToTfor);

    {
        LineGuard iteratorLine(state_, s.iterators.front()->getLine());
        codeABC(OpCode::TFORLOOP, base, 0, nvars);
    }

    codeAsBx(OpCode::JMP, 0, loopStart - getLabel() - 1);

    leaveBlock();
}

void StatementEmitter::block(const Vec<StmtPtr>& stmts) {
    i32 oldActiveVarCount = scopes_.activeLocalCount();

    Vec<HashSet<Str>> readsAfter(stmts.size() + 1);
    for (usize i = stmts.size(); i > 0; --i) {
        readsAfter[i - 1] = readsAfter[i];
        collectStmtReads(*stmts[i - 1], readsAfter[i - 1]);
    }

    const HashSet<Str>* previousFutureReads = futureReads_;
    for (usize i = 0; i < stmts.size(); ++i) {
        futureReads_ = &readsAfter[i + 1];
        statement(*stmts[i]);
    }
    futureReads_ = previousFutureReads;

    removeLocalVars(oldActiveVarCount);
}

}  // namespace Lua
