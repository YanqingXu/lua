/**
 * @file codegen_stmt.cpp
 * @brief 函数级代码生成辅助函数的兼容转发层
 */

#include "compiler/codegen/codegen.hpp"

namespace Lua {

CompiledFunction CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                                i32 linedefined, i32 lastlinedefined) {
    return functions_.compile(params, isVararg, body, linedefined, lastlinedefined);
}

void CodeGenerator::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    functions_.emitClosureUpvalues(upvalues);
}

void CodeGenerator::attachDebugMetadata() {
    functions_.attachDebugMetadata();
}

}  // namespace Lua
