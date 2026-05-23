#pragma once

/**
 * @file statement_emitter.hpp
 * @brief Statement and block lowering boundary.
 */

#include "compiler/ast.hpp"
#include "compiler/ast_visitor.hpp"
#include "compiler/codegen/codegen_state.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/expression_emitter.hpp"
#include "compiler/codegen/jump_patcher.hpp"
#include "compiler/codegen/scope_manager.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

class CodeGenerator;

/**
 * @brief Owns CodeGenerator statement lowering operations.
 *
 * StatementEmitter centralizes statement and block lowering while sharing the
 * facade-owned CodegenState and helper boundaries. It does not own compiler
 * state and keeps CodeGenerator's existing private wrapper API intact.
 */
class StatementEmitter : private StmtVisitor<StatementEmitter, void> {
    friend struct StmtVisitor<StatementEmitter, void>;
    template <typename Visitor, typename Node, typename R>
    friend consteval bool detail::canVisitNode();

public:
    explicit StatementEmitter(CodeGenerator& owner) noexcept;

    void statement(const Stmt& s);
    void block(const Vec<StmtPtr>& stmts);

    void emitStmt(const EmptyStmt& s);
    void emitStmt(const AssignStmt& s);
    void emitStmt(const LocalStmt& s);
    void emitStmt(const ReturnStmt& s);
    void emitStmt(const IfStmt& s);
    void emitStmt(const WhileStmt& s);
    void emitStmt(const DoStmt& s);
    void emitStmt(const ForNumStmt& s);
    void emitStmt(const ForInStmt& s);
    void emitStmt(const FunctionStmt& s);
    void emitStmt(const CallStmt& s);
    void emitStmt(const BreakStmt& s);
    void emitStmt(const RepeatStmt& s);

private:
    void visitNode(const EmptyStmt& s);
    void visitNode(const AssignStmt& s);
    void visitNode(const LocalStmt& s);
    void visitNode(const ReturnStmt& s);
    void visitNode(const IfStmt& s);
    void visitNode(const WhileStmt& s);
    void visitNode(const DoStmt& s);
    void visitNode(const ForNumStmt& s);
    void visitNode(const ForInStmt& s);
    void visitNode(const FunctionStmt& s);
    void visitNode(const CallStmt& s);
    void visitNode(const BreakStmt& s);
    void visitNode(const RepeatStmt& s);

    i32 codeABC(OpCode op, i32 a, i32 b, i32 c);
    i32 codeABx(OpCode op, i32 a, i32 bx);
    i32 codeAsBx(OpCode op, i32 a, i32 sbx);

    i32 allocReg();
    void freeReg(i32 reg);
    void checkStack(i32 n);

    i32 numberConstant(f64 value);
    i32 stringConstant(const Str& value);

    i32 addLocalVar(const Str& name);
    void adjustLocalVars(i32 nvars);
    void removeLocalVars(i32 tolevel);

    SymbolRef resolve(const Str& name);
    ValueResult symbolToValue(const SymbolRef& sym);

    CondResult emitCondResult(const Expr& e);
    ValueResult emitValue(const Expr& e);
    void materializeValue(const ValueResult& val, i32 reg);
    i32 valueToAnyReg(const ValueResult& val);
    void valueToNextReg(const ValueResult& val);
    ValueResult forceSingleValue(const ValueResult& val);

    CallResultInfo emitCallExpr(const CallExpr& e, i32 targetBase = -1);
    CallResultInfo emitVarargExpr();
    void setOpenMultiRet(CallResultInfo& info);
    void setWantedResults(CallResultInfo& info, i32 wanted);

    LValueRef emitLValue(const Expr& e);
    void emitStore(const LValueRef& target, const ValueResult& val);

    i32 jump();
    void patchList(i32 list, i32 target);
    void patchList(const PatchList& list, i32 target);
    i32 getLabel();
    void patchtohere(i32 list);
    void patchtohere(const PatchList& list);
    void fixjump(i32 pc, i32 dest);

    void enterBlock(bool isbreakable);
    void leaveBlock();
    void closeScopeUpvalues(i32 level);

    Proto* compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                           i32 linedefined, i32 lastlinedefined,
                           Vec<UpvalueCapture>* outUpvalues);
    void emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues);

    CodeGenerator& owner_;
    CodegenState& state_;
    JumpPatcher& jumps_;
    ScopeManager& scopes_;
    ExpressionEmitter& expressions_;
};

}  // namespace Lua
