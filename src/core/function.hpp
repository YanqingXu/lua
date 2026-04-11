/**
 * @file function.hpp
 * @brief Lua函数对象：函数原型和闭包实现
 * 
 * 本文件实现了Lua的函数对象系统，包括函数原型（Proto）和闭包（Closure）。
 * 
 * 核心概念：
 * - Proto：函数原型，包含字节码、常量表等编译时信息
 * - Closure：闭包对象，包装函数原型和上值（upvalues）
 * - C函数：用C++实现的函数，可以被Lua调用
 * - Lua函数：用Lua编写的函数，由虚拟机执行
 * 
 * 设计特点：
 * - 简化实现：当前版本不包含完整的字节码系统
 * - 支持C函数：可以注册C++函数供Lua调用
 * - 为后续扩展预留接口：字节码、上值等
 * - 继承GCObject：支持垃圾回收
 * 
 * 参考实现：
 * - lua_c_analysis/src/lobject.h - Proto和Closure定义
 * - lua_c_analysis/src/lfunc.h/c - 函数对象操作
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#ifndef LUA_CORE_FUNCTION_HPP
#define LUA_CORE_FUNCTION_HPP

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include "core/value.hpp"
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>

namespace Lua {

// =====================================================================
// 常量去重辅助类型
// =====================================================================

/**
 * @brief 常量键类型 - 用于常量表去重的哈希键
 *
 * 参考Lua 5.1的addk()实现，使用哈希表对常量进行去重。
 * 该结构可以表示nil、bool、number、string四种常量类型，
 * 支持作为std::unordered_map的键。
 *
 * 设计说明：
 * - nil使用特殊的sentinel标记（因为nil不能作为哈希表的键）
 * - string使用GCString*指针（依赖StringPool的内部化保证唯一性）
 * - number使用double值直接哈希
 * - bool值直接哈希
 */
struct ConstantKey {
    /// 键的内部表示：monostate=nil, bool, f64(number), GCString*(string)
    using KeyVariant = std::variant<std::monostate, bool, f64, GCString*>;
    KeyVariant key;

    /// 从Value构造ConstantKey（仅支持常量类型：nil/bool/number/string）
    static ConstantKey fromValue(const Value& v) {
        ConstantKey ck;
        if (v.isNil()) {
            ck.key = std::monostate{};
        } else if (v.isBoolean()) {
            ck.key = v.asBoolean();
        } else if (v.isNumber()) {
            ck.key = v.asNumber();
        } else if (v.isString()) {
            ck.key = v.asString();
        } else {
            // 非常量类型不参与去重，使用monostate作为fallback
            // 这意味着非常量类型的值不会被去重
            ck.key = std::monostate{};
        }
        return ck;
    }

    bool operator==(const ConstantKey& other) const {
        return key == other.key;
    }
};

/// ConstantKey的哈希函数
struct ConstantKeyHash {
    std::size_t operator()(const ConstantKey& ck) const noexcept {
        return std::visit([](const auto& val) -> std::size_t {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                // nil的哈希值使用固定值
                return std::hash<int>{}(0);
            } else if constexpr (std::is_same_v<T, bool>) {
                return std::hash<bool>{}(val);
            } else if constexpr (std::is_same_v<T, f64>) {
                return std::hash<f64>{}(val);
            } else if constexpr (std::is_same_v<T, GCString*>) {
                // 字符串使用GCString的预计算哈希值
                return val ? val->getHash() : 0;
            } else {
                return 0;
            }
        }, ck.key);
    }
};

// 前向声明
class LuaState;
class GCString;
class Upvalue;

// 指令类型（32位无符号整数）
using Instruction = u32;

/**
 * @brief C函数类型定义
 *
 * C函数接受LuaState指针作为参数，返回结果数量。
 *
 * @param L Lua状态指针
 * @return 返回值数量
 */
using CFunction = i32 (*)(LuaState* L);

// =====================================================================
// 可变参数标志常量（Lua 5.1兼容）
// =====================================================================

/**
 * @brief 可变参数掩码：用于标识函数的可变参数特性
 *
 * Lua 5.1引入了新的可变参数语法（...），这些标志位用于标识
 * 函数如何处理可变参数。
 */

/// 有参数标志：函数有实际的可变参数
constexpr u8 VARARG_HASARG = 1;

/// 是可变参数函数：函数声明时使用了...语法
constexpr u8 VARARG_ISVARARG = 2;

/// 需要参数：函数需要可变参数（旧式兼容）
constexpr u8 VARARG_NEEDSARG = 4;

/**
 * @brief 局部变量信息结构
 *
 * 存储函数中局部变量的调试信息，包括变量名称和生命周期。
 * 对应Lua C实现中的LocVar结构。
 *
 * 用途：
 * - 调试器显示变量名称
 * - 错误报告中包含变量信息
 * - 反射和元编程支持
 *
 * 生命周期：
 * - startpc: 变量开始有效的字节码位置
 * - endpc: 变量失效的字节码位置（不包含）
 */
struct LocVar {
    GCString* varname;      ///< 变量名称
    i32 startpc;            ///< 起始PC：变量开始有效的字节码位置
    i32 endpc;              ///< 结束PC：变量失效的字节码位置（不包含）
    i32 reg;                ///< 对应的寄存器槽位（相对于当前栈帧base）

    /**
     * @brief 默认构造函数
     */
    LocVar() : varname(nullptr), startpc(0), endpc(0), reg(-1) {}

    /**
     * @brief 构造函数
     * @param name 变量名称
     * @param start 起始PC
     * @param end 结束PC
     * @param slot 寄存器槽位
     */
    LocVar(GCString* name, i32 start, i32 end, i32 slot)
        : varname(name), startpc(start), endpc(end), reg(slot) {}
};

/**
 * @brief 函数原型类（完整版）
 *
 * Proto包含了Lua函数编译后的所有信息，是Lua虚拟机执行的基础数据结构。
 * 本实现完全对应Lua 5.1.5 C版本中的Proto结构。
 *
 * 核心组成部分：
 * 1. **字节码序列**：
 *    - code_: 函数的可执行指令数组
 *    - 优化后的虚拟机代码
 *    - 支持调试的行号映射
 *
 * 2. **常量池**：
 *    - constants_: 字符串、数值、布尔值等常量
 *    - 嵌套函数的原型
 *    - 预计算的复杂常量
 *
 * 3. **变量信息**：
 *    - locvars_: 局部变量名称和作用域
 *    - upvalues_: 上值名称和索引
 *    - numParams_: 参数数量
 *    - isVararg_: 可变参数标志
 *
 * 4. **调试信息**：
 *    - source_: 源文件名
 *    - lineInfo_: 字节码到源码行号的映射
 *    - linedefined_: 函数定义开始行号
 *    - lastlinedefined_: 函数定义结束行号
 *
 * 5. **元数据**：
 *    - maxStackSize_: 栈大小需求
 *    - nups_: 上值数量
 *    - gclist_: 垃圾回收链表指针
 *
 * 参考实现：
 * - lua_c_analysis/src/lobject.h - Proto结构定义
 * - lua_c_analysis/src/lfunc.c - Proto管理函数
 */
class Proto : public GCObject {
public:
    /**
     * @brief 构造函数
     */
    Proto();
    
    /**
     * @brief 析构函数
     */
    ~Proto() override;
    
    // =====================================================================
    // 基本属性访问
    // =====================================================================
    
    /**
     * @brief 获取参数数量
     * @return 固定参数个数
     */
    u8 getNumParams() const noexcept { return numParams_; }
    
    /**
     * @brief 设置参数数量
     * @param n 参数个数
     */
    void setNumParams(u8 n) noexcept { numParams_ = n; }
    
    /**
     * @brief 是否为可变参数函数
     * @return 如果接受可变参数返回true
     *
     * 注意：Lua 5.1使用位标志表示可变参数特性：
     * - VARARG_HASARG (1): 函数有实际的可变参数
     * - VARARG_ISVARARG (2): 函数声明时使用了...语法
     */
    bool isVararg() const noexcept { return isVararg_ != 0; }

    /**
     * @brief 获取可变参数标志（原始值）
     * @return 可变参数标志位
     */
    u8 getVarargFlags() const noexcept { return isVararg_; }

    /**
     * @brief 设置可变参数标志
     * @param vararg 是否为可变参数
     */
    void setVararg(bool vararg) noexcept {
        isVararg_ = vararg ? 2 : 0;  // VARARG_ISVARARG = 2
    }

    /**
     * @brief 设置可变参数标志（原始值）
     * @param flags 可变参数标志位
     */
    void setVarargFlags(u8 flags) noexcept { isVararg_ = flags; }
    
    /**
     * @brief 获取最大栈大小
     * @return 函数执行需要的最大栈空间
     */
    u8 getMaxStackSize() const noexcept { return maxStackSize_; }
    
    /**
     * @brief 设置最大栈大小
     * @param size 栈大小
     */
    void setMaxStackSize(u8 size) noexcept { maxStackSize_ = size; }
    
    /**
     * @brief 获取源文件名
     * @return 源文件名字符串
     */
    GCString* getSource() const noexcept { return source_; }
    
    /**
     * @brief 设置源文件名
     * @param src 源文件名
     */
    void setSource(GCString* src) noexcept { source_ = src; }
    
    // =====================================================================
    // 常量表操作（简化版）
    // =====================================================================
    
    /**
     * @brief 添加常量
     * @param value 常量值
     * @return 常量在常量表中的索引
     */
    usize addConstant(const Value& value);
    
    /**
     * @brief 获取常量
     * @param index 常量索引
     * @return 常量值
     */
    Value getConstant(usize index) const;
    
    /**
     * @brief 获取常量数量
     * @return 常量表大小
     */
    usize getConstantCount() const noexcept { return constants_.size(); }

    // =====================================================================
    // 字节码操作
    // =====================================================================

    /**
     * @brief 添加指令
     * @param inst 指令
     * @return 指令在代码数组中的索引
     */
    usize addInstruction(Instruction inst);

    /**
     * @brief 获取指令
     * @param index 指令索引
     * @return 指令
     */
    Instruction getInstruction(usize index) const;

    /**
     * @brief 设置指令
     * @param index 指令索引
     * @param inst 新指令
     */
    void setInstruction(usize index, Instruction inst);

    /**
     * @brief 获取指令数量
     * @return 代码数组大小
     */
    usize getInstructionCount() const noexcept { return code_.size(); }

    /**
     * @brief 获取代码数组（只读）
     * @return 代码数组引用
     */
    const Vec<Instruction>& getCode() const noexcept { return code_; }

    /**
     * @brief 获取代码数组（可写）
     * @return 代码数组引用
     */
    Vec<Instruction>& getCode() noexcept { return code_; }

    // =====================================================================
    // 行号信息
    // =====================================================================

    /**
     * @brief 添加行号信息
     * @param line 行号
     */
    void addLineInfo(i32 line);

    /**
     * @brief 获取指令对应的行号
     * @param pc 指令索引
     * @return 行号
     */
    i32 getLine(usize pc) const;

    /**
     * @brief 获取行号信息数组（只读）
     * @return 行号信息数组引用
     */
    const Vec<i32>& getLineInfo() const noexcept { return lineInfo_; }

    /**
     * @brief 获取行号信息数组（可写）
     * @return 行号信息数组引用
     */
    Vec<i32>& getLineInfo() noexcept { return lineInfo_; }

    // =====================================================================
    // 子函数原型管理
    // =====================================================================

    /**
     * @brief 添加子函数原型
     * @param proto 子函数原型指针
     * @return 子函数在数组中的索引
     */
    usize addProto(Proto* proto);

    /**
     * @brief 获取子函数原型
     * @param index 子函数索引
     * @return 子函数原型指针
     */
    Proto* getSubProto(usize index) const;

    /**
     * @brief 获取子函数数量
     * @return 子函数数量
     */
    usize getSubProtoCount() const noexcept { return subProtos_.size(); }

    // =====================================================================
    // 局部变量信息管理（调试支持）
    // =====================================================================

    /**
     * @brief 添加局部变量信息
     * @param varname 变量名称
     * @param startpc 起始PC
     * @param endpc 结束PC
     * @return 变量在数组中的索引
     */
    usize addLocVar(GCString* varname, i32 startpc, i32 endpc, i32 reg);

    /**
     * @brief 获取局部变量信息
     * @param index 变量索引
     * @return 局部变量信息
     */
    const LocVar& getLocVar(usize index) const;

    /**
     * @brief 获取局部变量数量
     * @return 局部变量数量
     */
    usize getLocVarCount() const noexcept { return locvars_.size(); }

    /**
     * @brief 获取指定PC位置的局部变量名称
     * @param localNumber 局部变量编号（从1开始）
     * @param pc 程序计数器位置
     * @return 变量名称，如果未找到返回nullptr
     *
     * 对应Lua C实现中的luaF_getlocalname函数
     */
    const char* getLocalName(i32 localNumber, i32 pc) const;

    /**
     * @brief 获取指定PC位置的局部变量调试信息
     * @param localNumber 局部变量编号（从1开始）
     * @param pc 程序计数器位置
     * @return 局部变量信息指针，未找到返回nullptr
     */
    const LocVar* getLocalVarInfo(i32 localNumber, i32 pc) const;

    // =====================================================================
    // 上值名称管理（调试支持）
    // =====================================================================

    /**
     * @brief 添加上值名称
     * @param name 上值名称
     * @return 上值在数组中的索引
     */
    usize addUpvalueName(GCString* name);

    /**
     * @brief 获取上值名称
     * @param index 上值索引
     * @return 上值名称
     */
    GCString* getUpvalueName(usize index) const;

    /**
     * @brief 获取上值名称数量
     * @return 上值名称数量
     */
    usize getUpvalueNameCount() const noexcept { return upvalueNames_.size(); }

    // =====================================================================
    // 函数定义位置信息
    // =====================================================================

    /**
     * @brief 获取函数定义开始行号
     * @return 开始行号
     */
    i32 getLineDefined() const noexcept { return linedefined_; }

    /**
     * @brief 设置函数定义开始行号
     * @param line 行号
     */
    void setLineDefined(i32 line) noexcept { linedefined_ = line; }

    /**
     * @brief 获取函数定义结束行号
     * @return 结束行号
     */
    i32 getLastLineDefined() const noexcept { return lastlinedefined_; }

    /**
     * @brief 设置函数定义结束行号
     * @param line 行号
     */
    void setLastLineDefined(i32 line) noexcept { lastlinedefined_ = line; }

    // =====================================================================
    // 上值数量管理
    // =====================================================================

    /**
     * @brief 获取上值数量
     * @return 上值数量
     */
    u8 getNumUpvalues() const noexcept { return nups_; }

    /**
     * @brief 设置上值数量
     * @param n 上值数量
     */
    void setNumUpvalues(u8 n) noexcept { nups_ = n; }

    // =====================================================================
    // GC链表管理
    // =====================================================================

    /**
     * @brief 获取GC链表指针
     * @return GC链表指针
     */
    GCObject* getGCList() const noexcept { return gclist_; }

    /**
     * @brief 设置GC链表指针
     * @param list GC链表指针
     */
    void setGCList(GCObject* list) noexcept { gclist_ = list; }

    // =====================================================================
    // GCObject接口实现
    // =====================================================================

    void mark() override;
    usize getSize() const override;

private:
    // =====================================================================
    // 核心数据结构
    // =====================================================================

    /// 常量表：函数使用的常量值数组
    Vec<Value> constants_;

    /// 常量去重缓存：从常量键到常量表索引的映射
    /// 参考Lua 5.1中addk()使用的哈希表（fs->h）
    std::unordered_map<ConstantKey, usize, ConstantKeyHash> constantMap_;

    /// 字节码数组：函数的指令序列
    Vec<Instruction> code_;

    /// 子函数原型数组：函数内定义的嵌套函数
    Vec<Proto*> subProtos_;

    /// 行号信息：字节码到源码行号的映射（每条指令对应一个行号）
    Vec<i32> lineInfo_;

    /// 局部变量信息：调试用的局部变量描述
    Vec<LocVar> locvars_;

    /// 上值名称数组：闭包变量的名称（用于调试）
    Vec<GCString*> upvalueNames_;

    // =====================================================================
    // 元数据字段
    // =====================================================================

    /// 源文件名：函数所在的源文件
    GCString* source_;

    /// 函数定义开始行号
    i32 linedefined_;

    /// 函数定义结束行号
    i32 lastlinedefined_;

    /// GC链表指针：用于垃圾回收遍历
    GCObject* gclist_;

    // =====================================================================
    // 函数签名信息（字节类型）
    // =====================================================================

    /// 上值数量：函数引用的外部变量个数
    u8 nups_;

    /// 参数数量：函数的固定参数个数
    u8 numParams_;

    /// 可变参数标志：函数是否接受可变数量的参数
    /// 对应Lua 5.1的VARARG_HASARG和VARARG_ISVARARG标志
    u8 isVararg_;

    /// 最大栈大小：函数执行时需要的最大栈空间
    u8 maxStackSize_;
};

/**
 * @brief 函数类（闭包）
 *
 * Function是Lua中的函数对象，可以是C函数或Lua函数。
 * 在Lua中也称为Closure（闭包）。
 *
 * C函数闭包：
 * - 包装C++函数指针
 * - 可以有上值（upvalues）
 * - 直接由C++代码执行
 *
 * Lua函数闭包：
 * - 包含函数原型（Proto）
 * - 可以有上值（upvalues）
 * - 由虚拟机解释执行
 *
 * 当前版本：
 * - 支持C函数
 * - 支持Lua函数（但暂无字节码执行）
 * - 支持上值（Upvalue）管理
 */
class Function : public GCObject {
public:
    /**
     * @brief 创建C函数闭包
     * @param func C函数指针
     */
    explicit Function(CFunction func);

    /**
     * @brief 创建Lua函数闭包
     * @param proto 函数原型
     */
    explicit Function(Proto* proto);

    /**
     * @brief 析构函数
     */
    ~Function() override;
    
    // =====================================================================
    // 类型检查
    // =====================================================================
    
    /**
     * @brief 是否为C函数
     * @return 如果是C函数返回true
     */
    bool isCFunction() const noexcept { return isC_; }
    
    /**
     * @brief 是否为Lua函数
     * @return 如果是Lua函数返回true
     */
    bool isLuaFunction() const noexcept { return !isC_; }
    
    // =====================================================================
    // C函数访问
    // =====================================================================
    
    /**
     * @brief 获取C函数指针
     * @return C函数指针（如果不是C函数返回nullptr）
     */
    CFunction getCFunction() const noexcept {
        return isC_ ? cFunction_ : nullptr;
    }
    
    // =====================================================================
    // Lua函数访问
    // =====================================================================
    
    /**
     * @brief 获取函数原型
     * @return 函数原型指针（如果不是Lua函数返回nullptr）
     */
    Proto* getProto() const noexcept {
        return isC_ ? nullptr : proto_;
    }

    // =====================================================================
    // Upvalue管理（仅Lua函数）
    // =====================================================================

    /**
     * @brief 获取Upvalue数量
     * @return Upvalue数量
     *
     * 注意：C函数也可以有upvalue，但当前实现仅支持Lua函数
     */
    usize getUpvalueCount() const noexcept {
        return upvalues_.size();
    }

    /**
     * @brief 获取指定索引的Upvalue
     * @param index Upvalue索引（从0开始）
     * @return Upvalue指针，如果索引越界返回nullptr
     */
    Upvalue* getUpvalue(usize index) const;

    /**
     * @brief 设置指定索引的Upvalue
     * @param index Upvalue索引（从0开始）
     * @param upvalue Upvalue指针
     *
     * 注意：如果索引越界会抛出异常
     */
    void setUpvalue(usize index, Upvalue* upvalue);

    /**
     * @brief 添加Upvalue到数组末尾
     * @param upvalue Upvalue指针
     */
    void addUpvalue(Upvalue* upvalue);

    // =====================================================================
    // 环境表管理（Lua 5.1兼容）
    // =====================================================================

    /**
     * @brief 获取函数的环境表
     * @return 环境表指针，如果未设置返回nullptr
     *
     * 环境表用于控制函数的全局变量访问范围，是Lua 5.1的核心特性。
     * 每个函数可以有独立的环境表，实现沙箱、模块隔离等功能。
     */
    Table* getEnv() const noexcept {
        return env_;
    }

    /**
     * @brief 设置函数的环境表
     * @param env 环境表指针
     *
     * 设置函数的环境表后，该函数的GETGLOBAL/SETGLOBAL指令将使用
     * 此环境表而非全局表。这是实现setfenv/getfenv的基础。
     */
    void setEnv(Table* env) noexcept {
        env_ = env;
    }

    // =====================================================================
    // ClosureHeader 字段访问（Lua C兼容）
    // =====================================================================

    /**
     * @brief 获取上值数量（ClosureHeader字段）
     * @return 上值数量
     *
     * 对应Lua C实现中的nupvalues字段。
     * 注意：这与upvalues_.size()可能不同，需要保持同步。
     */
    u8 getNumUpvalues() const noexcept { return nupvalues_; }

    /**
     * @brief 设置上值数量（ClosureHeader字段）
     * @param n 上值数量
     *
     * 对应Lua C实现中的nupvalues字段。
     * 注意：调用此方法时应确保与upvalues_数组大小保持一致。
     */
    void setNumUpvalues(u8 n) noexcept { nupvalues_ = n; }

    /**
     * @brief 获取GC链表指针（ClosureHeader字段）
     * @return GC链表指针
     *
     * 对应Lua C实现中的gclist字段，用于增量GC和分代GC。
     */
    GCObject* getGCList() const noexcept { return gclist_; }

    /**
     * @brief 设置GC链表指针（ClosureHeader字段）
     * @param list GC链表指针
     *
     * 对应Lua C实现中的gclist字段，用于增量GC和分代GC。
     */
    void setGCList(GCObject* list) noexcept { gclist_ = list; }

    // =====================================================================
    // GCObject接口实现
    // =====================================================================

    void mark() override;
    usize getSize() const override;

private:
    // =====================================================================
    // ClosureHeader 字段（对应Lua C实现）
    // =====================================================================

    /// 是否为C函数（对应C实现的lu_byte isC）
    /// 注意：为了与C实现完全兼容，应该使用u8类型，但为了保持现有代码兼容性暂时保留bool
    bool isC_;

    /// 上值数量（对应C实现的lu_byte nupvalues）
    /// 注意：这是新增字段，用于完全兼容Lua C实现
    u8 nupvalues_;

    /// GC链表指针（对应C实现的GCObject *gclist）
    /// 用于增量GC和分代GC的灰色对象链表遍历
    /// 注意：这是新增字段，用于完全兼容Lua C实现
    GCObject* gclist_;

    /// 环境表（对应C实现的struct Table *env）
    /// 用于控制函数的全局变量访问范围
    /// 如果为nullptr，则使用LuaState的全局表
    Table* env_;

    // =====================================================================
    // 函数特有字段
    // =====================================================================

    /// C函数指针（仅当isC_为true时有效）
    /// 对应CClosure的lua_CFunction f字段
    CFunction cFunction_;

    /// 函数原型（仅当isC_为false时有效）
    /// 对应LClosure的struct Proto *p字段
    Proto* proto_;

    /// Upvalue数组（闭包捕获的外部变量）
    /// 注意：
    /// - 对于C函数（CClosure），应该直接存储Value（对应TValue upvalue[1]）
    /// - 对于Lua函数（LClosure），存储Upvalue*指针（对应UpVal *upvals[1]）
    /// - 当前实现统一使用Upvalue*，这与C实现略有不同
    /// - Upvalue由GC管理，这里只持有指针
    Vec<Upvalue*> upvalues_;
};

} // namespace Lua

#endif // LUA_CORE_FUNCTION_HPP

