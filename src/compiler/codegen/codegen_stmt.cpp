/**
 * @file codegen_stmt.cpp
 * @brief Compatibility forwarding for function-level codegen helpers.
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

Proto* CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                      i32 linedefined, i32 lastlinedefined,
                                      Vec<UpvalueCapture>* outUpvalues) {
    return functions_.compile(params, isVararg, body, linedefined, lastlinedefined, outUpvalues);
}

void CodeGenerator::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    functions_.emitClosureUpvalues(upvalues);
}

void CodeGenerator::attachDebugMetadata() {
    functions_.attachDebugMetadata();
}

}  // namespace Lua
