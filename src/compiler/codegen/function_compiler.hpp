#pragma once

/**
 * @file function_compiler.hpp
 * @brief Function prototype compilation boundary for codegen.
 */

#include "common/types.hpp"
#include "compiler/ast.hpp"
#include "compiler/codegen/codegen_context.hpp"

namespace Lua {

class CodeGenerator;
class Proto;

/**
 * Owns the function-level codegen lifecycle.
 *
 * CodeGenerator remains the public facade. FunctionCompiler centralizes child
 * prototype creation, parameter binding, upvalue metadata, closure upvalue
 * instructions, and local debug metadata attachment. Returned Proto pointers
 * are GC-managed non-owning observers.
 */
class FunctionCompiler {
public:
    explicit FunctionCompiler(CodeGenerator& owner) noexcept
        : owner_(owner) {}

    [[nodiscard]] CompiledFunction compile(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                           i32 linedefined = 0, i32 lastlinedefined = 0);

    void emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues);
    void attachDebugMetadata();

private:
    CodeGenerator& owner_;
};

}  // namespace Lua
