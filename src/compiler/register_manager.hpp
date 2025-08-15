#pragma once

#include "../common/types.hpp"
#include <vector>
#include <string>

namespace Lua {

/**
 * Lua 5.1官方寄存器分配器
 * 
 * 基于Lua 5.1源码的寄存器分配策略：
 * 1. 寄存器从0开始编号（0-based indexing）
 * 2. 局部变量占用固定的低位寄存器槽位
 * 3. 临时值使用栈顶的寄存器
 * 4. 函数调用需要连续的寄存器块
 * 5. 支持嵌套作用域和寄存器回收
 */
class RegisterManager {
public:
    static constexpr int MAX_REGISTERS = 250;  // Lua 5.1的寄存器限制
    
    RegisterManager();
    ~RegisterManager() = default;
    
    // === 局部变量寄存器管理 ===
    
    /**
     * 为局部变量分配固定寄存器槽位
     * @param name 变量名（用于调试）
     * @return 分配的寄存器编号
     */
    int allocateLocal(const std::string& name = "");
    
    /**
     * 获取局部变量数量
     */
    int getLocalCount() const { return localCount_; }
    
    // === 临时值寄存器管理 ===
    
    /**
     * 分配临时寄存器（在栈顶）
     * @param name 用途描述（用于调试）
     * @return 分配的寄存器编号
     */
    int allocateTemp(const std::string& name = "");
    
    /**
     * 释放最后分配的临时寄存器
     */
    void freeTemp();
    
    /**
     * 获取当前栈顶寄存器编号
     */
    int getStackTop() const { return stackTop_; }
    
    // === 函数调用寄存器管理 ===
    
    /**
     * 为函数调用分配连续的寄存器块
     * @param count 需要的寄存器数量（函数 + 参数）
     * @param name 调用描述（用于调试）
     * @return 起始寄存器编号
     */
    int allocateCallFrame(int count, const std::string& name = "");
    
    /**
     * 释放函数调用的寄存器块
     * @param startReg 起始寄存器编号
     * @param count 寄存器数量
     */
    void freeCallFrame(int startReg, int count);
    
    // === 作用域管理 ===
    
    /**
     * 进入新的作用域
     */
    void enterScope();
    
    /**
     * 退出当前作用域，回收作用域内的局部变量寄存器
     */
    void exitScope();
    
    /**
     * 获取当前作用域深度
     */
    int getScopeDepth() const { return static_cast<int>(scopeStack_.size()); }
    
    // === 寄存器生命周期管理 ===

    /**
     * 标记寄存器为活跃状态（在跳转指令中需要保持有效）
     * @param reg 寄存器编号
     * @param reason 活跃原因（用于调试）
     */
    void markRegisterLive(int reg, const std::string& reason = "");

    /**
     * 取消寄存器的活跃状态
     * @param reg 寄存器编号
     */
    void unmarkRegisterLive(int reg);

    /**
     * 检查寄存器是否处于活跃状态
     * @param reg 寄存器编号
     * @return 是否活跃
     */
    bool isRegisterLive(int reg) const;

    /**
     * 获取所有活跃寄存器的列表
     */
    std::vector<int> getLiveRegisters() const;

    // === 寄存器状态查询 ===

    /**
     * 检查寄存器是否可用（考虑活跃性）
     */
    bool isRegisterAvailable(int reg) const;

    /**
     * 获取已使用的寄存器数量
     */
    int getUsedRegisterCount() const { return stackTop_; }

    /**
     * 重置分配器（用于新函数编译）
     */
    void reset();
    
    // === 调试支持 ===
    
    /**
     * 打印当前寄存器分配状态
     */
    void printStatus() const;
    
    /**
     * 验证寄存器分配的一致性
     */
    bool validate() const;

private:
    struct ScopeInfo {
        int localCount;     // 该作用域开始时的局部变量数量
        int stackTop;       // 该作用域开始时的栈顶位置
    };
    
    int localCount_;                    // 局部变量数量
    int stackTop_;                      // 当前栈顶位置（下一个可分配的寄存器）
    std::vector<ScopeInfo> scopeStack_; // 作用域栈

    // 寄存器活跃性跟踪
    std::vector<bool> liveRegisters_;   // 寄存器活跃状态
    std::vector<std::string> liveReasons_; // 活跃原因（调试用）

    // 调试信息
    std::vector<std::string> registerNames_;  // 寄存器用途描述
    
    /**
     * 检查寄存器编号是否有效
     */
    bool isValidRegister(int reg) const {
        return reg >= 0 && reg < MAX_REGISTERS;
    }
    
    /**
     * 更新栈顶位置
     */
    void updateStackTop(int newTop) {
        if (newTop > stackTop_) {
            stackTop_ = newTop;
            // 扩展调试信息数组
            while (registerNames_.size() < static_cast<size_t>(stackTop_)) {
                registerNames_.push_back("");
            }
        }
    }

    /**
     * 查找可用的寄存器（避免活跃寄存器）
     * @return 可用寄存器编号，如果没有则返回-1
     */
    int findAvailableRegister() const;
};

} // namespace Lua
