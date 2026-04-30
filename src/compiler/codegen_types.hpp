#pragma once

#include "common/types.hpp"

namespace Lua {

constexpr i32 NO_JUMP = -1;

enum class ExprKind {
    Void,
    Nil,
    True,
    False,
    Const,
    Number,
    NonRelocatable,
    Local,
    Upval,
    Global,
    Indexed,
    Jump,
    Relocatable,
    Call,
    Vararg
};

struct ExprDesc {
    struct SlotInfo {
        i32 info;
        i32 aux;
    };

    ExprKind kind;

    union ValueInfo {
        SlotInfo s;
        f64 nval;

        ValueInfo() : s{0, 0} {}
    } u;

    i32 t;
    i32 f;

    ExprDesc() : kind(ExprKind::Void), t(NO_JUMP), f(NO_JUMP) {}
};

struct PatchList {
    Vec<i32> pcs;

    bool empty() const noexcept {
        return pcs.empty();
    }

    usize size() const noexcept {
        return pcs.size();
    }

    i32 front() const noexcept {
        return pcs.empty() ? NO_JUMP : pcs.front();
    }

    void clear() noexcept {
        pcs.clear();
    }

    void append(i32 pc) {
        if (pc != NO_JUMP) {
            pcs.push_back(pc);
        }
    }

    void append(const PatchList& other) {
        pcs.insert(pcs.end(), other.pcs.begin(), other.pcs.end());
    }

    static PatchList merge(PatchList lhs, const PatchList& rhs) {
        lhs.append(rhs);
        return lhs;
    }
};

struct CondResult {
    PatchList trueList;
    PatchList falseList;
    bool knownConstant = false;
    bool constantValue = false;
};

struct ValueResult {
    enum class Kind {
        None,
        Immediate,
        Constant,
        Register,
        PendingLoad,
        Relocatable,
        MultiRet,
        PendingJump
    };

    enum class ImmediateKind {
        None,
        Nil,
        Boolean,
        Number
    };

    enum class AccessKind {
        None,
        Local,
        Upvalue,
        Global,
        Indexed,
        Call,
        Vararg
    };

    Kind kind = Kind::None;
    ImmediateKind immediate = ImmediateKind::None;
    AccessKind access = AccessKind::None;
    i32 reg = -1;
    i32 constIndex = -1;
    i32 aux = -1;
    i32 instructionPc = NO_JUMP;
    bool boolValue = false;
    f64 numberValue = 0.0;
    bool ownsRegister = false;
    bool isMultiResult = false;
    bool isSingleValue = true;
};

struct LValueRef {
    enum class Kind {
        None,
        Local,
        Upvalue,
        Global,
        Indexed
    };

    Kind kind = Kind::None;
    i32 slot = -1;
    i32 tableReg = -1;
    i32 key = -1;
    i32 aux = -1;

    bool valid() const noexcept {
        return kind != Kind::None;
    }
};

struct CallResultInfo {
    enum class Kind {
        None,
        Call,
        Vararg
    };

    Kind kind = Kind::None;
    i32 baseReg = -1;
    i32 instructionPc = NO_JUMP;
    bool openMultiRet = false;

    bool valid() const noexcept {
        return kind != Kind::None;
    }
};

// =============================================================================
// SymbolRef — 名字绑定结果（PR-8 Symbol Binding）
// =============================================================================

/**
 * @brief 名字解析结果
 *
 * 将 NameExpr 中的名字解析为 Local / Upvalue / Global 三种绑定结果。
 * 从 CodeGenerator 的 findLocalVar / resolveUpvalue / global fallback 提取，
 * 消除 emitValue / emitLValue / emitExpr(NameExpr) / FunctionStmt 中重复的查找逻辑。
 */
struct SymbolRef {
    enum class Kind {
        None,
        Local,
        Upvalue,
        Global
    };

    Kind kind = Kind::None;
    i32 index = -1;       // Local: 寄存器槽位; Upvalue: upvalue 索引; Global: 字符串常量索引
    Str name;             // 原始名字（用于 Global 场景或调试）

    bool valid() const noexcept {
        return kind != Kind::None;
    }
};

inline ValueResult adaptLegacyExprDescValue(const ExprDesc& desc) {
    ValueResult result;

    switch (desc.kind) {
        case ExprKind::Nil:
            result.kind = ValueResult::Kind::Immediate;
            result.immediate = ValueResult::ImmediateKind::Nil;
            break;

        case ExprKind::True:
        case ExprKind::False:
            result.kind = ValueResult::Kind::Immediate;
            result.immediate = ValueResult::ImmediateKind::Boolean;
            result.boolValue = (desc.kind == ExprKind::True);
            break;

        case ExprKind::Number:
            result.kind = ValueResult::Kind::Immediate;
            result.immediate = ValueResult::ImmediateKind::Number;
            result.numberValue = desc.u.nval;
            break;

        case ExprKind::Const:
            result.kind = ValueResult::Kind::Constant;
            result.constIndex = desc.u.s.info;
            break;

        case ExprKind::Local:
            result.kind = ValueResult::Kind::Register;
            result.access = ValueResult::AccessKind::Local;
            result.reg = desc.u.s.info;
            break;

        case ExprKind::NonRelocatable:
            result.kind = ValueResult::Kind::Register;
            result.reg = desc.u.s.info;
            result.ownsRegister = true;
            break;

        case ExprKind::Upval:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Upvalue;
            result.aux = desc.u.s.info;
            break;

        case ExprKind::Global:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Global;
            result.constIndex = desc.u.s.info;
            break;

        case ExprKind::Indexed:
            result.kind = ValueResult::Kind::PendingLoad;
            result.access = ValueResult::AccessKind::Indexed;
            result.reg = desc.u.s.info;
            result.aux = desc.u.s.aux;
            break;

        case ExprKind::Relocatable:
            result.kind = ValueResult::Kind::Relocatable;
            result.instructionPc = desc.u.s.info;
            break;

        case ExprKind::Call:
            result.kind = ValueResult::Kind::MultiRet;
            result.access = ValueResult::AccessKind::Call;
            result.reg = desc.u.s.info;
            result.instructionPc = desc.u.s.aux;
            result.isMultiResult = true;
            result.isSingleValue = false;
            break;

        case ExprKind::Vararg:
            result.kind = ValueResult::Kind::MultiRet;
            result.access = ValueResult::AccessKind::Vararg;
            result.instructionPc = desc.u.s.info;
            result.isMultiResult = true;
            result.isSingleValue = false;
            break;

        case ExprKind::Jump:
            result.kind = ValueResult::Kind::PendingJump;
            result.instructionPc = desc.u.s.info;
            result.isSingleValue = false;
            break;

        case ExprKind::Void:
        default:
            break;
    }

    return result;
}

inline LValueRef adaptLegacyExprDescLValue(const ExprDesc& desc) {
    LValueRef result;

    switch (desc.kind) {
        case ExprKind::Local:
            result.kind = LValueRef::Kind::Local;
            result.slot = desc.u.s.info;
            break;

        case ExprKind::Upval:
            result.kind = LValueRef::Kind::Upvalue;
            result.slot = desc.u.s.info;
            break;

        case ExprKind::Global:
            result.kind = LValueRef::Kind::Global;
            result.slot = desc.u.s.info;
            break;

        case ExprKind::Indexed:
            result.kind = LValueRef::Kind::Indexed;
            result.tableReg = desc.u.s.info;
            result.key = desc.u.s.aux;
            break;

        case ExprKind::Void:
        case ExprKind::Nil:
        case ExprKind::True:
        case ExprKind::False:
        case ExprKind::Const:
        case ExprKind::Number:
        case ExprKind::NonRelocatable:
        case ExprKind::Jump:
        case ExprKind::Relocatable:
        case ExprKind::Call:
        case ExprKind::Vararg:
        default:
            break;
    }

    return result;
}

inline CallResultInfo adaptLegacyExprDescCall(const ExprDesc& desc) {
    CallResultInfo result;

    switch (desc.kind) {
        case ExprKind::Call:
            result.kind = CallResultInfo::Kind::Call;
            result.baseReg = desc.u.s.info;
            result.instructionPc = desc.u.s.aux;
            break;

        case ExprKind::Vararg:
            result.kind = CallResultInfo::Kind::Vararg;
            result.instructionPc = desc.u.s.info;
            break;

        case ExprKind::Void:
        case ExprKind::Nil:
        case ExprKind::True:
        case ExprKind::False:
        case ExprKind::Const:
        case ExprKind::Number:
        case ExprKind::NonRelocatable:
        case ExprKind::Local:
        case ExprKind::Upval:
        case ExprKind::Global:
        case ExprKind::Indexed:
        case ExprKind::Jump:
        case ExprKind::Relocatable:
        default:
            break;
    }

    return result;
}

// PR-6: ValueResult → ExprDesc 逆向适配（供 emitExpr 兼容壳使用）
inline void valueResultToExprDesc(const ValueResult& val, ExprDesc& desc) {
    desc.t = NO_JUMP;
    desc.f = NO_JUMP;
    switch (val.kind) {
        case ValueResult::Kind::Immediate:
            switch (val.immediate) {
                case ValueResult::ImmediateKind::Nil:
                    desc.kind = ExprKind::Nil; break;
                case ValueResult::ImmediateKind::Boolean:
                    desc.kind = val.boolValue ? ExprKind::True : ExprKind::False; break;
                case ValueResult::ImmediateKind::Number:
                    desc.kind = ExprKind::Number; desc.u.nval = val.numberValue; break;
                default:
                    desc.kind = ExprKind::Void; break;
            }
            break;
        case ValueResult::Kind::Constant:
            desc.kind = ExprKind::Const; desc.u.s.info = val.constIndex; break;
        case ValueResult::Kind::Register:
            if (val.access == ValueResult::AccessKind::Local) {
                desc.kind = ExprKind::Local; desc.u.s.info = val.reg;
            } else {
                desc.kind = ExprKind::NonRelocatable; desc.u.s.info = val.reg;
            }
            break;
        case ValueResult::Kind::PendingLoad:
            switch (val.access) {
                case ValueResult::AccessKind::Global:
                    desc.kind = ExprKind::Global; desc.u.s.info = val.constIndex; break;
                case ValueResult::AccessKind::Upvalue:
                    desc.kind = ExprKind::Upval; desc.u.s.info = val.aux; break;
                case ValueResult::AccessKind::Indexed:
                    desc.kind = ExprKind::Indexed; desc.u.s.info = val.reg; desc.u.s.aux = val.aux; break;
                default:
                    desc.kind = ExprKind::Void; break;
            }
            break;
        case ValueResult::Kind::Relocatable:
            desc.kind = ExprKind::Relocatable; desc.u.s.info = val.instructionPc; break;
        case ValueResult::Kind::MultiRet:
            if (val.access == ValueResult::AccessKind::Call) {
                desc.kind = ExprKind::Call; desc.u.s.info = val.reg; desc.u.s.aux = val.instructionPc;
            } else if (val.access == ValueResult::AccessKind::Vararg) {
                desc.kind = ExprKind::Vararg; desc.u.s.info = val.instructionPc;
            } else {
                desc.kind = ExprKind::Void;
            }
            break;
        case ValueResult::Kind::PendingJump:
            desc.kind = ExprKind::Jump; desc.u.s.info = val.instructionPc; break;
        default:
            desc.kind = ExprKind::Void; break;
    }
}

}  // namespace Lua
