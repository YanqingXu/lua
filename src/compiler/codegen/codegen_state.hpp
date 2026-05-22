#pragma once

/**
 * @file codegen_state.hpp
 * @brief Shared mutable state for CodeGenerator implementation slices.
 */

#include "compiler/codegen/bytecode_builder.hpp"
#include "compiler/codegen/codegen_context.hpp"
#include "core/string_pool.hpp"
#include "runtime/runtime_services.hpp"

namespace Lua {

class CodeGenerator;
class Proto;

/**
 * @brief Mutable state used by CodeGenerator while lowering one function body.
 *
 * This is the first 8B boundary: implementation slices still use CodeGenerator
 * member functions, but their shared state now lives in one explicit object.
 */
struct CodegenState {
    RuntimeServices services;
    StringPool* pool = nullptr;
    CodeGenerator* parent = nullptr;
    Proto* proto = nullptr;
    i32 pc = 0;
    i32 currentLine = 0;

    RegisterAllocator regs;
    LocalVarScope locals;
    BlockManager blocks;
    UpvalueContext upvalues;
    BytecodeBuilder bytecode;

    explicit CodegenState(RuntimeServices runtimeServices)
        : services(runtimeServices)
        , pool(&runtimeServices.strings) {}

    CodegenState(RuntimeServices runtimeServices, StringPool* stringPool)
        : services(runtimeServices)
        , pool(stringPool) {}

    void resetForProto(Proto& nextProto, bool isVararg, StrView sourceName = {}) {
        proto = &nextProto;
        bytecode.bind(nextProto, *pool);
        regs.bind(proto);
        proto->setMaxStackSize(2);
        proto->setVararg(isVararg);
        if (!sourceName.empty()) {
            bytecode.setSource(sourceName);
        }

        regs.reset(0);
        locals.clear();
        blocks.reset();
        upvalues.clear();
        pc = 0;
        currentLine = 0;
    }
};

}  // namespace Lua
