/**
 * @file codegen_stmt.cpp
 * @brief CodeGenerator statement facade wrappers, function compilation, and block management.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

// =====================================================================
// Statement lowering facade wrappers
// =====================================================================

void CodeGenerator::statement(const Stmt& s) {
    statements_.statement(s);
}

void CodeGenerator::emitStmt(const EmptyStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const AssignStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const LocalStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const ReturnStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const IfStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const WhileStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const DoStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const ForNumStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const ForInStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const FunctionStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const CallStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const BreakStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::emitStmt(const RepeatStmt& s) {
    statements_.emitStmt(s);
}

void CodeGenerator::block(const Vec<StmtPtr>& stmts) {
    statements_.block(stmts);
}

// =====================================================================
// Function definition helpers
// =====================================================================

Proto* CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                      i32 linedefined, i32 lastlinedefined,
                                      Vec<UpvalueCapture>* outUpvalues) {
    CodeGenerator child(state_.services);
    child.state_.parent = this;

    Proto* newProto = new Proto();
    state_.services.gc.registerObject(newProto);
    newProto->setNumParams(static_cast<u8>(params.size()));
    newProto->setVararg(isVararg);
    newProto->setLineDefined(linedefined);
    newProto->setLastLineDefined(lastlinedefined);

    if (state_.proto != nullptr) {
        newProto->setSource(state_.proto->getSource());
    }

    child.state_.resetForProto(*newProto, isVararg);
    child.state_.currentLine = linedefined;

    for (const Str& param : params) {
        child.addLocalVar(param);
    }
    child.adjustLocalVars(static_cast<i32>(params.size()));

    child.block(body);

    child.codeABC(OpCode::RETURN, 0, 1, 0);

    newProto->setNumUpvalues(static_cast<u8>(child.scopes_.upvalues().size()));
    for (const UpvalueCapture& uv : child.scopes_.upvalues()) {
        newProto->addUpvalueName(state_.pool->intern(uv.name));
    }

    child.attachDebugMetadata();

    if (static_cast<i32>(newProto->getMaxStackSize()) < child.state_.regs.current()) {
        newProto->setMaxStackSize(static_cast<u8>(child.state_.regs.current()));
    }

    if (outUpvalues != nullptr) {
        *outUpvalues = child.scopes_.upvalues();
    }

    return newProto;
}

void CodeGenerator::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    for (const UpvalueCapture& uv : upvalues) {
        if (uv.inStack) {
            codeABC(OpCode::MOVE, 0, uv.index, 0);
        } else {
            codeABC(OpCode::GETUPVAL, 0, uv.index, 0);
        }
    }
}

void CodeGenerator::attachDebugMetadata() {
    if (state_.proto == nullptr) {
        return;
    }

    for (const LocalVar& local : scopes_.localVars()) {
        i32 endpc = local.endpc >= 0
            ? local.endpc
            : state_.bytecode.instructionCount();
        state_.bytecode.addLocalDebug(local.name, local.startpc, endpc, local.reg);
    }
}

// =====================================================================
// Block lifecycle facade wrappers
// =====================================================================

void CodeGenerator::enterBlock(bool isbreakable) {
    scopes_.enterBlock(isbreakable);
}

void CodeGenerator::closeScopeUpvalues(i32 level) {
    scopes_.closeScopeUpvalues(level);
}

void CodeGenerator::leaveBlock() {
    scopes_.leaveBlock();
}

}  // namespace Lua
