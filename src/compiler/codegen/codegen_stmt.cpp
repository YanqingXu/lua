/**
 * @file codegen_stmt.cpp
 * @brief CodeGenerator function compilation and debug metadata helpers.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

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
        child.scopes_.addLocalVar(param);
    }
    child.scopes_.adjustLocalVars(static_cast<i32>(params.size()));

    child.statements_.block(body);

    child.codeABC(OpCode::RETURN, 0, 1, 0);

    newProto->setNumUpvalues(static_cast<u8>(child.scopes_.upvalues().size()));
    for (const UpvalueCapture& uv : child.scopes_.upvalues()) {
        newProto->addUpvalueName(state_.pool->intern(uv.name));
    }

    child.attachDebugMetadata();

    if (static_cast<i32>(newProto->getMaxStackSize()) < child.state_.registers.current()) {
        newProto->setMaxStackSize(static_cast<u8>(child.state_.registers.current()));
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

}  // namespace Lua
