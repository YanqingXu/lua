#include "register_file.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include <algorithm>
#include <iostream>

namespace Lua {
    
    RegisterFile::RegisterFile(LuaState* L) : L_(L) {
        if (!L_) {
            throw LuaException("LuaState cannot be null");
        }
    }
    
    // Register access operations
    Value& RegisterFile::get(i32 reg) {
        validateRegister(reg, "get");
        return L_->get(getStackPos(reg));
    }
    
    const Value& RegisterFile::get(i32 reg) const {
        validateRegister(reg, "get");
        return L_->get(getStackPos(reg));
    }
    
    void RegisterFile::set(i32 reg, const Value& val) {
        validateRegister(reg, "set");
        L_->set(getStackPos(reg), val);
    }
    
    Value* RegisterFile::getPtr(i32 reg) {
        validateRegister(reg, "getPtr");
        return L_->index2addr(getStackPos(reg));
    }
    
    // Register range operations
    void RegisterFile::move(i32 dest, i32 src) {
        validateRegister(dest, "move dest");
        validateRegister(src, "move src");
        
        const Value& srcVal = get(src);
        set(dest, srcVal);
    }
    
    void RegisterFile::moveRange(i32 destStart, i32 srcStart, i32 count) {
        if (count <= 0) {
            return;
        }
        
        // Validate all registers in range
        for (i32 i = 0; i < count; i++) {
            validateRegister(destStart + i, "moveRange dest");
            validateRegister(srcStart + i, "moveRange src");
        }
        
        // Handle overlapping ranges by copying in appropriate direction
        if (destStart < srcStart) {
            // Copy forward
            for (i32 i = 0; i < count; i++) {
                set(destStart + i, get(srcStart + i));
            }
        } else {
            // Copy backward
            for (i32 i = count - 1; i >= 0; i--) {
                set(destStart + i, get(srcStart + i));
            }
        }
    }
    
    void RegisterFile::fillNil(i32 start, i32 count) {
        if (count <= 0) {
            return;
        }
        
        Value nilValue; // Default constructor creates nil
        for (i32 i = 0; i < count; i++) {
            validateRegister(start + i, "fillNil");
            set(start + i, nilValue);
        }
    }
    
    // Register validation and bounds checking
    bool RegisterFile::isValidRegister(i32 reg) const {
        if (reg < 0 || reg >= MAX_REGISTERS_PER_FUNCTION) {
            return false;
        }
        
        i32 stackPos = getStackPos(reg);
        return L_->index2addr(stackPos) != nullptr;
    }
    
    i32 RegisterFile::getBase() const {
        CallInfo* ci = L_->getCurrentCI();
        if (!ci || !ci->base) {
            return 0;
        }
        return static_cast<i32>(ci->base - L_->index2addr(0));
    }
    
    i32 RegisterFile::getTop() const {
        return L_->getTop();
    }
    
    void RegisterFile::setTop(i32 top) {
        L_->setTop(top);
    }
    
    // Type checking operations
    bool RegisterFile::checkType(i32 reg, ValueType expectedType) const {
        if (!isValidRegister(reg)) {
            return false;
        }
        
        const Value& val = get(reg);
        return val.type() == expectedType;
    }
    
    void RegisterFile::ensureType(i32 reg, ValueType expectedType, const Str& errorMsg) const {
        if (!checkType(reg, expectedType)) {
            throw LuaException(errorMsg.c_str());
        }
    }
    
    // Constant access (for instruction execution)
    Value RegisterFile::getConstant(u32 idx) const {
        // TODO: Get constant from current function
        // For now, return nil
        return Value();
    }
    
    bool RegisterFile::isConstant(u32 operand) {
        return (operand & REGISTER_CONSTANT_FLAG) != 0;
    }
    
    Value RegisterFile::getRegisterOrConstant(u32 operand) {
        if (isConstant(operand)) {
            // Remove K flag and get constant
            u32 constIdx = operand & ~REGISTER_CONSTANT_FLAG;
            return getConstant(constIdx);
        } else {
            // Direct register access
            return get(static_cast<i32>(operand));
        }
    }
    
    // Debug and diagnostic operations
    void RegisterFile::printRegisters(i32 start, i32 count) const {
        std::cout << "=== Register File State ===" << std::endl;
        std::cout << "Base: " << getBase() << ", Top: " << getTop() << std::endl;
        
        for (i32 i = start; i < start + count; i++) {
            if (isValidRegister(i)) {
                const Value& val = get(i);
                std::cout << "R[" << i << "] = ";
                
                switch (val.type()) {
                    case ValueType::Nil:
                        std::cout << "nil";
                        break;
                    case ValueType::Boolean:
                        std::cout << (val.asBoolean() ? "true" : "false");
                        break;
                    case ValueType::Number:
                        std::cout << val.asNumber();
                        break;
                    case ValueType::String:
                        std::cout << "\"" << val.asString() << "\"";
                        break;
                    default:
                        std::cout << "<" << static_cast<i32>(val.type()) << ">";
                        break;
                }
                std::cout << std::endl;
            } else {
                std::cout << "R[" << i << "] = <invalid>" << std::endl;
            }
        }
        std::cout << "=========================" << std::endl;
    }
    
    bool RegisterFile::validate() const {
        if (!L_) {
            return false;
        }
        
        // Check that base and top are reasonable
        i32 base = getBase();
        i32 top = getTop();
        
        if (base < 0 || top < base) {
            return false;
        }
        
        // Check that we can access registers within reasonable bounds
        for (i32 i = 0; i < std::min(top - base, 10); i++) {
            if (!isValidRegister(i)) {
                return false;
            }
        }
        
        return true;
    }
    
    // Static utility functions
    i32 RegisterFile::regToStackPos(LuaState* L, i32 reg) {
        if (!L) {
            return -1;
        }
        
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->base) {
            return reg; // Fallback to direct indexing
        }
        
        i32 base = static_cast<i32>(ci->base - L->index2addr(0));
        return base + reg;
    }
    
    i32 RegisterFile::stackPosToReg(LuaState* L, i32 stackPos) {
        if (!L) {
            return -1;
        }
        
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->base) {
            return stackPos; // Fallback to direct indexing
        }
        
        i32 base = static_cast<i32>(ci->base - L->index2addr(0));
        return stackPos - base;
    }
    
    // Private helper methods
    i32 RegisterFile::getStackPos(i32 reg) const {
        return regToStackPos(L_, reg);
    }
    
    void RegisterFile::validateRegister(i32 reg, const Str& operation) const {
        if (reg < 0) {
            throw LuaException(("Invalid register index for " + operation + ": " + std::to_string(reg) + " (negative)").c_str());
        }
        
        if (reg >= MAX_REGISTERS_PER_FUNCTION) {
            throw LuaException(("Invalid register index for " + operation + ": " + std::to_string(reg) + " (too large)").c_str());
        }
        
        i32 stackPos = getStackPos(reg);
        if (!L_->index2addr(stackPos)) {
            throw LuaException(("Invalid register access for " + operation + ": R[" + std::to_string(reg) + "] -> stack[" + std::to_string(stackPos) + "]").c_str());
        }
    }
    
} // namespace Lua
