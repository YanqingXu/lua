#pragma once

/**
 * @file bytecode_builder.hpp
 * @brief Narrow Proto write boundary for bytecode emission.
 */

#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/string_pool.hpp"
#include "core/value.hpp"

#include <stdexcept>

namespace Lua {

/**
 * @brief Small facade for writes to the current Proto during code generation.
 *
 * CodeGenerator lowering code still decides what to emit; this class owns the
 * direct mutation of bytecode, line info, constants, sub-protos, and debug names.
 */
class BytecodeBuilder {
public:
    BytecodeBuilder() = default;

    void bind(Proto& proto, StringPool& pool) noexcept {
        proto_ = &proto;
        pool_ = &pool;
    }

    bool isBound() const noexcept {
        return proto_ != nullptr && pool_ != nullptr;
    }

    Proto& proto() {
        return requireProto();
    }

    const Proto& proto() const {
        return requireProto();
    }

    i32 instructionCount() const {
        return static_cast<i32>(requireProto().getInstructionCount());
    }

    bool hasInstructions() const {
        return instructionCount() > 0;
    }

    OpCode lastOpcode() const {
        if (!hasInstructions()) {
            throw std::out_of_range("BytecodeBuilder has no instructions");
        }
        return GET_OPCODE(instruction(instructionCount() - 1));
    }

    i32 emitABC(i32 line, OpCode op, i32 a, i32 b, i32 c) {
        return emit(line, CREATE_ABC(op, a, b, c));
    }

    i32 emitABx(i32 line, OpCode op, i32 a, i32 bx) {
        return emit(line, CREATE_ABx(op, a, bx));
    }

    i32 emitAsBx(i32 line, OpCode op, i32 a, i32 sbx) {
        return emit(line, CREATE_AsBx(op, a, sbx));
    }

    i32 emitRaw(i32 line, Instruction inst) {
        return emit(line, inst);
    }

    Instruction instruction(i32 pc) const {
        if (pc < 0) {
            throw std::out_of_range("Instruction index out of range");
        }
        return requireProto().getInstruction(static_cast<usize>(pc));
    }

    void replaceInstruction(i32 pc, Instruction inst) {
        if (pc < 0) {
            throw std::out_of_range("Instruction index out of range");
        }
        requireProto().setInstruction(static_cast<usize>(pc), inst);
    }

    i32 addNumberConstant(f64 value) {
        return addConstant(Value(value));
    }

    i32 addStringConstant(StrView value) {
        return addConstant(Value(requirePool().intern(value)));
    }

    i32 addBoolConstant(bool value) {
        return addConstant(Value(value));
    }

    i32 addNilConstant() {
        return addConstant(Value());
    }

    i32 addSubProto(Proto* proto) {
        return static_cast<i32>(requireProto().addProto(proto));
    }

    void setSource(StrView sourceName) {
        requireProto().setSource(requirePool().intern(sourceName));
    }

    void addLocalDebug(StrView name, i32 startpc, i32 endpc, i32 reg) {
        requireProto().addLocVar(requirePool().intern(name), startpc, endpc, reg);
    }

private:
    i32 emit(i32 line, Instruction inst) {
        Proto& current = requireProto();
        i32 pc = static_cast<i32>(current.addInstruction(inst));
        current.addLineInfo(line);
        return pc;
    }

    i32 addConstant(const Value& value) {
        return static_cast<i32>(requireProto().addConstant(value));
    }

    Proto& requireProto() const {
        if (proto_ == nullptr) {
            throw std::logic_error("BytecodeBuilder is not bound to a Proto");
        }
        return *proto_;
    }

    StringPool& requirePool() const {
        if (pool_ == nullptr) {
            throw std::logic_error("BytecodeBuilder is not bound to a StringPool");
        }
        return *pool_;
    }

    Proto* proto_ = nullptr;
    StringPool* pool_ = nullptr;
};

}  // namespace Lua
