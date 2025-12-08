/**
 * @file bytecode_printer.cpp
 * @brief Lua Proto 字节码打印器实现
 *
 * 参考 lua_c_analysis/src/print.c 的 PrintFunction/PrintCode/PrintHeader 实现
 */

#include "compiler/bytecode_printer.hpp"

#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "core/gc_string.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>

namespace Lua {
namespace {

// 辅助函数：复数形式
inline const char* plural(int n) { return n == 1 ? "" : "s"; }

// 打印单个常量值（类似 lua_c_analysis/src/print.c 的 PrintConstant）
void printConstant(const Proto* f, int index, std::ostream& out) {
    Value v = f->getConstant(static_cast<usize>(index));

    if (v.isNil()) {
        out << "nil";
    } else if (v.isBoolean()) {
        out << (v.asBoolean() ? "true" : "false");
    } else if (v.isNumber()) {
        // 打印数字，尽量与 Lua C 版本格式一致
        double num = v.asNumber();
        // 检查是否为整数
        if (num == static_cast<double>(static_cast<long long>(num))) {
            out << static_cast<long long>(num);
        } else {
            out << num;
        }
    } else if (v.isString()) {
        // 打印字符串，用双引号包围
        GCString* str = v.asString();
        out << "\"" << str->c_str() << "\"";
    } else {
        // 其他类型（表、函数等）
        out << v.toString();
    }
}

// 打印函数头部信息（类似 lua_c_analysis/src/print.c 的 PrintHeader）
void printHeader(const Proto* f, std::ostream& out) {
    const char* src = f->getSource() ? f->getSource()->c_str() : "(unknown)";
    const char* s = src;
    if (*s == '@' || *s == '=') ++s;

    int sizecode = static_cast<int>(f->getInstructionCount());
    out << "\n"
        << (f->getLineDefined() == 0 ? "main" : "function")
        << " <" << s << ":" << f->getLineDefined()
        << "," << f->getLastLineDefined() << "> ("
        << sizecode << " instruction" << plural(sizecode)
        << ", " << sizecode * static_cast<int>(sizeof(Instruction))
        << " bytes at " << static_cast<const void*>(f) << ")\n";

    int nparams = static_cast<int>(f->getNumParams());
    int nslots  = static_cast<int>(f->getMaxStackSize());
    int nups    = static_cast<int>(f->getNumUpvalues());
    int nloc    = static_cast<int>(f->getLocVarCount());
    int nk      = static_cast<int>(f->getConstantCount());
    int nfunc   = static_cast<int>(f->getSubProtoCount());

    out << nparams << (f->isVararg() ? "+" : "") << " param" << plural(nparams)
        << ", " << nslots << " slot" << plural(nslots)
        << ", " << nups << " upvalue" << plural(nups)
        << ", " << nloc << " local" << plural(nloc)
        << ", " << nk << " constant" << plural(nk)
        << ", " << nfunc << " function" << plural(nfunc) << "\n";
}

// 打印字节码指令（类似 lua_c_analysis/src/print.c 的 PrintCode）
void printCode(const Proto* f, std::ostream& out) {
    int n = static_cast<int>(f->getInstructionCount());
    const auto& code = f->getCode();

    for (int pc = 0; pc < n; pc++) {
        Instruction i = code[pc];
        OpCode o = GET_OPCODE(i);
        int a = GETARG_A(i);
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        int bx = GETARG_Bx(i);
        int sbx = GETARG_sBx(i);
        int line = f->getLine(static_cast<usize>(pc));

        // 打印指令索引和行号
        out << "\t" << (pc + 1) << "\t";
        if (line > 0) {
            out << "[" << line << "]\t";
        } else {
            out << "[-]\t";
        }

        // 打印操作码名称
        out << std::left << std::setw(9) << getOpName(o) << "\t";

        // 打印参数（根据指令格式）
        switch (getOpMode(o)) {
        case OpMode::iABC:
            out << a;
            if (getBMode(o) != OpArgMask::OpArgN) {
                out << " " << (ISK(b) ? (-1 - INDEXK(b)) : b);
            }
            if (getCMode(o) != OpArgMask::OpArgN) {
                out << " " << (ISK(c) ? (-1 - INDEXK(c)) : c);
            }
            break;
        case OpMode::iABx:
            if (getBMode(o) == OpArgMask::OpArgK) {
                out << a << " " << (-1 - bx);
            } else {
                out << a << " " << bx;
            }
            break;
        case OpMode::iAsBx:
            if (o == OpCode::JMP) {
                out << sbx;
            } else {
                out << a << " " << sbx;
            }
            break;
        }

        // 打印注释（常量值、跳转目标等）
        switch (o) {
        case OpCode::LOADK:
            out << "\t; ";
            printConstant(f, bx, out);
            break;
        case OpCode::GETUPVAL:
        case OpCode::SETUPVAL:
            if (f->getUpvalueNameCount() > 0 && b < static_cast<int>(f->getUpvalueNameCount())) {
                out << "\t; " << f->getUpvalueName(static_cast<usize>(b))->c_str();
            }
            break;
        case OpCode::GETGLOBAL:
        case OpCode::SETGLOBAL:
            // C 实现使用 svalue() 直接打印字符串值（不带引号）
            {
                Value v = f->getConstant(static_cast<usize>(bx));
                if (v.isString()) {
                    out << "\t; " << v.asString()->c_str();
                } else {
                    out << "\t; ";
                    printConstant(f, bx, out);
                }
            }
            break;
        case OpCode::GETTABLE:
        case OpCode::SELF:
            if (ISK(c)) {
                out << "\t; ";
                printConstant(f, INDEXK(c), out);
            }
            break;

        case OpCode::SETTABLE:
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::POW:
        case OpCode::EQ:
        case OpCode::LT:
        case OpCode::LE:
            if (ISK(b) || ISK(c)) {
                out << "\t; ";
                if (ISK(b)) {
                    printConstant(f, INDEXK(b), out);
                } else {
                    out << "-";
                }
                out << " ";
                if (ISK(c)) {
                    printConstant(f, INDEXK(c), out);
                } else {
                    out << "-";
                }
            }
            break;
        case OpCode::JMP:
        case OpCode::FORLOOP:
        case OpCode::FORPREP:
            out << "\t; to " << (sbx + pc + 2);
            break;
        case OpCode::CLOSURE:
            out << "\t; " << static_cast<const void*>(f->getSubProto(static_cast<usize>(bx)));
            break;
        case OpCode::SETLIST:
            // C 实现: if (c==0) printf("\t; %d",(int)code[++pc]); else printf("\t; %d",c);
            if (c == 0) {
                // 下一条指令存储实际的列表大小
                if (static_cast<usize>(pc + 1) < f->getInstructionCount()) {
                    out << "\t; " << static_cast<int>(code[pc + 1]);
                }
            } else {
                out << "\t; " << c;
            }
            break;
        default:
            break;
        }

        out << "\n";
    }
}

// 打印常量表（full 模式）
void printConstants(const Proto* f, std::ostream& out) {
    int n = static_cast<int>(f->getConstantCount());
    out << "constants (" << n << ") for " << static_cast<const void*>(f) << ":\n";
    for (int i = 0; i < n; i++) {
        out << "\t" << (i + 1) << "\t";
        printConstant(f, i, out);
        out << "\n";
    }
}

// 打印局部变量表（full 模式）
void printLocals(const Proto* f, std::ostream& out) {
    int n = static_cast<int>(f->getLocVarCount());
    out << "locals (" << n << ") for " << static_cast<const void*>(f) << ":\n";
    for (int i = 0; i < n; i++) {
        const auto& locvar = f->getLocVar(static_cast<usize>(i));
        out << "\t" << i << "\t" << locvar.varname->c_str()
            << "\t" << (locvar.startpc + 1)
            << "\t" << (locvar.endpc + 1) << "\n";
    }
}

// 打印上值表（full 模式）
void printUpvalues(const Proto* f, std::ostream& out) {
    int n = static_cast<int>(f->getUpvalueNameCount());
    out << "upvalues (" << n << ") for " << static_cast<const void*>(f) << ":\n";
    for (int i = 0; i < n; i++) {
        out << "\t" << i << "\t" << f->getUpvalueName(static_cast<usize>(i))->c_str() << "\n";
    }
}

} // anonymous namespace

// 主函数：打印 Proto 字节码（类似 lua_c_analysis/src/print.c 的 PrintFunction）
void printProtoBytecode(const Proto* f, std::ostream& out, bool full) {
    printHeader(f, out);
    printCode(f, out);

    if (full) {
        printConstants(f, out);
        printLocals(f, out);
        printUpvalues(f, out);
    }

    // 递归打印所有子函数
    int n = static_cast<int>(f->getSubProtoCount());
    for (int i = 0; i < n; i++) {
        printProtoBytecode(f->getSubProto(static_cast<usize>(i)), out, full);
    }
}

} // namespace Lua

