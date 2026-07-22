#include "runtime/bytecode_verifier.hpp"

#include "compiler/opcode.hpp"
#include "core/function.hpp"

#include <limits>
#include <vector>

namespace Lua {

namespace {

class Verifier {
public:
    explicit Verifier(const BytecodeVerifierLimits& limits) : limits_(limits) {}

    std::expected<void, Str> run(const Proto& root) {
        if (!verifyProto(root, 1)) {
            return std::unexpected(error_);
        }
        return {};
    }

private:
    const BytecodeVerifierLimits& limits_;
    usize protoCount_ = 0;
    usize instructionCount_ = 0;
    usize constantCount_ = 0;
    usize debugEntryCount_ = 0;
    Str error_;
    std::vector<const Proto*> protoStack_;

    bool fail(const char* message, usize pc = std::numeric_limits<usize>::max()) {
        error_ = "bytecode verification failed: ";
        if (!protoStack_.empty()) {
            const Proto& proto = *protoStack_.back();
            error_ += "proto ";
            if (proto.getSource() != nullptr) {
                error_ += proto.getSource()->c_str();
            } else {
                error_ += "?";
            }
            error_ += ":" + std::to_string(proto.getLineDefined()) + ": ";
        }
        if (pc != std::numeric_limits<usize>::max()) {
            error_ += "pc " + std::to_string(pc) + ": ";
        }
        error_ += message;
        return false;
    }

    bool addWithin(usize value, usize& total, usize limit, const char* message) {
        if (value > limit || total > limit - value) {
            return fail(message);
        }
        total += value;
        return true;
    }

    static bool isConstantValue(const Value& value) {
        return value.isNil() || value.isBoolean() || value.isNumber() || value.isString();
    }

    bool verifyRegister(i32 reg, usize maxStack, usize pc) {
        return reg >= 0 && static_cast<usize>(reg) < maxStack ? true : fail("register out of range", pc);
    }

    bool verifyRange(i32 first, i32 count, usize maxStack, usize pc) {
        if (first < 0 || count < 0) {
            return fail("negative register range", pc);
        }
        const u64 end = static_cast<u64>(first) + static_cast<u64>(count);
        return end <= static_cast<u64>(maxStack) ? true : fail("register range out of bounds", pc);
    }

    bool verifyRK(i32 operand, const Proto& proto, usize maxStack, usize pc) {
        if (ISK(operand)) {
            return static_cast<usize>(INDEXK(operand)) < proto.getConstantCount()
                       ? true
                       : fail("RK constant index out of range", pc);
        }
        return verifyRegister(operand, maxStack, pc);
    }

    bool verifyTarget(i64 target, const std::vector<u8>& dataWords, usize pc) {
        if (target < 0 || static_cast<u64>(target) >= static_cast<u64>(dataWords.size())) {
            return fail("control-flow target out of range", pc);
        }
        if (dataWords[static_cast<usize>(target)] != 0) {
            return fail("control-flow target enters pseudo data", pc);
        }
        return true;
    }

    bool verifyJumpAt(const Proto& proto, usize jumpPc, const std::vector<u8>& dataWords, usize ownerPc) {
        if (jumpPc >= proto.getInstructionCount() || dataWords[jumpPc] != 0 ||
            GET_OPCODE(proto.getInstruction(jumpPc)) != OpCode::JMP) {
            return fail("test instruction is not followed by JMP", ownerPc);
        }
        const i64 target = static_cast<i64>(jumpPc) + 1 + GETARG_sBx(proto.getInstruction(jumpPc));
        return verifyTarget(target, dataWords, ownerPc);
    }

    bool verifyProto(const Proto& proto, usize depth) {
        protoStack_.push_back(&proto);
        if (depth > limits_.maxProtoDepth) {
            return fail("Proto nesting limit exceeded");
        }
        if (++protoCount_ > limits_.maxProtoCount) {
            return fail("Proto count limit exceeded");
        }

        const usize codeCount = proto.getInstructionCount();
        const usize constantCount = proto.getConstantCount();
        const usize debugCount = proto.getLineInfo().size() + proto.getLocVarCount() + proto.getUpvalueNameCount();
        if (!addWithin(codeCount, instructionCount_, limits_.maxInstructionCount, "instruction count limit exceeded") ||
            !addWithin(constantCount, constantCount_, limits_.maxConstantCount, "constant count limit exceeded") ||
            !addWithin(debugCount, debugEntryCount_, limits_.maxDebugEntries, "debug entry limit exceeded")) {
            return false;
        }

        const usize maxStack = proto.getMaxStackSize();
        if (maxStack == 0 || proto.getNumParams() > maxStack) {
            return fail("invalid function stack metadata");
        }
        if ((proto.getVarargFlags() & ~(VARARG_HASARG | VARARG_ISVARARG | VARARG_NEEDSARG)) != 0) {
            return fail("invalid vararg flags");
        }
        if (codeCount == 0) {
            return fail("empty instruction stream");
        }
        if (!proto.getLineInfo().empty() && proto.getLineInfo().size() != codeCount) {
            return fail("line metadata count does not match code");
        }
        if (proto.getUpvalueNameCount() > proto.getNumUpvalues()) {
            return fail("too many upvalue names");
        }

        for (usize index = 0; index < constantCount; ++index) {
            if (!isConstantValue(proto.getConstant(index))) {
                return fail("unsupported constant type");
            }
        }
        for (usize index = 0; index < proto.getLocVarCount(); ++index) {
            const LocVar& local = proto.getLocVar(index);
            if (local.startpc < 0 || local.endpc < local.startpc || static_cast<usize>(local.endpc) > codeCount ||
                (local.reg != -1 && !verifyRegister(local.reg, maxStack, 0))) {
                return fail("invalid local-variable metadata");
            }
        }

        std::vector<u8> dataWords(codeCount, 0);
        for (usize pc = 0; pc < codeCount; ++pc) {
            const Instruction instruction = proto.getInstruction(pc);
            const OpCode opcode = GET_OPCODE(instruction);
            if (!isValidOpcode(opcode)) {
                return fail("unknown opcode", pc);
            }

            if (opcode == OpCode::SETLIST && GETARG_C(instruction) == 0) {
                if (pc + 1 >= codeCount) {
                    return fail("SETLIST is missing its extended operand", pc);
                }
                const u32 block = proto.getInstruction(pc + 1);
                if (block == 0 || block > static_cast<u32>(std::numeric_limits<i32>::max()) ||
                    block - 1U > static_cast<u32>(std::numeric_limits<i32>::max() / LFIELDS_PER_FLUSH)) {
                    return fail("SETLIST extended operand overflows", pc);
                }
                dataWords[++pc] = 1;
                continue;
            }

            if (opcode == OpCode::CLOSURE) {
                const i32 bx = GETARG_Bx(instruction);
                if (static_cast<usize>(bx) >= proto.getSubProtoCount() || proto.getSubProto(bx) == nullptr) {
                    return fail("CLOSURE child Proto index out of range", pc);
                }
                const usize upvalues = proto.getSubProto(bx)->getNumUpvalues();
                if (upvalues > codeCount - pc - 1) {
                    return fail("CLOSURE is missing upvalue pseudo instructions", pc);
                }
                for (usize offset = 1; offset <= upvalues; ++offset) {
                    const Instruction pseudo = proto.getInstruction(pc + offset);
                    const OpCode pseudoOpcode = GET_OPCODE(pseudo);
                    if (pseudoOpcode == OpCode::MOVE) {
                        if (!verifyRegister(GETARG_B(pseudo), maxStack, pc + offset)) {
                            return false;
                        }
                    } else if (pseudoOpcode == OpCode::GETUPVAL) {
                        if (GETARG_B(pseudo) < 0 || static_cast<usize>(GETARG_B(pseudo)) >= proto.getNumUpvalues()) {
                            return fail("CLOSURE parent upvalue index out of range", pc + offset);
                        }
                    } else {
                        return fail("CLOSURE has an invalid upvalue pseudo instruction", pc + offset);
                    }
                    dataWords[pc + offset] = 1;
                }
                pc += upvalues;
            }
        }

        for (usize pc = 0; pc < codeCount; ++pc) {
            if (dataWords[pc] != 0) {
                continue;
            }
            if (!verifyInstruction(proto, pc, dataWords, maxStack)) {
                return false;
            }
        }

        for (usize index = 0; index < proto.getSubProtoCount(); ++index) {
            Proto* child = proto.getSubProto(index);
            if (child == nullptr || !verifyProto(*child, depth + 1)) {
                return child != nullptr ? false : fail("null child Proto");
            }
        }
        protoStack_.pop_back();
        return true;
    }

    bool verifyInstruction(const Proto& proto, usize pc, const std::vector<u8>& dataWords, usize maxStack) {
        const Instruction instruction = proto.getInstruction(pc);
        const OpCode opcode = GET_OPCODE(instruction);
        const i32 a = GETARG_A(instruction);
        const i32 b = GETARG_B(instruction);
        const i32 c = GETARG_C(instruction);
        const i32 bx = GETARG_Bx(instruction);

        const auto regA = [&] { return verifyRegister(a, maxStack, pc); };
        const auto regB = [&] { return verifyRegister(b, maxStack, pc); };
        const auto rkB = [&] { return verifyRK(b, proto, maxStack, pc); };
        const auto rkC = [&] { return verifyRK(c, proto, maxStack, pc); };
        const auto jump = [&] {
            return verifyTarget(static_cast<i64>(pc) + 1 + GETARG_sBx(instruction), dataWords, pc);
        };

        switch (opcode) {
        case OpCode::MOVE:
            return regA() && regB();
        case OpCode::LOADK:
            return regA() &&
                   (static_cast<usize>(bx) < proto.getConstantCount() || fail("constant index out of range", pc));
        case OpCode::LOADBOOL:
            return regA() && (c == 0 || verifyTarget(static_cast<i64>(pc) + 2, dataWords, pc));
        case OpCode::LOADNIL:
            return a <= b && verifyRange(a, b - a + 1, maxStack, pc);
        case OpCode::GETUPVAL:
            return regA() && (static_cast<usize>(b) < proto.getNumUpvalues() || fail("upvalue index out of range", pc));
        case OpCode::GETGLOBAL:
            return regA() &&
                   (static_cast<usize>(bx) < proto.getConstantCount() || fail("constant index out of range", pc));
        case OpCode::GETTABLE:
            return regA() && regB() && rkC();
        case OpCode::SETGLOBAL:
            return regA() &&
                   (static_cast<usize>(bx) < proto.getConstantCount() || fail("constant index out of range", pc));
        case OpCode::SETUPVAL:
            return regA() && (static_cast<usize>(b) < proto.getNumUpvalues() || fail("upvalue index out of range", pc));
        case OpCode::SETTABLE:
            return regA() && rkB() && rkC();
        case OpCode::NEWTABLE:
            return regA();
        case OpCode::SELF:
            return verifyRange(a, 2, maxStack, pc) && regB() && rkC();
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::POW:
            return regA() && rkB() && rkC();
        case OpCode::UNM:
        case OpCode::NOT:
        case OpCode::LEN:
            return regA() && regB();
        case OpCode::CONCAT:
            return regA() && b <= c && verifyRange(b, c - b + 1, maxStack, pc);
        case OpCode::JMP:
            return jump();
        case OpCode::EQ:
        case OpCode::LT:
        case OpCode::LE:
            return a <= 1 && rkB() && rkC() && verifyTarget(static_cast<i64>(pc) + 2, dataWords, pc);
        case OpCode::TEST:
            return regA() && c <= 1 && verifyJumpAt(proto, pc + 1, dataWords, pc);
        case OpCode::TESTSET:
            return regA() && regB() && c <= 1 && verifyJumpAt(proto, pc + 1, dataWords, pc);
        case OpCode::CALL:
            return regA() && (b == 0 || verifyRange(a, b, maxStack, pc)) &&
                   (c == 0 || c == 1 || verifyRange(a, c - 1, maxStack, pc));
        case OpCode::TAILCALL:
            return regA() && (b == 0 || verifyRange(a, b, maxStack, pc));
        case OpCode::RETURN:
            return regA() && (b == 0 || b == 1 || verifyRange(a, b - 1, maxStack, pc));
        case OpCode::FORLOOP:
        case OpCode::FORPREP:
            return verifyRange(a, 4, maxStack, pc) && jump();
        case OpCode::TFORLOOP:
            return c > 0 && verifyRange(a, c + 3, maxStack, pc) && verifyJumpAt(proto, pc + 1, dataWords, pc);
        case OpCode::SETLIST:
            return regA() && (b == 0 || verifyRange(a + 1, b, maxStack, pc));
        case OpCode::CLOSE:
            return regA();
        case OpCode::CLOSURE:
            return regA() && static_cast<usize>(bx) < proto.getSubProtoCount();
        case OpCode::VARARG:
            // Open VARARG results are allowed to begin exactly at maxStack.
            // The VM grows the physical stack before copying those dynamic
            // results, and SETLIST/CALL consume them through the open top.
            // Fixed results must still fit in the declared register frame.
            return proto.isVararg() &&
                   ((b == 0 || b == 1) ? verifyRange(a, 0, maxStack, pc) : verifyRange(a, b - 1, maxStack, pc));
        }
        return fail("unknown opcode", pc);
    }
};

} // namespace

std::expected<void, Str> BytecodeVerifier::verify(const Proto& root, const BytecodeVerifierLimits& limits) {
    return Verifier(limits).run(root);
}

} // namespace Lua
