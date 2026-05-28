#pragma once

#include "common/types.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"

namespace Lua::VM::handlers::diagnostics {

inline const char* luaTypeName(const Value& value) {
    switch (value.getType()) {
        case ValueType::Nil: return "nil";
        case ValueType::Boolean: return "boolean";
        case ValueType::LightUserdata: return "userdata";
        case ValueType::Number: return "number";
        case ValueType::String: return "string";
        case ValueType::Table: return "table";
        case ValueType::Function: return "function";
        case ValueType::Userdata: return "userdata";
        case ValueType::Thread: return "thread";
    }
    return "value";
}

inline Opt<Str> constantString(Proto* proto, i32 index) {
    if (!proto || index < 0 || static_cast<usize>(index) >= proto->getConstantCount()) {
        return std::nullopt;
    }
    Value value = proto->getConstant(static_cast<usize>(index));
    if (!value.isString()) {
        return std::nullopt;
    }
    return value.asString()->getData();
}

inline Opt<Str> rkString(Proto* proto, i32 rk) {
    if (!ISK(rk)) {
        return std::nullopt;
    }
    return constantString(proto, INDEXK(rk));
}

inline Opt<Str> localNameForRegister(Proto* proto, i32 reg, usize pc) {
    if (!proto) {
        return std::nullopt;
    }

    i32 currentPc = static_cast<i32>(pc);
    for (usize i = 0; i < proto->getLocVarCount(); ++i) {
        const LocVar& local = proto->getLocVar(i);
        if (local.reg != reg || local.startpc > currentPc || currentPc >= local.endpc ||
            local.varname == nullptr) {
            continue;
        }

        Str name = local.varname->getData();
        if (!name.empty() && name.front() != '(') {
            return name;
        }
    }

    return std::nullopt;
}

inline Opt<Str> describeRegister(Proto* proto, i32 reg, usize pc, i32 depth = 0) {
    if (!proto || depth > 4) {
        return std::nullopt;
    }

    if (Opt<Str> local = localNameForRegister(proto, reg, pc)) {
        return Str("local '") + *local + "'";
    }

    const auto code = proto->getInstructionSpan();
    usize scanPc = pc;
    while (scanPc > 0) {
        --scanPc;
        Instruction inst = code[scanPc];
        OpCode op = GET_OPCODE(inst);
        i32 a = GETARG_A(inst);
        if (a != reg) {
            continue;
        }

        switch (op) {
            case OpCode::GETGLOBAL:
                if (Opt<Str> name = constantString(proto, GETARG_Bx(inst))) {
                    return Str("global '") + *name + "'";
                }
                return std::nullopt;
            case OpCode::GETUPVAL: {
                GCString* name = proto->getUpvalueName(static_cast<usize>(GETARG_B(inst)));
                if (name) {
                    return Str("upvalue '") + name->getData() + "'";
                }
                return std::nullopt;
            }
            case OpCode::GETTABLE:
                if (Opt<Str> key = rkString(proto, GETARG_C(inst))) {
                    return Str("field '") + *key + "'";
                }
                return std::nullopt;
            case OpCode::SELF:
                if (Opt<Str> key = rkString(proto, GETARG_C(inst))) {
                    return Str("method '") + *key + "'";
                }
                return std::nullopt;
            case OpCode::MOVE: {
                i32 sourceReg = GETARG_B(inst);
                if (Opt<Str> local = localNameForRegister(proto, GETARG_B(inst), scanPc)) {
                    return Str("local '") + *local + "'";
                }

                usize previousPc = scanPc;
                while (previousPc > 0) {
                    --previousPc;
                    Instruction previous = code[previousPc];
                    OpCode previousOp = GET_OPCODE(previous);
                    if (GETARG_A(previous) == sourceReg &&
                        (previousOp == OpCode::GETGLOBAL ||
                         previousOp == OpCode::GETUPVAL ||
                         previousOp == OpCode::GETTABLE ||
                         previousOp == OpCode::SELF)) {
                        usize guardPc = previousPc;
                        usize guardLimit = previousPc > 4 ? previousPc - 4 : 0;
                        while (guardPc > guardLimit) {
                            --guardPc;
                            Instruction guard = code[guardPc];
                            OpCode guardOp = GET_OPCODE(guard);
                            if ((guardOp == OpCode::TEST || guardOp == OpCode::TESTSET) &&
                                GETARG_A(guard) == sourceReg) {
                                return std::nullopt;
                            }
                        }
                        return describeRegister(proto, sourceReg, scanPc, depth + 1);
                    }
                    if (previousOp != OpCode::MOVE) {
                        break;
                    }
                }
                return std::nullopt;
            }
            default:
                return std::nullopt;
        }
    }

    return std::nullopt;
}

inline Str formatTypeActionError(StrView action, const Value& value, const Str& sourceName) {
    const char* typeName = luaTypeName(value);
    if (!sourceName.empty()) {
        return Str("attempt to ") + Str(action) + " " + sourceName + " (a " + typeName + " value)";
    }
    return Str("attempt to ") + Str(action) + " a " + typeName + " value";
}

} // namespace Lua::VM::handlers::diagnostics
