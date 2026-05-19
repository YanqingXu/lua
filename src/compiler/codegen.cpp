/**
 * @file codegen.cpp
 * @brief Lua字节码生成器实现
 */

#include "compiler/codegen.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/value.hpp"
#include "gc/garbage_collector.hpp"
#include <stdexcept>

namespace Lua {

// =====================================================================
// 构造和析构
// =====================================================================

CodeGenerator::CodeGenerator(StringPool* pool)
    : services_(RuntimeServices::fromSingletons())
    , pool_(pool)
    , parent_(nullptr)
    , proto_(nullptr)
    , regs_()
    , locals_()
    , blocks_()
    , upvalueCtx_()
    , pc_(0)
    , currentLine_(0)
{
    if (pool == nullptr) {
        throw std::invalid_argument("StringPool cannot be null");
    }
}

CodeGenerator::CodeGenerator(RuntimeServices& services)
    : services_(services)
    , pool_(&services.strings)
    , parent_(nullptr)
    , proto_(nullptr)
    , regs_()
    , locals_()
    , blocks_()
    , upvalueCtx_()
    , pc_(0)
    , currentLine_(0)
{
}

CodeGenerator::~CodeGenerator() {
    // Proto由GC管理
}

// =====================================================================
// 主生成函数
// =====================================================================

Proto* CodeGenerator::generate(const Chunk& chunk, StrView sourceName) {
    // 创建新的Proto对象
    proto_ = new Proto();
    services_.gc.registerObject(proto_);
    regs_.bind(proto_);
    proto_->setMaxStackSize(2);  // 最小栈大小
    proto_->setVararg(true);     // 主函数（chunk）默认是可变参数的
    if (!sourceName.empty()) {
        proto_->setSource(pool_->intern(sourceName));
    }

    // 重置状态
    regs_.setFreeReg(0);
    locals_.nactvar_ = 0;
    locals_.localVars_.clear();
    upvalueCtx_.upvalues_.clear();
    pc_ = 0;
    currentLine_ = 0;
    
    // 生成语句块
    block(chunk.statements);
    
    // 添加RETURN指令（如果最后一条指令不是RETURN）
    if (proto_->getInstructionCount() == 0 || 
        GET_OPCODE(proto_->getInstruction(proto_->getInstructionCount() - 1)) != OpCode::RETURN) {
        codeABC(OpCode::RETURN, 0, 1, 0);  // return (no values)
    }

    attachDebugMetadata();
    
    return proto_;
}

// =====================================================================
// 指令生成
// =====================================================================

i32 CodeGenerator::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    // 在生成指令前刷新所有待处理跳转
    flushPendingJumps();

    Instruction inst = CREATE_ABC(op, a, b, c);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(currentLine_);
    return pc;
}

i32 CodeGenerator::codeABx(OpCode op, i32 a, i32 bx) {
    // 在生成指令前刷新待处理跳转
    flushPendingJumps();

    Instruction inst = CREATE_ABx(op, a, bx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(currentLine_);
    return pc;
}

i32 CodeGenerator::codeAsBx(OpCode op, i32 a, i32 sbx) {
    // 在生成指令前刷新待处理跳转
    flushPendingJumps();

    Instruction inst = CREATE_AsBx(op, a, sbx);
    i32 pc = static_cast<i32>(proto_->addInstruction(inst));
    proto_->addLineInfo(currentLine_);
    return pc;
}

// =====================================================================
// 寄存器管理
// =====================================================================

i32 CodeGenerator::allocReg() {
    return regs_.alloc();
}

void CodeGenerator::freeReg(i32 reg) {
    regs_.freeReg(reg, locals_.nactvar_);
}

void CodeGenerator::freeRegs(i32 n) {
    regs_.freeRegs(n);
}

void CodeGenerator::checkStack(i32 n) {
    regs_.checkStack(n);
}

// =====================================================================
// 常量表管理
// =====================================================================

i32 CodeGenerator::numberConstant(f64 value) {
    Value v(value);
    return static_cast<i32>(proto_->addConstant(v));
}

i32 CodeGenerator::stringConstant(const Str& value) {
    GCString* str = pool_->intern(value);
    Value v(str);
    return static_cast<i32>(proto_->addConstant(v));
}

i32 CodeGenerator::boolConstant(bool value) {
    Value v(value);
    return static_cast<i32>(proto_->addConstant(v));
}

i32 CodeGenerator::nilConstant() {
    Value v;  // nil
    return static_cast<i32>(proto_->addConstant(v));
}

// =====================================================================
// 局部变量管理
// =====================================================================

i32 CodeGenerator::addLocalVar(const Str& name) {
    i32 reg = regs_.current();
    locals_.localVars_.emplace_back(name, reg, static_cast<i32>(proto_->getInstructionCount()));
    regs_.reserve(1);
    checkStack(0);
    return reg;
}

i32 CodeGenerator::findLocalVar(const Str& name) {
    return locals_.findLocal(name);
}

void CodeGenerator::adjustLocalVars(i32 nvars) {
    locals_.nactvar_ += nvars;
    regs_.resetToLocals(locals_.nactvar_);
    regs_.checkStack(0);
}

void CodeGenerator::removeLocalVars(i32 tolevel) {
    closeScopeUpvalues(tolevel);
    i32 pc = static_cast<i32>(proto_->getInstructionCount());
    locals_.closeLocals(tolevel, pc);
    regs_.resetToLocals(locals_.nactvar_);
    regs_.checkStack(0);
}

}  // namespace Lua
