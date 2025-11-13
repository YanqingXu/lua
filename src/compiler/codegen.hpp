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

#include "common/types.hpp"
#include "compiler/ast.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include <memory>
#include <unordered_map>

namespace Lua {

// 前向声明
class GCString;
class StringPool;

/**
 * @brief 跳转补丁列表结束标记
 */
constexpr i32 NO_JUMP = -1;

/**
 * @brief 表达式描述符类型
 */
enum class ExprKind {
    Void,       // 无值
    Nil,        // nil常量
    True,       // true常量
    False,      // false常量
    Const,      // 常量表中的常量
    Number,     // 数字常量（可能未加入常量表）
    NonRelocatable,  // 固定寄存器中的表达式
    Local,      // 局部变量
    Upval,      // Upvalue
    Indexed,    // 表索引表达式
    Jump,       // 跳转表达式
    Relocatable,     // 可重定位的表达式（结果可以放到任意寄存器）
    Call,       // 函数调用
    Vararg      // 可变参数
};

/**
 * @brief 表达式描述符
 * 
 * 描述表达式的类型、值和位置信息。
 */
struct ExprDesc {
    ExprKind kind;
    
    union {
        struct {
            i32 info;      // 寄存器索引或常量索引
            i32 aux;       // 辅助信息
        } s;
        f64 nval;          // 数字值
    } u;
    
    i32 t;  // 真值跳转链表
    i32 f;  // 假值跳转链表
    
    ExprDesc() : kind(ExprKind::Void), t(NO_JUMP), f(NO_JUMP) {
        u.s.info = 0;
        u.s.aux = 0;
    }
};

/**
 * @brief 局部变量信息
 */
struct LocalVar {
    Str name;       // 变量名
    i32 reg;        // 寄存器索引
    i32 startpc;    // 作用域开始位置
    i32 endpc;      // 作用域结束位置
    
    LocalVar(const Str& n, i32 r, i32 start)
        : name(n), reg(r), startpc(start), endpc(-1) {}
};

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
    Proto* generate(const Chunk& chunk);
    
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
    void adjustLocalVars(i32 nvars);
    void removeLocalVars(i32 tolevel);
    
    // =====================================================================
    // 跳转管理
    // =====================================================================
    
    i32 jump();
    void patchList(i32 list, i32 target);
    void patchToHere(i32 list);
    i32 getLabel();
    
    // =====================================================================
    // 表达式代码生成
    // =====================================================================
    
    void expr(const Expr& e, ExprDesc& desc);
    void discharge(ExprDesc& desc, i32 reg);
    i32 exp2RK(ExprDesc& desc);
    i32 exp2AnyReg(ExprDesc& desc);
    void exp2NextReg(ExprDesc& desc);
    void exp2Val(ExprDesc& desc);
    
    // =====================================================================
    // 语句代码生成
    // =====================================================================
    
    void statement(const Stmt& s);
    void block(const Vec<StmtPtr>& stmts);
    
private:
    StringPool* pool_;          // 字符串池
    Proto* proto_;              // 当前函数原型
    i32 freereg_;               // 第一个空闲寄存器
    i32 nactvar_;               // 活跃局部变量数量
    Vec<LocalVar> localVars_;   // 局部变量列表
    i32 pc_;                    // 当前指令索引
};

}  // namespace Lua

