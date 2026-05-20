#include "bytecode_printer.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
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
    std::ostringstream out;

    if (value.isNil()) {
        out << "nil";
    } else if (value.isBoolean()) {
        out << "boolean " << (value.asBoolean() ? "true" : "false");
    } else if (value.isNumber()) {
        out << "number " << value.asNumber();
    } else if (value.isString()) {
        out << "string \"" << escapeString(value.asString() ? value.asString()->c_str() : "") << "\"";
    } else if (value.isLightUserdata()) {
        out << "lightuserdata";
    } else if (value.isTable()) {
        out << "table";
    } else if (value.isFunction()) {
        out << "function";
    } else if (value.isUserdata()) {
        out << "userdata";
    } else if (value.isThread()) {
        out << "thread";
    } else {
        out << "unknown";
    }

    return out.str();
}

std::string formatConstant(const Proto* proto, i32 index) {
    std::ostringstream out;

    out << "K[" << index << "]";
    if (!proto || index < 0 || static_cast<usize>(index) >= proto->getConstantCount()) {
        out << " = <out of range>";
        return out.str();
    }

    out << " = " << formatValue(proto->getConstant(static_cast<usize>(index)));
    return out.str();
}

void addRKComment(std::vector<std::string>& comments,
                  const Proto* proto,
                  const char* name,
                  i32 operand,
                  OpArgMask mode) {
    if (mode != OpArgMask::OpArgK || !ISK(operand)) {
        return;
    }

    std::ostringstream out;
    out << name << "=" << formatConstant(proto, INDEXK(operand));
    comments.push_back(out.str());
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

} // namespace

void printProtoBytecode(const Proto* f, std::ostream& out, bool full) {
    (void)full;

    if (!f) {
        out << "Proto <null>" << '\n';
        return;
    }

    out << "Proto" << '\n';
    out << "  source: " << (f->getSource() ? f->getSource()->c_str() : "?") << '\n';
    out << "  linedefined: " << f->getLineDefined() << '\n';
    out << "  lastlinedefined: " << f->getLastLineDefined() << '\n';
    out << "  numparams: " << static_cast<int>(f->getNumParams()) << '\n';
    out << "  is_vararg: " << (f->isVararg() ? "true" : "false") << '\n';
    out << "  maxStackSize: " << static_cast<int>(f->getMaxStackSize()) << '\n';

    out << "  upvalues (" << f->getUpvalueNameCount() << "): ";
    if (f->getUpvalueNameCount() == 0) {
        out << "(none)";
    } else {
        for (usize i = 0; i < f->getUpvalueNameCount(); ++i) {
            if (i != 0) {
                out << ", ";
            }
            GCString* name = f->getUpvalueName(i);
            out << (name ? name->c_str() : "?");
        }
    }
    out << '\n';

    out << "instructions (" << f->getInstructionCount() << ")" << '\n';
    for (usize pc = 0; pc < f->getInstructionCount(); ++pc) {
        Instruction inst = f->getInstruction(pc);
        OpCode op = GET_OPCODE(inst);
        OpMode mode = getOpMode(op);
        std::vector<std::string> comments;

        out << std::setw(4) << std::setfill('0') << pc << std::setfill(' ')
            << " | line " << f->getLine(pc)
            << " | " << getOpName(op) << " | ";

        switch (mode) {
        case OpMode::iABC: {
            i32 a = GETARG_A(inst);
            i32 b = GETARG_B(inst);
            i32 c = GETARG_C(inst);
            out << "A=" << a << " B=" << b << " C=" << c;
            addRKComment(comments, f, "B", b, getBMode(op));
            addRKComment(comments, f, "C", c, getCMode(op));
            break;
        }
        case OpMode::iABx: {
            i32 a = GETARG_A(inst);
            i32 bx = GETARG_Bx(inst);
            out << "A=" << a << " Bx=" << bx;
            if (getBMode(op) == OpArgMask::OpArgK) {
                comments.push_back(formatConstant(f, bx));
            }
            break;
        }
        case OpMode::iAsBx: {
            i32 a = GETARG_A(inst);
            i32 sbx = GETARG_sBx(inst);
            out << "A=" << a << " sBx=" << sbx;

            std::ostringstream target;
            target << "target=" << (static_cast<i32>(pc) + 1 + sbx);
            comments.push_back(target.str());
            break;
        }
        }

        printComments(out, comments);
        out << '\n';
    }

    out << "constants (" << f->getConstantCount() << ")" << '\n';
    for (usize i = 0; i < f->getConstantCount(); ++i) {
        out << "  " << formatConstant(f, static_cast<i32>(i)) << '\n';
    }
}

} // namespace Lua
