#pragma once

/**
 * @file codegen_state.hpp
 * @brief 代码生成器各实现分片共享的可变状态
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
 * @brief 代码生成器降级单个函数体时使用的可变状态
 *
 * 这是第一个 8B 边界：实现分片仍使用代码生成器成员函数，但共享状态现已集中存放在
 * 一个显式对象中。
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
