/**
 * @file codegen_expr.cpp
 * @brief CodeGenerator expression lowering facade wrappers.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

i32 CodeGenerator::emitCond(const Expr& e) {
    return expressions_.emitCond(e);
}

CondResult CodeGenerator::emitCondResult(const Expr& e) {
    return expressions_.emitCondResult(e);
}

CondResult CodeGenerator::emitCondResultTrue(const Expr& e) {
    return expressions_.emitCondResultTrue(e);
}

ValueResult CodeGenerator::emitValue(const Expr& e) {
    return expressions_.emitValue(e);
}

void CodeGenerator::materializeValue(const ValueResult& val, i32 reg) {
    expressions_.materializeValue(val, reg);
}

i32 CodeGenerator::valueToRK(const ValueResult& val) {
    return expressions_.valueToRK(val);
}

i32 CodeGenerator::valueToAnyReg(const ValueResult& val) {
    return expressions_.valueToAnyReg(val);
}

void CodeGenerator::valueToNextReg(const ValueResult& val) {
    expressions_.valueToNextReg(val);
}

ValueResult CodeGenerator::forceSingleValue(const ValueResult& val) {
    return expressions_.forceSingleValue(val);
}

CallResultInfo CodeGenerator::emitCallExpr(const CallExpr& e, i32 targetBase) {
    return expressions_.emitCallExpr(e, targetBase);
}

CallResultInfo CodeGenerator::emitVarargExpr() {
    return expressions_.emitVarargExpr();
}

void CodeGenerator::setOpenMultiRet(CallResultInfo& info) {
    expressions_.setOpenMultiRet(info);
}

void CodeGenerator::setWantedResults(CallResultInfo& info, i32 wanted) {
    expressions_.setWantedResults(info, wanted);
}

LValueRef CodeGenerator::emitLValue(const Expr& e) {
    return expressions_.emitLValue(e);
}

void CodeGenerator::emitStore(const LValueRef& target, const ValueResult& val) {
    expressions_.emitStore(target, val);
}

}  // namespace Lua
