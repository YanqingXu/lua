#include "register_manager.hpp"
#include "../common/defines.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace Lua {

RegisterManager::RegisterManager() 
    : localCount_(0)
    , stackTop_(0) {
}

// === 局部变量寄存器管理 ===

int RegisterManager::allocateLocal(const std::string& name) {
    if (localCount_ >= MAX_REGISTERS) {
        throw LuaException("Too many local variables (max " + std::to_string(MAX_REGISTERS) + ")");
    }
    
    int reg = localCount_++;
    
    // 更新栈顶位置
    updateStackTop(localCount_);
    
    // 记录调试信息
    if (reg < static_cast<int>(registerNames_.size())) {
        registerNames_[reg] = "local:" + (name.empty() ? "unnamed" : name);
    }
    
    return reg;
}

// === 临时值寄存器管理 ===

int RegisterManager::allocateTemp(const std::string& name) {
    if (stackTop_ >= MAX_REGISTERS) {
        throw LuaException("Register stack overflow (max " + std::to_string(MAX_REGISTERS) + ")");
    }

    // 智能寄存器分配：避免覆盖活跃寄存器
    int reg = findAvailableRegister();
    if (reg == -1) {
        // 如果没有可用寄存器，使用栈顶
        reg = stackTop_;
    }

    // 更新栈顶位置
    if (reg >= stackTop_) {
        updateStackTop(reg + 1);
    }

#if DEBUG_REGISTER_ALLOCATION
    DEBUG_PRINT("allocateTemp: " << name << " reg=" << reg << " newStackTop=" << stackTop_);
#endif

    // 记录调试信息
    if (reg < static_cast<int>(registerNames_.size())) {
        registerNames_[reg] = "temp:" + (name.empty() ? "expr" : name);
    }

    return reg;
}

void RegisterManager::freeTemp() {
    if (stackTop_ > localCount_) {
        stackTop_--;
        
        // 清除调试信息
        if (stackTop_ < static_cast<int>(registerNames_.size())) {
            registerNames_[stackTop_] = "";
        }
    }
}

// === 函数调用寄存器管理 ===

int RegisterManager::allocateCallFrame(int count, const std::string& name) {
    if (stackTop_ + count > MAX_REGISTERS) {
        throw LuaException("Not enough registers for function call (need " +
                          std::to_string(count) + ", available " +
                          std::to_string(MAX_REGISTERS - stackTop_) + ")");
    }

    int startReg = stackTop_;
    updateStackTop(stackTop_ + count);

#if DEBUG_REGISTER_ALLOCATION
    DEBUG_PRINT("allocateCallFrame: " << name << " count=" << count << " startReg=" << startReg << " newStackTop=" << stackTop_);
#endif

    // 记录调试信息
    for (int i = 0; i < count; i++) {
        int reg = startReg + i;
        if (reg < static_cast<int>(registerNames_.size())) {
            if (i == 0) {
                registerNames_[reg] = "call:" + (name.empty() ? "func" : name);
            } else {
                registerNames_[reg] = "call:arg" + std::to_string(i - 1);
            }
        }
    }

    return startReg;
}

void RegisterManager::freeCallFrame(int startReg, int count) {
    // 验证参数
    if (startReg < 0 || count <= 0 || startReg + count > stackTop_) {
        return; // 无效的释放请求
    }
    
    // 只有当这是栈顶的调用帧时才能释放
    if (startReg + count == stackTop_) {
        stackTop_ = startReg;
        
        // 清除调试信息
        for (int i = startReg; i < startReg + count; i++) {
            if (i < static_cast<int>(registerNames_.size())) {
                registerNames_[i] = "";
            }
        }
    }
}

// === 作用域管理 ===

void RegisterManager::enterScope() {
    ScopeInfo scope;
    scope.localCount = localCount_;
    scope.stackTop = stackTop_;
    scopeStack_.push_back(scope);
}

void RegisterManager::exitScope() {
    if (scopeStack_.empty()) {
        return; // 没有作用域可退出
    }
    
    ScopeInfo scope = scopeStack_.back();
    scopeStack_.pop_back();
    
    // 回收作用域内分配的局部变量
    for (int i = scope.localCount; i < localCount_; i++) {
        if (i < static_cast<int>(registerNames_.size())) {
            registerNames_[i] = "";
        }
    }
    
    localCount_ = scope.localCount;
    
    // 回收临时寄存器（但不能低于局部变量数量）
    stackTop_ = std::max(scope.stackTop, localCount_);
    
    // 清除超出栈顶的调试信息
    for (int i = stackTop_; i < static_cast<int>(registerNames_.size()); i++) {
        registerNames_[i] = "";
    }
}

// === 寄存器生命周期管理 ===

void RegisterManager::markRegisterLive(int reg, const std::string& reason) {
    if (!isValidRegister(reg)) {
        return;
    }

    // 扩展活跃性数组
    while (liveRegisters_.size() <= static_cast<size_t>(reg)) {
        liveRegisters_.push_back(false);
        liveReasons_.push_back("");
    }

    liveRegisters_[reg] = true;
    liveReasons_[reg] = reason;

#if DEBUG_REGISTER_ALLOCATION
    DEBUG_PRINT("markRegisterLive: R" << reg << " reason=" << reason);
#endif
}

void RegisterManager::unmarkRegisterLive(int reg) {
    if (!isValidRegister(reg) || reg >= static_cast<int>(liveRegisters_.size())) {
        return;
    }

    liveRegisters_[reg] = false;
    liveReasons_[reg] = "";

#if DEBUG_REGISTER_ALLOCATION
    DEBUG_PRINT("unmarkRegisterLive: R" << reg);
#endif
}

bool RegisterManager::isRegisterLive(int reg) const {
    if (!isValidRegister(reg) || reg >= static_cast<int>(liveRegisters_.size())) {
        return false;
    }
    return liveRegisters_[reg];
}

std::vector<int> RegisterManager::getLiveRegisters() const {
    std::vector<int> result;
    for (int i = 0; i < static_cast<int>(liveRegisters_.size()); i++) {
        if (liveRegisters_[i]) {
            result.push_back(i);
        }
    }
    return result;
}

// === 寄存器状态查询 ===

bool RegisterManager::isRegisterAvailable(int reg) const {
    // 寄存器可用的条件：
    // 1. 寄存器编号有效
    // 2. 寄存器在栈顶之上（未分配）
    // 3. 寄存器不处于活跃状态
    return isValidRegister(reg) && reg >= stackTop_ && !isRegisterLive(reg);
}

void RegisterManager::reset() {
    localCount_ = 0;
    stackTop_ = 0;
    scopeStack_.clear();
    registerNames_.clear();
    liveRegisters_.clear();
    liveReasons_.clear();
}

void RegisterManager::resetToStackTop(int newStackTop) {
    if (newStackTop < localCount_) {
        // 不能重置到局部变量区域以下
        newStackTop = localCount_;
    }

    if (newStackTop < stackTop_) {
        // 清除被释放寄存器的调试信息
        for (int i = newStackTop; i < stackTop_; i++) {
            if (i < static_cast<int>(registerNames_.size())) {
                registerNames_[i] = "";
            }
            if (i < static_cast<int>(liveRegisters_.size())) {
                liveRegisters_[i] = false;
                liveReasons_[i] = "";
            }
        }

        stackTop_ = newStackTop;
    }
}

// === 调试支持 ===

void RegisterManager::printStatus() const {
    std::cout << "=== RegisterManager Status ===" << std::endl;
    std::cout << "Local count: " << localCount_ << std::endl;
    std::cout << "Stack top: " << stackTop_ << std::endl;
    std::cout << "Scope depth: " << scopeStack_.size() << std::endl;
    std::cout << "Used registers: " << stackTop_ << "/" << MAX_REGISTERS << std::endl;

    // 显示活跃寄存器
    auto liveRegs = getLiveRegisters();
    if (!liveRegs.empty()) {
        std::cout << "Live registers: ";
        for (int reg : liveRegs) {
            std::cout << "R" << reg;
            if (reg < static_cast<int>(liveReasons_.size()) && !liveReasons_[reg].empty()) {
                std::cout << "(" << liveReasons_[reg] << ")";
            }
            std::cout << " ";
        }
        std::cout << std::endl;
    }

    if (!registerNames_.empty()) {
        std::cout << "Register allocation:" << std::endl;
        for (int i = 0; i < std::min(stackTop_, static_cast<int>(registerNames_.size())); i++) {
            std::cout << "  R" << std::setw(3) << i << ": "
                      << (registerNames_[i].empty() ? "(free)" : registerNames_[i]);
            if (isRegisterLive(i)) {
                std::cout << " [LIVE]";
            }
            std::cout << std::endl;
        }
    }
    std::cout << "==============================" << std::endl;
}

bool RegisterManager::validate() const {
    // 检查基本不变量
    if (localCount_ < 0 || stackTop_ < 0) {
        return false;
    }
    
    if (localCount_ > stackTop_) {
        return false; // 局部变量数量不能超过栈顶
    }
    
    if (stackTop_ > MAX_REGISTERS) {
        return false; // 不能超过最大寄存器数量
    }
    
    // 检查作用域栈的一致性
    for (const auto& scope : scopeStack_) {
        if (scope.localCount < 0 || scope.stackTop < 0) {
            return false;
        }
        if (scope.localCount > localCount_ || scope.stackTop > stackTop_) {
            return false; // 作用域信息不能超过当前状态
        }
    }
    
    return true;
}

int RegisterManager::findAvailableRegister() const {
    // 从栈顶开始查找第一个不活跃的寄存器
    for (int reg = stackTop_; reg < MAX_REGISTERS; reg++) {
        if (!isRegisterLive(reg)) {
            return reg;
        }
    }

    // 如果栈顶之上没有可用寄存器，检查栈顶之下的临时寄存器
    // （但不能使用局部变量寄存器）
    for (int reg = localCount_; reg < stackTop_; reg++) {
        if (!isRegisterLive(reg)) {
            // 检查这个寄存器是否真的可以重用
            // 这里可以添加更复杂的活跃性分析
            return reg;
        }
    }

    return -1; // 没有可用寄存器
}

} // namespace Lua
