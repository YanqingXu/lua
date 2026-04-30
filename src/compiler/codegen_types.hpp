#pragma once

#include "common/types.hpp"

namespace Lua {

constexpr i32 NO_JUMP = -1;

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

}  // namespace Lua
