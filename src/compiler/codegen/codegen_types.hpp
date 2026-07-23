#pragma once

/**
 * @file codegen_types.hpp
 * @brief 代码生成器使用的绑定、值和控制流通道类型
 */

#include "common/types.hpp"

namespace Lua {

constexpr i32 NO_JUMP = -1;

/** @brief 将多个可调用对象组合为 std::visit 使用的访问器。 */
template <typename... Visitors> struct ValueResultVisitor : Visitors... {
    using Visitors::operator()...;
};

template <typename... Visitors> ValueResultVisitor(Visitors...) -> ValueResultVisitor<Visitors...>;

/** @brief 保存等待回填的指令位置列表。 */
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

/** @brief 条件表达式的真假跳转列表及可选常量结果。 */
struct CondResult {
    PatchList trueList;
    PatchList falseList;
    bool knownConstant = false;
    bool constantValue = false;
};

/** @brief 表达式降级后的右值通道。 */
struct ValueResult {
    /** @brief 右值载荷类型。 */
    enum class Kind { None, Immediate, Constant, Register, PendingLoad, Relocatable, MultiRet, PendingJump };

    /** @brief 立即数的具体类型。 */
    enum class ImmediateKind { None, Nil, Boolean, Number };

    /** @brief 右值的来源或访问方式。 */
    enum class AccessKind { None, Local, Upvalue, Global, Indexed, Call, Vararg };

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

    using Variant =
        std::variant<None, Immediate, ConstantRef, RegisterRef, PendingLoad, Relocatable, MultiRet, PendingJump>;

    ValueResult() = default;

    [[nodiscard]] const Variant& payload() const noexcept {
        return payload_;
    }

    template <typename Visitor> decltype(auto) visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), payload_);
    }

    template <typename Visitor> decltype(auto) visit(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), payload_);
    }

    static ValueResult makeNil() {
        return ValueResult(Immediate{ImmediateKind::Nil});
    }

    static ValueResult makeBoolean(bool value) {
        return ValueResult(Immediate{ImmediateKind::Boolean, value, 0.0});
    }

    static ValueResult makeNumber(f64 value) {
        return ValueResult(Immediate{ImmediateKind::Number, false, value});
    }

    static ValueResult makeConstant(i32 index) {
        return ValueResult(ConstantRef{index});
    }

    static ValueResult makeRegister(i32 index, bool owns, AccessKind accessKind = AccessKind::None) {
        return ValueResult(RegisterRef{index, owns, accessKind});
    }

    static ValueResult makePendingLoad(AccessKind accessKind, i32 sourceReg = -1, i32 constantIndex = -1,
                                       i32 auxIndex = -1) {
        return ValueResult(PendingLoad{accessKind, sourceReg, constantIndex, auxIndex});
    }

    static ValueResult makeRelocatable(i32 pc) {
        return ValueResult(Relocatable{pc});
    }

    static ValueResult makeMultiRet(AccessKind accessKind, i32 baseReg, i32 pc) {
        return ValueResult(MultiRet{accessKind, baseReg, pc});
    }

    static ValueResult makePendingJump(i32 pc) {
        return ValueResult(PendingJump{pc});
    }

private:
    Variant payload_ = None{};

    explicit ValueResult(Variant value) : payload_(std::move(value)) {}
};

/** @brief 赋值目标的局部、上值、全局或索引引用。 */
struct LValueRef {
    enum class Kind { None, Local, Upvalue, Global, Indexed };

    Kind kind = Kind::None;
    i32 slot = -1;
    i32 tableReg = -1;
    i32 key = -1;
    i32 aux = -1;

    bool valid() const noexcept {
        return kind != Kind::None;
    }
};

/** @brief 调用或可变参数产生的多返回值通道。 */
struct CallResultInfo {
    enum class Kind { None, Call, Vararg };

    Kind kind = Kind::None;
    i32 baseReg = -1;
    i32 instructionPc = NO_JUMP;
    bool openMultiRet = false;

    bool valid() const noexcept {
        return kind != Kind::None;
    }
};

// =============================================================================
/** @brief 符号引用——名称绑定结果（第 8 次拉取请求）。 */
// =============================================================================

/**
 * @brief 名字解析结果
 *
 * 将名称表达式中的名称解析为局部变量、上值或全局变量三种绑定结果。
 * 从代码生成器的局部变量查找、上值解析和全局变量回退流程中提取，
 * 消除值发射、左值发射、名称表达式发射和函数语句路径中重复的查找逻辑。
 */
struct SymbolRef {
    enum class Kind { None, Local, Upvalue, Global };

    Kind kind = Kind::None;
    i32 index = -1; // 局部变量：寄存器槽位；上值：上值索引；全局变量：字符串常量索引
    Str name;       // 原始名称（用于全局变量场景或调试）

    bool valid() const noexcept {
        return kind != Kind::None;
    }
};

} // namespace Lua
