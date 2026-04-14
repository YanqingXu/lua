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
    Global,     // 全局变量（Lua 5.1 GETGLOBAL/SETGLOBAL）
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
 * @brief Upvalue捕获信息（用于CLOSURE后伪指令生成）
 */
struct UpvalueCapture {
    Str name;       // Upvalue名称（用于调试/打印）
    bool inStack;   // true: 来自父函数局部变量（MOVE）；false: 来自父函数upvalue（GETUPVAL）
    i32 index;      // 源索引（局部寄存器索引或父upvalue索引）

    UpvalueCapture(const Str& n, bool inStackVar, i32 idx)
        : name(n), inStack(inStackVar), index(idx) {}
};

/**
 * @brief 代码块信息
 *
 * 用于管理嵌套作用域和break语句的跳转目标。
 * 参考官方Lua的BlockCnt结构。
 */
struct BlockInfo {
    BlockInfo* previous;    // 父级代码块
    i32 breaklist;          // break语句的跳转链表
    i32 nactvar;            // 进入块时的活跃变量数
    bool isbreakable;       // 是否可以使用break（循环块）

    BlockInfo(BlockInfo* prev, i32 nact, bool breakable)
        : previous(prev), breaklist(NO_JUMP), nactvar(nact), isbreakable(breakable) {}
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
    Proto* generate(const Chunk& chunk, StrView sourceName = {});
    
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

    // =====================================================================
    // 跳转管理
    // =====================================================================
    
    i32 jump();
    void patchList(i32 list, i32 target);
    i32 getLabel();
    
    // =====================================================================
    // 表达式代码生成
    // =====================================================================

    void expr(const Expr& e, ExprDesc& desc);
    void emitExpr(const NilExpr& e, ExprDesc& desc);
    void emitExpr(const BoolExpr& e, ExprDesc& desc);
    void emitExpr(const NumberExpr& e, ExprDesc& desc);
    void emitExpr(const StringExpr& e, ExprDesc& desc);
    void emitExpr(const VarargExpr& e, ExprDesc& desc);
    void emitExpr(const NameExpr& e, ExprDesc& desc);
    void emitExpr(const BinaryExpr& e, ExprDesc& desc);
    void emitExpr(const UnaryExpr& e, ExprDesc& desc);
    void emitExpr(const TableExpr& e, ExprDesc& desc);
    void emitExpr(const CallExpr& e, ExprDesc& desc);
    void emitExpr(const IndexExpr& e, ExprDesc& desc);
    void emitExpr(const MemberExpr& e, ExprDesc& desc);
    void emitExpr(const FunctionExpr& e, ExprDesc& desc);
    void emitExpr(const ParenExpr& e, ExprDesc& desc);
    void discharge(ExprDesc& desc, i32 reg);
    i32 exp2RK(ExprDesc& desc);
    i32 exp2AnyReg(ExprDesc& desc);
    void exp2NextReg(ExprDesc& desc);
    void exp2Val(ExprDesc& desc);

    // 表索引和成员访问
    void luaK_indexed(ExprDesc& t, ExprDesc& k);
    void luaK_self(ExprDesc& e, ExprDesc& key);
    void luaK_storevar(ExprDesc& var, ExprDesc& ex);

    // 算术和比较指令生成
    void codearith(OpCode op, ExprDesc& e1, ExprDesc& e2);
    void codecomp(OpCode op, i32 cond, ExprDesc& e1, ExprDesc& e2);
    void codenot(ExprDesc& e);

    // 跳转处理
    void luaK_goiftrue(ExprDesc& e);
    void luaK_goiffalse(ExprDesc& e);
    void luaK_dischargevars(ExprDesc& e);
    void luaK_concat(i32& l1, i32 l2);
    void invertJump(ExprDesc& e);
    i32 jumponcond(ExprDesc& e, i32 cond);
    i32 condjump(OpCode op, i32 a, i32 b, i32 c);
    void patchtohere(i32 list);
    void dischargejpc();  // 修补所有待处理的跳转到当前位置
    void luaK_getlabel();
    i32 getjump(i32 pc);
    void fixjump(i32 pc, i32 dest);

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
    i32 freereg_;               // 第一个空闲寄存器
    i32 nactvar_;               // 活跃局部变量数量
    Vec<LocalVar> localVars_;   // 局部变量列表
    Vec<UpvalueCapture> upvalues_; // 当前函数捕获的upvalue列表
    i32 pc_;                    // 当前指令索引
    i32 jpc_;                   // 待处理的跳转链表
    BlockInfo* currentBlock_;   // 当前代码块
    i32 forcedCallBase_;        // 临时强制 CALL 基址（仅供当前最外层 CallExpr 使用）
    i32 currentLine_;           // 当前发射指令所属的源码行号
};

}  // namespace Lua

