#pragma once

/**
 * @file codegen_state.hpp
 * @brief Shared mutable state for CodeGenerator implementation slices.
 */

#include "compiler/codegen/bytecode_builder.hpp"
#include "compiler/codegen/codegen_context.hpp"
#include "core/string_pool.hpp"
#include "runtime/runtime_services.hpp"

#include <memory>

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

    RegisterAllocator registers;
    LocalVarScope localScope;
    BlockManager blockManager;
    UpvalueContext upvalueContext;
    BytecodeBuilder bytecode;
    std::shared_ptr<CompilationBudget> compilationBudget;

    explicit CodegenState(RuntimeServices runtimeServices)
        : services(runtimeServices)
        , pool(&runtimeServices.strings)
        , compilationBudget(std::make_shared<CompilationBudget>(
              runtimeServices.globalState.getCompilationPolicy(),
              &runtimeServices.globalState.getExecutionPolicy())) {}

    CodegenState(RuntimeServices runtimeServices, StringPool* stringPool)
        : services(runtimeServices)
        , pool(stringPool)
        , compilationBudget(std::make_shared<CompilationBudget>(
              runtimeServices.globalState.getCompilationPolicy(),
              &runtimeServices.globalState.getExecutionPolicy())) {}

    void resetForProto(Proto& nextProto, bool isVararg, StrView sourceName = {}) {
        proto = &nextProto;
        bytecode.bind(nextProto, *pool, compilationBudget.get());
        registers.bind(proto);
        proto->setMaxStackSize(2);
        proto->setVararg(isVararg);
        if (!sourceName.empty()) {
            bytecode.setSource(sourceName);
        }

        registers.reset(0);
        localScope.clear();
        blockManager.reset();
        upvalueContext.clear();
        pc = 0;
        currentLine = 0;
    }
};

}  // namespace Lua
