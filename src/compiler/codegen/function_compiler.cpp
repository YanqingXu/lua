/**
 * @file function_compiler.cpp
 * @brief Function prototype compilation boundary implementation.
 */

#include "compiler/codegen/function_compiler.hpp"
#include "compiler/codegen/codegen.hpp"

namespace Lua {

Proto* FunctionCompiler::compile(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                 i32 linedefined, i32 lastlinedefined,
                                 Vec<UpvalueCapture>* outUpvalues) {
    CodeGenerator child(owner_.state_.services);
    child.state_.parent = &owner_;

    Proto* newProto = new Proto();
    owner_.state_.services.gc.registerObject(newProto);
    newProto->setNumParams(static_cast<u8>(params.size()));
    newProto->setVararg(isVararg);
    newProto->setLineDefined(linedefined);
    newProto->setLastLineDefined(lastlinedefined);

    if (owner_.state_.proto != nullptr) {
        newProto->setSource(owner_.state_.proto->getSource());
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
        newProto->addUpvalueName(owner_.state_.pool->intern(uv.name));
    }

    child.functions_.attachDebugMetadata();

    if (static_cast<i32>(newProto->getMaxStackSize()) < child.state_.registers.current()) {
        newProto->setMaxStackSize(static_cast<u8>(child.state_.registers.current()));
    }

    if (outUpvalues != nullptr) {
        *outUpvalues = child.scopes_.upvalues();
    }

    return newProto;
}

void FunctionCompiler::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    for (const UpvalueCapture& uv : upvalues) {
        if (uv.inStack) {
            owner_.codeABC(OpCode::MOVE, 0, uv.index, 0);
        } else {
            owner_.codeABC(OpCode::GETUPVAL, 0, uv.index, 0);
        }
    }
}

void FunctionCompiler::attachDebugMetadata() {
    if (owner_.state_.proto == nullptr) {
        return;
    }

    for (const LocalVar& local : owner_.scopes_.localVars()) {
        i32 endpc = local.endpc >= 0
            ? local.endpc
            : owner_.state_.bytecode.instructionCount();
        owner_.state_.bytecode.addLocalDebug(local.name, local.startpc, endpc, local.reg);
    }
}

}  // namespace Lua
