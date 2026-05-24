#pragma once

/**
 * @file codegen.hpp
 * @brief Lua字节码生成器
 * 
 * 将AST转换为Lua字节码的代码生成器。
 * 
 * 核心功能：
 * - 遍历AST并生成字节码指令
 * - 管理寄存器分配
 * - 管理常量表
 * - 处理跳转指令回填
 * - 支持局部变量作用域
 * 
 * 设计原则：
 * - 基于寄存器的虚拟机架构
 * - 基于 AST 的字节码生成
 * - 通过 ValueResult / CondResult / LValueRef 分离右值、条件和左值通道
 * - 通过 RegisterAllocator 管理临时寄存器与 maxStackSize
 */

#include "compiler/ast.hpp"
#include "compiler/opcode.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/codegen_state.hpp"
#include "compiler/codegen/jump_patcher.hpp"
#include "compiler/codegen/name_binder.hpp"
#include "compiler/codegen/scope_manager.hpp"
#include "compiler/codegen/expression_emitter.hpp"
#include "compiler/codegen/statement_emitter.hpp"
#include "common/lua_error.hpp"
#include "core/function.hpp"
#include <expected>
#include <memory>
#include <unordered_map>

namespace Lua {

// 前向声明
class GCString;
class StringPool;

// LocalVar / UpvalueCapture / BlockInfo 已迁移至 compiler/codegen/codegen_context.hpp
// 同一命名空间 Lua 中无需额外 using 声明

/**
 * @brief 代码生成器
 * 
 * 负责将AST转换为字节码。
 */
class CodeGenerator {
    friend class ScopeManager;
    friend class ExpressionEmitter;
    friend class StatementEmitter;

public:
    /**
     * @brief 构造函数
     * @param pool 字符串池（用于创建字符串常量）
     */
    explicit CodeGenerator(StringPool* pool);

    /**
     * @brief 构造函数
     * @param services 显式运行时服务集合
     */
    explicit CodeGenerator(RuntimeServices& services);
    
    /**
     * @brief 析构函数
     */
    ~CodeGenerator();
    
    /**
     * @brief 生成字节码
     * @param chunk AST根节点
     * @return 生成的函数原型
     */
    [[nodiscard]] Proto* generate(const Chunk& chunk, StrView sourceName = {});

    /**
     * @brief 生成字节码，以 expected 形式返回入口错误
     * @param chunk AST根节点
     * @return 生成的函数原型，或 CodegenError
     */
    [[nodiscard]] std::expected<Proto*, CodegenError> tryGenerate(const Chunk& chunk, StrView sourceName = {});

    // =====================================================================
    // 符号绑定（PR-8 Symbol Binding）
    // =====================================================================

    /**
     * @brief 解析名字到 SymbolRef（Local → Upvalue → Global 三阶段查找）
     *
     * 统一所有 NameExpr 的查找逻辑：先查局部变量，再查 upvalue，最后 fallback 为全局。
     */
    SymbolRef resolve(const Str& name);

    /**
     * @brief 将 SymbolRef 转为 ValueResult（读路径）
     */
    ValueResult symbolToValue(const SymbolRef& sym);

    /**
     * @brief 将 SymbolRef 转为 LValueRef（写路径）
     */
    LValueRef symbolToLValue(const SymbolRef& sym);

private:
    // =====================================================================
    // 指令生成
    // =====================================================================
    
    i32 codeABC(OpCode op, i32 a, i32 b, i32 c);
    i32 codeABx(OpCode op, i32 a, i32 bx);
    i32 codeAsBx(OpCode op, i32 a, i32 sbx);

    // =====================================================================
    // 函数编译辅助
    // =====================================================================

    // 编译函数体，返回新的Proto
    Proto* compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                          i32 linedefined = 0, i32 lastlinedefined = 0,
                          Vec<UpvalueCapture>* outUpvalues = nullptr);

    void emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues);
    void attachDebugMetadata();

private:
    Proto* generateUnchecked(const Chunk& chunk, StrView sourceName);
    void discardCurrentProto() noexcept;

    CodegenState state_;
    JumpPatcher jumps_;
    ScopeManager scopes_;
    NameBinder binder_;
    ExpressionEmitter expressions_;
    StatementEmitter statements_;
};

}  // namespace Lua

