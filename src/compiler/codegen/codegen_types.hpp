#pragma once

#include "common/types.hpp"

namespace Lua {

constexpr i32 NO_JUMP = -1;

template <typename... Visitors>
struct ValueResultVisitor : Visitors... {
    using Visitors::operator()...;
};

template <typename... Visitors>
ValueResultVisitor(Visitors...) -> ValueResultVisitor<Visitors...>;

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

    struct None {};

    struct Immediate {
        ImmediateKind kind = ImmediateKind::None;
        bool boolValue = false;
        f64 numberValue = 0.0;
    };

    struct ConstantRef {
        i32 constIndex = -1;
    };

    struct RegisterRef {
        i32 reg = -1;
        bool ownsRegister = false;
        AccessKind access = AccessKind::None;
    };

    struct PendingLoad {
        AccessKind access = AccessKind::None;
        i32 reg = -1;
        i32 constIndex = -1;
        i32 aux = -1;
    };

    struct Relocatable {
        i32 instructionPc = NO_JUMP;
    };

    struct MultiRet {
        AccessKind access = AccessKind::None;
        i32 reg = -1;
        i32 instructionPc = NO_JUMP;
    };

    struct PendingJump {
        i32 instructionPc = NO_JUMP;
    };

    using Variant = std::variant<None, Immediate, ConstantRef, RegisterRef, PendingLoad, Relocatable,
                                 MultiRet, PendingJump>;

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

    [[nodiscard]] const Variant& payload() const noexcept {
        return payload_;
    }

    [[nodiscard]] Variant& payload() noexcept {
        return payload_;
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), payload_);
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), payload_);
    }

    static ValueResult makeNil() {
        ValueResult result;
        result.setPayload(Immediate{ImmediateKind::Nil});
        return result;
    }

    static ValueResult makeBoolean(bool value) {
        ValueResult result;
        result.setPayload(Immediate{ImmediateKind::Boolean, value, 0.0});
        return result;
    }

    static ValueResult makeNumber(f64 value) {
        ValueResult result;
        result.setPayload(Immediate{ImmediateKind::Number, false, value});
        return result;
    }

    static ValueResult makeConstant(i32 index) {
        ValueResult result;
        result.setPayload(ConstantRef{index});
        return result;
    }

    static ValueResult makeRegister(i32 index, bool owns, AccessKind accessKind = AccessKind::None) {
        ValueResult result;
        result.setPayload(RegisterRef{index, owns, accessKind});
        return result;
    }

    static ValueResult makePendingLoad(AccessKind accessKind, i32 sourceReg = -1, i32 constantIndex = -1,
                                       i32 auxIndex = -1) {
        ValueResult result;
        result.setPayload(PendingLoad{accessKind, sourceReg, constantIndex, auxIndex});
        return result;
    }

    static ValueResult makeRelocatable(i32 pc) {
        ValueResult result;
        result.setPayload(Relocatable{pc});
        return result;
    }

    static ValueResult makeMultiRet(AccessKind accessKind, i32 baseReg, i32 pc) {
        ValueResult result;
        result.setPayload(MultiRet{accessKind, baseReg, pc});
        return result;
    }

    static ValueResult makePendingJump(i32 pc) {
        ValueResult result;
        result.setPayload(PendingJump{pc});
        return result;
    }

    void setPayload(Variant value) {
        payload_ = std::move(value);
        syncLegacyFieldsFromPayload();
    }

private:
    Variant payload_ = None{};

    void resetLegacyFields() noexcept {
        kind = Kind::None;
        immediate = ImmediateKind::None;
        access = AccessKind::None;
        reg = -1;
        constIndex = -1;
        aux = -1;
        instructionPc = NO_JUMP;
        boolValue = false;
        numberValue = 0.0;
        ownsRegister = false;
        isMultiResult = false;
        isSingleValue = true;
    }

    void syncLegacyFieldsFromPayload() {
        resetLegacyFields();

        struct Visitor {
            ValueResult& result;

            void operator()(const None&) const noexcept {}

            void operator()(const Immediate& value) const noexcept {
                result.kind = Kind::Immediate;
                result.immediate = value.kind;
                result.boolValue = value.boolValue;
                result.numberValue = value.numberValue;
            }

            void operator()(const ConstantRef& value) const noexcept {
                result.kind = Kind::Constant;
                result.constIndex = value.constIndex;
            }

            void operator()(const RegisterRef& value) const noexcept {
                result.kind = Kind::Register;
                result.access = value.access;
                result.reg = value.reg;
                result.ownsRegister = value.ownsRegister;
            }

            void operator()(const PendingLoad& value) const noexcept {
                result.kind = Kind::PendingLoad;
                result.access = value.access;
                result.reg = value.reg;
                result.constIndex = value.constIndex;
                result.aux = value.aux;
            }

            void operator()(const Relocatable& value) const noexcept {
                result.kind = Kind::Relocatable;
                result.instructionPc = value.instructionPc;
            }

            void operator()(const MultiRet& value) const noexcept {
                result.kind = Kind::MultiRet;
                result.access = value.access;
                result.reg = value.reg;
                result.instructionPc = value.instructionPc;
                result.isMultiResult = true;
                result.isSingleValue = false;
            }

            void operator()(const PendingJump& value) const noexcept {
                result.kind = Kind::PendingJump;
                result.instructionPc = value.instructionPc;
            }
        };

        std::visit(Visitor{*this}, payload_);
    }
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

}  // namespace Lua
