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
 * - 单遍代码生成
 * - 简单的寄存器分配策略
 * - 支持基本的代码优化
 * 
 * 参考实现：
 * - lua_c_analysis/src/lcode.h/c - Lua 5.1.5代码生成器
 * - lua_c_analysis/src/lparser.c - 解析器中的代码生成部分
 */

#include "compiler/ast.hpp"
#include "compiler/opcode.hpp"
#include "compiler/codegen_types.hpp"
#include "compiler/codegen_context.hpp"
#include "core/function.hpp"
#include <memory>
#include <unordered_map>

namespace Lua {

// 前向声明
class GCString;
class StringPool;

// LocalVar / UpvalueCapture / BlockInfo 已迁移至 compiler/codegen_context.hpp
// 同一命名空间 Lua 中无需额外 using 声明

/**
 * @brief 代码生成器
 * 
 * 负责将AST转换为字节码。
 */
class CodeGenerator {
public:
    /**
     * @brief 构造函数
     * @param pool 字符串池（用于创建字符串常量）
     */
    explicit CodeGenerator(StringPool* pool);
    
    /**
     * @brief 析构函数
     */
    ~CodeGenerator();
    
    /**
     * @brief 生成字节码
     * @param chunk AST根节点
     * @return 生成的函数原型
     */
    Proto* generate(const Chunk& chunk, StrView sourceName = {});

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
    // 寄存器管理
    // =====================================================================

    i32 allocReg();
    void freeReg(i32 reg);
    void freeRegs(i32 n);
    void checkStack(i32 n);  // 检查并更新maxStackSize

    // =====================================================================
    // 常量表管理
    // =====================================================================

    i32 numberConstant(f64 value);
    i32 stringConstant(const Str& value);
    i32 boolConstant(bool value);
    i32 nilConstant();

    // =====================================================================
    // 局部变量管理
    // =====================================================================

    i32 addLocalVar(const Str& name);
    i32 findLocalVar(const Str& name);
    i32 findUpvalue(const Str& name);
    i32 addUpvalue(const Str& name, bool inStack, i32 index);
    i32 resolveUpvalue(const Str& name);
    void adjustLocalVars(i32 nvars);
    void removeLocalVars(i32 tolevel);
    
    // =====================================================================
    // 条件代码生成（emitCond通道）
    // =====================================================================

    /**
     * @brief 条件代码生成入口
     *
     * 将表达式编译为条件跳转，返回"条件为假时的跳转链表"。
     * 作为 luaK_goiftrue/luaK_goiffalse 的上层包装，统一处理：
     * - 直接比较优化（BinaryExpr 比较运算直接生成 CMP+JMP）
     * - 通用路径（expr → luaK_goiftrue → 返回 false list）
     *
     * @param e 条件表达式
     * @return 条件为假时的跳转链表（patch list）
     */
    i32 emitCond(const Expr& e);
    CondResult emitCondResult(const Expr& e);
    CondResult emitCondResultTrue(const Expr& e);

    // =====================================================================
    // 跳转管理
    // =====================================================================
    
    i32 jump();
    void patchList(i32 list, i32 target);
    void patchList(const PatchList& list, i32 target);
    i32 getLabel();
    
    // =====================================================================
    // 值通道（PR-4 emitValue pipeline）
    // =====================================================================

    /**
     * @brief 将表达式编译为右值，返回 ValueResult
     *
     * 直接从 AST 节点生成值描述，直接通过原生通道。
     * 支持字面量、名字读取、括号、函数表达式、索引/成员/调用等。
     */
    ValueResult emitValue(const Expr& e);

    /**
     * @brief 将 ValueResult 物化到指定寄存器
     */
    void materializeValue(const ValueResult& val, i32 reg);

    /**
     * @brief 将 ValueResult 转为 RK 操作数（常量直接编码或分配寄存器）
     */
    i32 valueToRK(const ValueResult& val);

    /**
     * @brief 确保 ValueResult 在某个寄存器中并返回寄存器编号
     */
    i32 valueToAnyReg(const ValueResult& val);

    /**
     * @brief 将 ValueResult 物化到下一个空闲寄存器
     */
    void valueToNextReg(const ValueResult& val);

    /**
     * @brief 多返回值收敛为单值（括号语义 / exp2Val 语义）
     */
    ValueResult forceSingleValue(const ValueResult& val);

    // =====================================================================
    // 复合表达式原生通道（PR-6 Composite Expressions Cleanup）
    // =====================================================================

    ValueResult emitValueBinary(const BinaryExpr& e);
    ValueResult emitValueUnary(const UnaryExpr& e);
    ValueResult emitValueTable(const TableExpr& e);
    ValueResult emitValueIndex(const IndexExpr& e);
    ValueResult emitValueMember(const MemberExpr& e);

    // =====================================================================
    // 调用/多返回值通道（PR-5 Call/Vararg/MultiRet pipeline）
    // =====================================================================

    /**
     * @brief 编译函数调用表达式，直接返回 CallResultInfo
     *
     * 直接通过原生通道，让 Call 结果有独立结构承载。
     * @param e       调用表达式 AST
     * @param targetBase 可选的强制基址（用于表构造器最后字段对齐）。
     *                   -1 表示不强制。
     * @return 调用结果描述（含 baseReg、instructionPc）
     */
    CallResultInfo emitCallExpr(const CallExpr& e, i32 targetBase = -1);

    /**
     * @brief 编译 vararg 表达式，直接返回 CallResultInfo
     *
     * 直接通过原生通道，让 Vararg 结果有独立结构承载。
     * @return vararg 结果描述（含 instructionPc）
     */
    CallResultInfo emitVarargExpr();

    /**
     * @brief 将 CallResultInfo 设为开放多返回值（C=0 / B=0）
     */
    void setOpenMultiRet(CallResultInfo& info);

    /**
     * @brief 将 CallResultInfo 设为指定数量的返回值
     */
    void setWantedResults(CallResultInfo& info, i32 wanted);

    // =====================================================================
    // LValue 通道（PR-3）
    // =====================================================================

    /**
     * @brief 将表达式解析为左值引用
     *
     * 直接从 AST 节点解析出可写位置（LValueRef）。
     * 支持：NameExpr → Local/Upvalue/Global，IndexExpr/MemberExpr → Indexed
     */
    LValueRef emitLValue(const Expr& e);

    /**
     * @brief 将值存储到左值目标
     *
     * 根据 LValueRef 类型生成对应的存储指令（MOVE/SETGLOBAL/SETUPVAL/SETTABLE）。
     */
    void emitStore(const LValueRef& target, const ValueResult& val);

    // 跳转处理
    void concatJumpList(i32& l1, i32 l2);
    i32 condjump(OpCode op, i32 a, i32 b, i32 c);
    void patchtohere(i32 list);
    void patchtohere(const PatchList& list);
    void flushPendingJumps();
    void syncPC();
    i32 getjump(i32 pc);
    void fixjump(i32 pc, i32 dest);
    PatchList collectPatchList(i32 list);
    PatchList emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue);
    void materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue);

    // =====================================================================
    // 语句代码生成
    // =====================================================================

    void statement(const Stmt& s);
    void emitStmt(const EmptyStmt& s);
    void emitStmt(const AssignStmt& s);
    void emitStmt(const LocalStmt& s);
    void emitStmt(const ReturnStmt& s);
    void emitStmt(const IfStmt& s);
    void emitStmt(const WhileStmt& s);
    void emitStmt(const DoStmt& s);
    void emitStmt(const ForNumStmt& s);
    void emitStmt(const ForInStmt& s);
    void emitStmt(const FunctionStmt& s);
    void emitStmt(const CallStmt& s);
    void emitStmt(const BreakStmt& s);
    void emitStmt(const RepeatStmt& s);
    void block(const Vec<StmtPtr>& stmts);

    // =====================================================================
    // 代码块管理
    // =====================================================================

    void enterBlock(bool isbreakable);
    void leaveBlock();

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
    StringPool* pool_;          // 字符串池
    CodeGenerator* parent_;     // 父函数代码生成器（用于upvalue解析）
    Proto* proto_;              // 当前函数原型
    i32 pc_;                    // 当前指令索引
    i32 currentLine_;           // 当前发射指令所属的源码行号

    // === PR-7: 提取的子系统结构 ===
    RegisterAllocator regs_;        // 寄存器分配/回收/栈检查
    LocalVarScope     locals_;      // 局部变量作用域管理
    BlockManager      blocks_;      // 代码块嵌套/break/跳转链
    UpvalueContext    upvalueCtx_;  // upvalue 捕获/查找
};

}  // namespace Lua

