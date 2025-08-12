#pragma once

#include "../common/types.hpp"
#include "value.hpp"
#include "lua_state.hpp"

namespace Lua {
    
    /**
     * @brief Register file abstraction for VM execution
     * 
     * This class provides a unified interface for register operations in the Lua VM,
     * following the Lua 5.1 register-based execution model. It abstracts the
     * underlying stack-based implementation and provides type-safe register access.
     */
    class RegisterFile {
    public:
        /**
         * @brief Construct a new Register File object
         * @param L Lua state to operate on
         */
        explicit RegisterFile(LuaState* L);
        
        /**
         * @brief Destroy the Register File object
         */
        ~RegisterFile() = default;
        
        // Register access operations
        /**
         * @brief Get value from register
         * @param reg Register index (0-based)
         * @return Value& Reference to register value
         * @throws LuaException if register index is invalid
         */
        Value& get(i32 reg);
        
        /**
         * @brief Get value from register (const version)
         * @param reg Register index (0-based)
         * @return const Value& Const reference to register value
         * @throws LuaException if register index is invalid
         */
        const Value& get(i32 reg) const;
        
        /**
         * @brief Set value in register
         * @param reg Register index (0-based)
         * @param val Value to set
         * @throws LuaException if register index is invalid
         */
        void set(i32 reg, const Value& val);
        
        /**
         * @brief Get pointer to register value
         * @param reg Register index (0-based)
         * @return Value* Pointer to register value (for upvalue access)
         * @throws LuaException if register index is invalid
         */
        Value* getPtr(i32 reg);
        
        // Register range operations
        /**
         * @brief Copy value between registers
         * @param dest Destination register
         * @param src Source register
         */
        void move(i32 dest, i32 src);
        
        /**
         * @brief Copy multiple values between register ranges
         * @param destStart Destination start register
         * @param srcStart Source start register
         * @param count Number of registers to copy
         */
        void moveRange(i32 destStart, i32 srcStart, i32 count);
        
        /**
         * @brief Fill register range with nil values
         * @param start Start register
         * @param count Number of registers to fill
         */
        void fillNil(i32 start, i32 count);
        
        // Register validation and bounds checking
        /**
         * @brief Check if register index is valid
         * @param reg Register index to check
         * @return bool True if register is valid
         */
        bool isValidRegister(i32 reg) const;
        
        /**
         * @brief Get current register base (for current function)
         * @return i32 Current register base offset
         */
        i32 getBase() const;
        
        /**
         * @brief Get current register top
         * @return i32 Current register top
         */
        i32 getTop() const;
        
        /**
         * @brief Set register top
         * @param top New register top
         */
        void setTop(i32 top);
        
        // Type checking operations
        /**
         * @brief Check if register contains specific type
         * @param reg Register index
         * @param expectedType Expected value type
         * @return bool True if register contains expected type
         */
        bool checkType(i32 reg, ValueType expectedType) const;
        
        /**
         * @brief Ensure register contains specific type
         * @param reg Register index
         * @param expectedType Expected value type
         * @param errorMsg Error message if type check fails
         * @throws LuaException if type check fails
         */
        void ensureType(i32 reg, ValueType expectedType, const Str& errorMsg = "type error") const;
        
        // Constant access (for instruction execution)
        /**
         * @brief Get constant value by index
         * @param idx Constant index
         * @return Value Constant value
         * @throws LuaException if constant index is invalid
         */
        Value getConstant(u32 idx) const;
        
        /**
         * @brief Check if operand is constant (K flag set)
         * @param operand Instruction operand
         * @return bool True if operand represents a constant
         */
        static bool isConstant(u32 operand);
        
        /**
         * @brief Get register or constant value based on operand
         * @param operand Instruction operand (register or constant index with K flag)
         * @return Value Register or constant value
         */
        Value getRegisterOrConstant(u32 operand);
        
        // Debug and diagnostic operations
        /**
         * @brief Print current register state (debug only)
         * @param start Start register to print
         * @param count Number of registers to print
         */
        void printRegisters(i32 start = 0, i32 count = 10) const;
        
        /**
         * @brief Validate register file consistency
         * @return bool True if register file is in valid state
         */
        bool validate() const;
        
        // Static utility functions
        /**
         * @brief Convert relative register index to absolute stack position
         * @param L Lua state
         * @param reg Relative register index
         * @return i32 Absolute stack position
         */
        static i32 regToStackPos(LuaState* L, i32 reg);
        
        /**
         * @brief Convert absolute stack position to relative register index
         * @param L Lua state
         * @param stackPos Absolute stack position
         * @return i32 Relative register index
         */
        static i32 stackPosToReg(LuaState* L, i32 stackPos);
        
    private:
        LuaState* L_;                    // Lua state reference
        
        // Helper methods
        /**
         * @brief Get absolute stack position for register
         * @param reg Register index
         * @return i32 Absolute stack position
         */
        i32 getStackPos(i32 reg) const;
        
        /**
         * @brief Validate register index and throw exception if invalid
         * @param reg Register index to validate
         * @param operation Operation name for error message
         */
        void validateRegister(i32 reg, const Str& operation) const;
        
        // Prevent copying
        RegisterFile(const RegisterFile&) = delete;
        RegisterFile& operator=(const RegisterFile&) = delete;
    };
    
    // Constants for register operations
    constexpr u32 REGISTER_CONSTANT_FLAG = 0x100;  // K flag for constants (bit 8)
    constexpr i32 MAX_REGISTERS_PER_FUNCTION = 250; // Lua 5.1 limit
    
} // namespace Lua
