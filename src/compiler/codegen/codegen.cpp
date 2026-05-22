/**
 * @file codegen.cpp
 * @brief Lua字节码生成器实现
 */

#include "compiler/codegen/codegen.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/value.hpp"
#include "gc/garbage_collector.hpp"
#include <expected>
#include <new>
#include <stdexcept>

namespace Lua {

// =====================================================================
// 构造和析构
// =====================================================================

CodeGenerator::CodeGenerator(StringPool* pool)
    : state_(RuntimeServices::fromSingletons(), pool)
    , jumps_(state_)
    , scopes_(state_, jumps_)
    , expressions_(*this)
    , statements_(*this)
{
    if (pool == nullptr) {
        throw std::invalid_argument("StringPool cannot be null");
    }
}

CodeGenerator::CodeGenerator(RuntimeServices& services)
    : state_(services)
    , jumps_(state_)
    , scopes_(state_, jumps_)
    , expressions_(*this)
    , statements_(*this)
{
}

CodeGenerator::~CodeGenerator() {
    // Proto由GC管理
}

// =====================================================================
// 主生成函数
// =====================================================================

Proto* CodeGenerator::generate(const Chunk& chunk, StrView sourceName) {
    auto generated = tryGenerate(chunk, sourceName);
    if (!generated) {
        throw generated.error();
    }
    return *generated;
}

std::expected<Proto*, CodegenError> CodeGenerator::tryGenerate(const Chunk& chunk, StrView sourceName) {
    try {
        return generateUnchecked(chunk, sourceName);
    } catch (const std::bad_alloc&) {
        discardCurrentProto();
        throw;
    } catch (const CodegenError& error) {
        discardCurrentProto();
        return std::unexpected(error);
    } catch (const LuaError& error) {
        discardCurrentProto();
        return std::unexpected(CodegenError(error.what()));
    } catch (const std::exception& error) {
        discardCurrentProto();
        return std::unexpected(CodegenError(error.what()));
    }
}

Proto* CodeGenerator::generateUnchecked(const Chunk& chunk, StrView sourceName) {
    // 创建新的Proto对象
    state_.proto = new Proto();
    state_.services.gc.registerObject(state_.proto);
    state_.resetForProto(*state_.proto, true, sourceName);

    // 生成语句块
    block(chunk.statements);

    // 保留一个兜底 RETURN，覆盖条件分支 return 后仍可落出的路径。
    codeABC(OpCode::RETURN, 0, 1, 0);  // return (no values)

    attachDebugMetadata();

    return state_.proto;
}

void CodeGenerator::discardCurrentProto() noexcept {
    Proto* failedProto = state_.proto;
    state_.proto = nullptr;
    state_.bytecode = BytecodeBuilder();
    state_.registers.bind(nullptr);

    if (failedProto == nullptr) {
        return;
    }

    state_.services.gc.unregisterObject(failedProto);
    delete failedProto;
}

// =====================================================================
// 指令生成
// =====================================================================

i32 CodeGenerator::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    // 在生成指令前刷新所有待处理跳转
    jumps_.flushPendingJumps();

    return state_.bytecode.emitABC(state_.currentLine, op, a, b, c);
}

i32 CodeGenerator::codeABx(OpCode op, i32 a, i32 bx) {
    // 在生成指令前刷新待处理跳转
    jumps_.flushPendingJumps();

    return state_.bytecode.emitABx(state_.currentLine, op, a, bx);
}

i32 CodeGenerator::codeAsBx(OpCode op, i32 a, i32 sbx) {
    // 在生成指令前刷新待处理跳转
    jumps_.flushPendingJumps();

    return state_.bytecode.emitAsBx(state_.currentLine, op, a, sbx);
}

// =====================================================================
// 寄存器管理
// =====================================================================

i32 CodeGenerator::allocReg() {
    return state_.registers.alloc();
}

void CodeGenerator::freeReg(i32 reg) {
    state_.registers.freeReg(reg, scopes_.activeLocalCount());
}

void CodeGenerator::freeRegs(i32 n) {
    state_.registers.freeRegs(n);
}

void CodeGenerator::checkStack(i32 n) {
    state_.registers.checkStack(n);
}

// =====================================================================
// 常量表管理
// =====================================================================

i32 CodeGenerator::numberConstant(f64 value) {
    return state_.bytecode.addNumberConstant(value);
}

i32 CodeGenerator::stringConstant(const Str& value) {
    return state_.bytecode.addStringConstant(value);
}

i32 CodeGenerator::boolConstant(bool value) {
    return state_.bytecode.addBoolConstant(value);
}

i32 CodeGenerator::nilConstant() {
    return state_.bytecode.addNilConstant();
}

// =====================================================================
// 局部变量管理
// =====================================================================

i32 CodeGenerator::addLocalVar(const Str& name) {
    return scopes_.addLocalVar(name);
}

i32 CodeGenerator::findLocalVar(const Str& name) {
    return scopes_.findLocalVar(name);
}

void CodeGenerator::adjustLocalVars(i32 nvars) {
    scopes_.adjustLocalVars(nvars);
}

void CodeGenerator::removeLocalVars(i32 tolevel) {
    scopes_.removeLocalVars(tolevel);
}

}  // namespace Lua
