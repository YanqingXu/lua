#pragma once

/**
 * @file expression_emitter.hpp
 * @brief 表达式、条件、调用、可变参数与左值的降级边界
 */

#include "compiler/ast.hpp"
#include "compiler/ast_visitor.hpp"
#include "compiler/codegen/codegen_ops.hpp"
#include "compiler/codegen/codegen_state.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/jump_patcher.hpp"
#include "compiler/codegen/name_binder.hpp"
#include "compiler/codegen/scope_manager.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

class CodeGenerator;

/**
 * @brief 负责代码生成器的表达式降级操作
 *
 * 表达式发射器集中生成值结果、条件结果、调用结果信息与左值引用。它与外观
 * 共享代码生成状态及既有的跳转修补器、作用域管理器辅助对象，但不拥有状态。
 */
class ExpressionEmitter : private ExprVisitor<ExpressionEmitter, ValueResult> {
    friend struct ExprVisitor<ExpressionEmitter, ValueResult>;
    template <typename Visitor, typename Node, typename R>
    friend consteval bool detail::canVisitNode();

public:
    explicit ExpressionEmitter(CodeGenerator& owner) noexcept;

    i32 emitCond(const Expr& e);
    CondResult emitCondResult(const Expr& e);
    CondResult emitCondResultTrue(const Expr& e);

    ValueResult emitValue(const Expr& e);
    void materializeValue(const ValueResult& val, i32 reg);
    i32 valueToRK(const ValueResult& val);
    i32 valueToAnyReg(const ValueResult& val);
    void valueToNextReg(const ValueResult& val);
    ValueResult forceSingleValue(const ValueResult& val);

    CallResultInfo emitCallExpr(const CallExpr& e, i32 targetBase = -1);
    CallResultInfo emitVarargExpr();
    void setOpenMultiRet(CallResultInfo& info);
    void setWantedResults(CallResultInfo& info, i32 wanted);

    LValueRef emitLValue(const Expr& e);
    void emitStore(const LValueRef& target, const ValueResult& val);

    PatchList emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue);
    void materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue);

private:
    ValueResult visitNode(const NilExpr& e);
    ValueResult visitNode(const BoolExpr& e);
    ValueResult visitNode(const NumberExpr& e);
    ValueResult visitNode(const StringExpr& e);
    ValueResult visitNode(const VarargExpr& e);
    ValueResult visitNode(const NameExpr& e);
    ValueResult visitNode(const BinaryExpr& e);
    ValueResult visitNode(const UnaryExpr& e);
    ValueResult visitNode(const TableExpr& e);
    ValueResult visitNode(const CallExpr& e);
    ValueResult visitNode(const IndexExpr& e);
    ValueResult visitNode(const MemberExpr& e);
    ValueResult visitNode(const FunctionExpr& e);
    ValueResult visitNode(const ParenExpr& e);

    ValueResult emitValueBinary(const BinaryExpr& e);
    ValueResult emitValueUnary(const UnaryExpr& e);
    ValueResult emitValueTable(const TableExpr& e);
    ValueResult emitValueIndex(const IndexExpr& e);
    ValueResult emitValueMember(const MemberExpr& e);

    i32 codeABC(OpCode op, i32 a, i32 b, i32 c);
    i32 codeABx(OpCode op, i32 a, i32 bx);
    i32 codeAsBx(OpCode op, i32 a, i32 sbx);

    i32 allocReg();
    void freeReg(i32 reg);
    void checkStack(i32 n);

    i32 numberConstant(f64 value);
    i32 stringConstant(const Str& value);

    SymbolRef resolve(const Str& name);
    ValueResult symbolToValue(const SymbolRef& sym);
    LValueRef symbolToLValue(const SymbolRef& sym);

    i32 jump();
    void patchList(const PatchList& list, i32 target);
    i32 getLabel();
    void patchtohere(const PatchList& list);
    void fixjump(i32 pc, i32 dest);

    CompiledFunction compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                     i32 linedefined, i32 lastlinedefined);
    void emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues);

    CodeGenerator& owner_;
    CodegenState& state_;
    CodegenOps& ops_;
    JumpPatcher& jumps_;
    ScopeManager& scopes_;
    NameBinder& binder_;
};

}  // namespace Lua
