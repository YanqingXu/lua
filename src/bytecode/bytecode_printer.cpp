#include "bytecode_printer.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"

#include <algorithm>
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace Lua {

namespace {

std::string escapeString(const char* value) {
    std::string out;
    for (const char* p = value ? value : ""; *p; ++p) {
        switch (*p) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += *p;
            break;
        }
    }
    return out;
}

std::string formatValue(const Value& value) {
    if (value.isNil()) {
        return "nil";
    } else if (value.isBoolean()) {
        return std::format("boolean {}", value.asBoolean() ? "true" : "false");
    } else if (value.isNumber()) {
        return std::format("number {}", value.asNumber());
    } else if (value.isString()) {
        return std::format("string \"{}\"", escapeString(value.asString() ? value.asString()->c_str() : ""));
    } else if (value.isLightUserdata()) {
        return "lightuserdata";
    } else if (value.isTable()) {
        return "table";
    } else if (value.isFunction()) {
        return "function";
    } else if (value.isUserdata()) {
        return "userdata";
    } else if (value.isThread()) {
        return "thread";
    } else {
        return "unknown";
    }
}

std::string formatConstant(const Proto* proto, i32 index) {
    std::string prefix = std::format("K[{}]", index);
    if (!proto || index < 0 || static_cast<usize>(index) >= proto->getConstantCount()) {
        return std::format("{} = <out of range>", prefix);
    }

    return std::format("{} = {}", prefix, formatValue(proto->getConstant(static_cast<usize>(index))));
}

void addRKComment(std::vector<std::string>& comments,
                  const Proto* proto,
                  const char* name,
                  i32 operand,
                  OpArgMask mode) {
    if (mode != OpArgMask::OpArgK || !ISK(operand)) {
        return;
    }

    comments.push_back(std::format("{}={}", name, formatConstant(proto, INDEXK(operand))));
}

void printComments(std::ostream& out, const std::vector<std::string>& comments) {
    if (comments.empty()) {
        return;
    }

    out << " ; ";
    for (usize i = 0; i < comments.size(); ++i) {
        if (i != 0) {
            out << "; ";
        }
        out << comments[i];
    }
}

void printProtoHeader(const Proto* f, std::ostream& out) {
    out << "Proto" << '\n';
    out << "  source: " << (f->getSource() ? f->getSource()->c_str() : "?") << '\n';
    out << "  linedefined: " << f->getLineDefined() << '\n';
    out << "  lastlinedefined: " << f->getLastLineDefined() << '\n';
    out << "  numparams: " << static_cast<int>(f->getNumParams()) << '\n';
    out << std::format("  is_vararg: {} (flags={})\n",
                       f->isVararg() ? "true" : "false",
                       static_cast<int>(f->getVarargFlags()));
    out << "  maxStackSize: " << static_cast<int>(f->getMaxStackSize()) << '\n';

    const usize upvalueCount =
        std::max(static_cast<usize>(f->getNumUpvalues()), f->getUpvalueNameCount());
    out << "  upvalues (" << upvalueCount << "): ";
    if (upvalueCount == 0) {
        out << "(none)";
    } else {
        for (usize i = 0; i < upvalueCount; ++i) {
            if (i != 0) {
                out << ", ";
            }
            GCString* name = f->getUpvalueName(i);
            out << (name ? name->c_str() : "?");
        }
    }
    out << '\n';
}

void printInstruction(const Proto* f, usize pc, Instruction inst, std::ostream& out) {
    const OpCode op = GET_OPCODE(inst);
    const OpcodeMetadata& metadata = opcodeMetadata(op);
    std::vector<std::string> comments;

    out << std::format("{:04} | line {} | {} | ", pc, f->getLine(pc), metadata.name);

    switch (metadata.mode) {
    case OpMode::iABC: {
        const i32 a = GETARG_A(inst);
        const i32 b = GETARG_B(inst);
        const i32 c = GETARG_C(inst);
        out << "A=" << a << " B=" << b << " C=" << c;
        addRKComment(comments, f, "B", b, metadata.bMode);
        addRKComment(comments, f, "C", c, metadata.cMode);
        break;
    }
    case OpMode::iABx: {
        const i32 a = GETARG_A(inst);
        const i32 bx = GETARG_Bx(inst);
        out << "A=" << a << " Bx=" << bx;
        if (metadata.bMode == OpArgMask::OpArgK) {
            comments.push_back(formatConstant(f, bx));
        }
        break;
    }
    case OpMode::iAsBx: {
        const i32 a = GETARG_A(inst);
        const i32 sbx = GETARG_sBx(inst);
        out << "A=" << a << " sBx=" << sbx;
        comments.push_back(std::format("target={}", static_cast<i32>(pc) + 1 + sbx));
        break;
    }
    }

    printComments(out, comments);
    out << '\n';
}

void printInstructions(const Proto* f, std::ostream& out) {
    const auto code = f->getInstructionSpan();
    out << "instructions (" << code.size() << ")" << '\n';
    for (usize pc = 0; pc < code.size(); ++pc) {
        printInstruction(f, pc, code[pc], out);
    }
}

void printConstants(const Proto* f, std::ostream& out) {
    out << "constants (" << f->getConstantCount() << ")" << '\n';
    for (usize i = 0; i < f->getConstantCount(); ++i) {
        out << "  " << formatConstant(f, static_cast<i32>(i)) << '\n';
    }
}

} // namespace

void printProtoBytecode(const Proto* f, std::ostream& out, bool full) {
    (void)full;

    if (!f) {
        out << "Proto <null>" << '\n';
        return;
    }

    printProtoHeader(f, out);
    printInstructions(f, out);
    printConstants(f, out);
}

} // namespace Lua
