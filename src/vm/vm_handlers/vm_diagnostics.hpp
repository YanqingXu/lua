#pragma once

#include "common/types.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"

namespace Lua::VM::handlers::diagnostics {

inline const char* luaTypeName(const Value& value) {
    switch (value.getType()) {
    case ValueType::Nil:
        return "nil";
    case ValueType::Boolean:
        return "boolean";
    case ValueType::LightUserdata:
        return "userdata";
    case ValueType::Number:
        return "number";
    case ValueType::String:
        return "string";
    case ValueType::Table:
        return "table";
    case ValueType::Function:
        return "function";
    case ValueType::Userdata:
        return "userdata";
    case ValueType::Thread:
        return "thread";
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
    return Str(value.asString()->getData());
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
        if (local.reg != reg || local.startpc > currentPc || currentPc >= local.endpc || local.varname == nullptr) {
            continue;
        }

        Str name(local.varname->getData());
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

    const auto code = proto->getInstructionSpan();
    auto hasRecentGuardForUse = [&](i32 guardedReg, usize usePc) {
        usize scanPc = std::min(usePc, code.size());
        i32 inspected = 0;
        while (scanPc > 0 && inspected < 8) {
            --scanPc;
            ++inspected;
            Instruction inst = code[scanPc];
            OpCode op = GET_OPCODE(inst);
            if ((op == OpCode::TEST || op == OpCode::TESTSET) && GETARG_A(inst) == guardedReg) {
                return true;
            }
        }
        return false;
    };

    auto hasGuardAfterSetter = [&](usize setterPc, i32 guardedReg, usize usePc) {
        usize guardEnd = std::min(usePc, code.size());
        for (usize guardPc = setterPc + 1; guardPc < guardEnd; ++guardPc) {
            Instruction guard = code[guardPc];
            OpCode guardOp = GET_OPCODE(guard);
            if ((guardOp == OpCode::TEST || guardOp == OpCode::TESTSET) && GETARG_A(guard) == guardedReg) {
                return true;
            }
        }
        return false;
    };

    i32 currentReg = reg;
    usize currentPc = pc;

    for (i32 remainingDepth = 4 - depth; remainingDepth >= 0; --remainingDepth) {
        if (hasRecentGuardForUse(currentReg, currentPc)) {
            return std::nullopt;
        }

        if (Opt<Str> local = localNameForRegister(proto, currentReg, currentPc)) {
            return Str("local '") + *local + "'";
        }

        usize scanPc = currentPc;
        bool followMove = false;

        while (scanPc > 0) {
            --scanPc;
            Instruction inst = code[scanPc];
            OpCode op = GET_OPCODE(inst);
            i32 a = GETARG_A(inst);
            if (a != currentReg) {
                continue;
            }

            switch (op) {
            case OpCode::GETGLOBAL:
                if (hasGuardAfterSetter(scanPc, currentReg, currentPc)) {
                    return std::nullopt;
                }
                if (Opt<Str> name = constantString(proto, GETARG_Bx(inst))) {
                    return Str("global '") + *name + "'";
                }
                return std::nullopt;
            case OpCode::GETUPVAL: {
                if (hasGuardAfterSetter(scanPc, currentReg, currentPc)) {
                    return std::nullopt;
                }
                GCString* name = proto->getUpvalueName(static_cast<usize>(GETARG_B(inst)));
                if (name) {
                    Str description("upvalue '");
                    description.append(name->getData());
                    description.push_back('\'');
                    return description;
                }
                return std::nullopt;
            }
            case OpCode::GETTABLE:
                if (hasGuardAfterSetter(scanPc, currentReg, currentPc)) {
                    return std::nullopt;
                }
                if (Opt<Str> key = rkString(proto, GETARG_C(inst))) {
                    return Str("field '") + *key + "'";
                }
                return std::nullopt;
            case OpCode::SELF:
                if (hasGuardAfterSetter(scanPc, currentReg, currentPc)) {
                    return std::nullopt;
                }
                if (Opt<Str> key = rkString(proto, GETARG_C(inst))) {
                    return Str("method '") + *key + "'";
                }
                return std::nullopt;
            case OpCode::MOVE: {
                i32 sourceReg = GETARG_B(inst);
                if (Opt<Str> local = localNameForRegister(proto, sourceReg, scanPc)) {
                    return Str("local '") + *local + "'";
                }

                usize previousPc = scanPc;
                while (previousPc > 0) {
                    --previousPc;
                    Instruction previous = code[previousPc];
                    OpCode previousOp = GET_OPCODE(previous);
                    if (GETARG_A(previous) == sourceReg &&
                        (previousOp == OpCode::GETGLOBAL || previousOp == OpCode::GETUPVAL ||
                         previousOp == OpCode::GETTABLE || previousOp == OpCode::SELF)) {
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

                        currentReg = sourceReg;
                        currentPc = scanPc;
                        followMove = true;
                        break;
                    }
                    if (previousOp != OpCode::MOVE) {
                        break;
                    }
                }

                if (followMove) {
                    break;
                }
                return std::nullopt;
            }
            default:
                return std::nullopt;
            }

            if (followMove) {
                break;
            }
        }

        if (!followMove) {
            return std::nullopt;
        }

        if (currentPc == 0) {
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
