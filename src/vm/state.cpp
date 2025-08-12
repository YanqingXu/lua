#include "state.hpp"
#include "global_state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "core_metamethods.hpp"
#include "metamethod_manager.hpp"
#include "../common/defines.hpp"
#include "../parser/parser.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/vm.hpp"
#include "../gc/core/garbage_collector.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

namespace Lua {
    State::State() : GCObject(GCObjectType::State, sizeof(State)), top(0),
                     globalState_(nullptr), useGlobalState_(false) {
        // Initialize stack space
        stack.resize(LUAI_MAXSTACK);

        // Note: VM is now static, no instance initialization needed
    }

    State::State(GlobalState* globalState) : GCObject(GCObjectType::State, sizeof(State)),
                                             top(0), globalState_(globalState), useGlobalState_(true) {
        // Initialize stack space
        stack.resize(LUAI_MAXSTACK);

        // TODO: Initialize with GlobalState-specific settings
        // This will be expanded in future iterations
    }

    State::~State() {
        // Clean up resources
    }

    void State::push(const Value& value) {
        if (top >= LUAI_MAXSTACK) {
            throw LuaException("stack overflow");
        }
        stack[top++] = value;
    }

    Value State::pop() {
        if (top <= 0) {
            throw LuaException("stack underflow");
        }
        return stack[--top];
    }

    Value& State::get(int idx) {
        static Value nil;  // Static nil value for invalid index returns

        // Handle absolute and relative indices
        int abs_idx;
        if (idx >= 0) {
            abs_idx = idx;  // Direct 0-based indexing
        } else {
            abs_idx = top + idx;  // Index relative to stack top
        }

        // Check if index is within range
        if (abs_idx < 0 || abs_idx >= top) {
            return nil;
        }

        return stack[abs_idx];
    }

    void State::set(int idx, const Value& value) {
        // Handle absolute and relative indices
        int abs_idx;
        if (idx >= 0) {
            abs_idx = idx;  // Direct 0-based indexing
        } else {
            abs_idx = top + idx;  // Index relative to stack top
        }

        // Automatically extend stack to accommodate new index
        if (abs_idx < 0) {
            return; // invalid
        }
        if (abs_idx >= top) {
            // Need to extend top to abs_idx+1
            if (abs_idx >= LUAI_MAXSTACK) {
                throw LuaException("stack overflow");
            }
            top = abs_idx + 1;
        }
        stack[abs_idx] = value;
    }

    // Unified stack pointer access: 0-based absolute index when idx >= 0,
    // negative idx means relative to current top (top + idx)
    Value* State::getPtr(int idx) {
        int abs_idx;
        if (idx >= 0) {
            abs_idx = idx;  // 0-based absolute indexing
        } else {
            abs_idx = top + idx;  // Index relative to stack top
        }
        if (abs_idx < 0 || abs_idx >= top) {
            return nullptr;
        }
        return &stack[abs_idx];
    }

    // Type checking functions
    bool State::isNil(int idx) const {
        if (idx <= 0 || idx > top) return true;
        return stack[idx - 1].isNil();
    }

    bool State::isBoolean(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isBoolean();
    }

    bool State::isNumber(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isNumber();
    }

    bool State::isString(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isString();
    }

    bool State::isTable(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isTable();
    }

    bool State::isFunction(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isFunction();
    }

    // Type conversion functions
    LuaBoolean State::toBoolean(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].asBoolean();
    }

    LuaNumber State::toNumber(int idx) const {
        if (idx <= 0 || idx > top) return 0.0;
        return stack[idx - 1].asNumber();
    }

    Str State::toString(int idx) const {
        if (idx <= 0 || idx > top) return "";
        return stack[idx - 1].toString();
    }

    GCRef<Table> State::toTable(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asTable();
    }

    GCRef<Function> State::toFunction(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asFunction();
    }

    // Global variable operations
    void State::setGlobal(const Str& name, const Value& value) {
        if (isUsingGlobalState()) {
            // Use GlobalState for global variable storage (Phase 1 refactoring)
            globalState_->setGlobal(name, value);
        } else {
            // Use local storage (backward compatibility)
            globals[name] = value;
        }
    }

    Value State::getGlobal(const Str& name) {
        if (isUsingGlobalState()) {
            // Use GlobalState for global variable retrieval (Phase 1 refactoring)
            return globalState_->getGlobal(name);
        } else {
            // Use local storage (backward compatibility)
            auto it = globals.find(name);
            if (it != globals.end()) {
                return it->second;
            }
            return Value(nullptr);  // nil
        }
    }

    // Function call (Lua 5.1官方设计)
    Value State::call(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Native function call
        if (function->getType() == Function::Type::Native) {
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("attempt to call a nil value");
                }

                // Push arguments onto stack for legacy function
                int oldTop = getTop();
                for (const Value& arg : args) {
                    push(arg);
                }

                // Legacy function call - return single value
                Value result = nativeFnLegacy(this, static_cast<int>(args.size()));

                // Restore stack top
                setTop(oldTop);

                return result;
            } else {
                // New multi-return function - call and return first value for compatibility
                CallResult callResult = callMultiple(func, args);
                if (callResult.count > 0) {
                    return callResult.getFirst();
                } else {
                    return Value();  // Return nil if no values
                }
            }
        }

        // For Lua function calls, use VM to execute
        try {
            // Save current state
            int oldTop = top;

            // Lua 5.1 calling convention:
            // Stack layout: [function] [arg1] [arg2] [arg3] ...
            // Register 0 = function, Register 1 = arg1, Register 2 = arg2, etc.

            // Push function onto stack first
            push(Value(function));

            // Push arguments onto stack
            for (const auto& arg : args) {
                push(arg);
            }

            // Execute Lua function with enhanced VM implementation
            try {
                // Get function code and constants
                const auto& code = function->getCode();
                const auto& constants = function->getConstants();

                // Simple register file (temporary implementation)
                Vec<Value> registers(256, Value()); // 256 registers initialized to nil

                // VM execution loop
                size_t pc = 0;
                while (pc < code.size()) {
                    Instruction instr = code[pc];
                    OpCode op = instr.getOpCode();

                    switch (op) {
                        case OpCode::MOVE: {
                            // MOVE A B: R(A) := R(B)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size() && b < registers.size()) {
                                registers[a] = registers[b];
                            }
                            break;
                        }
                        case OpCode::LOADK: {
                            // LOADK A Bx: R(A) := Kst(Bx)
                            u8 a = instr.getA();
                            u16 bx = instr.getBx();
                            if (a < registers.size() && bx < constants.size()) {
                                registers[a] = constants[bx];
                            }
                            break;
                        }
                        case OpCode::LOADBOOL: {
                            // LOADBOOL A B C: R(A) := (Bool)B; if (C) pc++
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                registers[a] = Value(static_cast<bool>(b));
                                if (c) pc++; // Skip next instruction
                            }
                            break;
                        }
                        case OpCode::LOADNIL: {
                            // LOADNIL A B: R(A) := ... := R(B) := nil
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            for (u8 i = a; i <= b && i < registers.size(); i++) {
                                registers[i] = Value();
                            }
                            break;
                        }
                        case OpCode::GETGLOBAL: {
                            // GETGLOBAL A Bx: R(A) := Gbl[Kst(Bx)]
                            u8 a = instr.getA();
                            u16 bx = instr.getBx();
                            if (a < registers.size() && bx < constants.size()) {
                                Value globalVal = getGlobal(constants[bx].toString());
                                registers[a] = globalVal;
                            }
                            break;
                        }
                        case OpCode::SETGLOBAL: {
                            // SETGLOBAL A Bx: Gbl[Kst(Bx)] := R(A)
                            u8 a = instr.getA();
                            u16 bx = instr.getBx();
                            if (a < registers.size() && bx < constants.size()) {
                                setGlobal(constants[bx].toString(), registers[a]);
                            }
                            break;
                        }
                        case OpCode::ADD:
                        case OpCode::ADD_MM: {
                            // ADD A B C: R(A) := RK(B) + RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (vb.isNumber() && vc.isNumber()) {
                                    registers[a] = Value(vb.asNumber() + vc.asNumber());
                                }
                            }
                            break;
                        }
                        case OpCode::SUB:
                        case OpCode::SUB_MM: {
                            // SUB A B C: R(A) := RK(B) - RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (vb.isNumber() && vc.isNumber()) {
                                    registers[a] = Value(vb.asNumber() - vc.asNumber());
                                }
                            }
                            break;
                        }
                        case OpCode::MUL:
                        case OpCode::MUL_MM: {
                            // MUL A B C: R(A) := RK(B) * RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (vb.isNumber() && vc.isNumber()) {
                                    registers[a] = Value(vb.asNumber() * vc.asNumber());
                                }
                            }
                            break;
                        }
                        case OpCode::DIV:
                        case OpCode::DIV_MM: {
                            // DIV A B C: R(A) := RK(B) / RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (vb.isNumber() && vc.isNumber() && vc.asNumber() != 0.0) {
                                    registers[a] = Value(vb.asNumber() / vc.asNumber());
                                } else if (vc.asNumber() == 0.0) {
                                    throw LuaException("Division by zero");
                                }
                            }
                            break;
                        }
                        case OpCode::MOD:
                        case OpCode::MOD_MM: {
                            // MOD A B C: R(A) := RK(B) % RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (vb.isNumber() && vc.isNumber() && vc.asNumber() != 0.0) {
                                    double result = fmod(vb.asNumber(), vc.asNumber());
                                    registers[a] = Value(result);
                                } else if (vc.asNumber() == 0.0) {
                                    throw LuaException("Modulo by zero");
                                }
                            }
                            break;
                        }
                        case OpCode::UNM:
                        case OpCode::UNM_MM: {
                            // UNM A B: R(A) := -RK(B)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                if (vb.isNumber()) {
                                    registers[a] = Value(-vb.asNumber());
                                }
                            }
                            break;
                        }
                        case OpCode::NOT: {
                            // NOT A B: R(A) := not RK(B)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                // In Lua, only nil and false are falsy
                                bool isTruthy = !vb.isNil() && !(vb.isBoolean() && !vb.asBoolean());
                                registers[a] = Value(!isTruthy);
                            }
                            break;
                        }
                        case OpCode::LEN: {
                            // LEN A B: R(A) := length of RK(B)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                if (vb.isTable()) {
                                    registers[a] = Value(static_cast<LuaNumber>(vb.asTable()->length()));
                                } else if (vb.isString()) {
                                    registers[a] = Value(static_cast<LuaNumber>(vb.toString().length()));
                                } else {
                                    registers[a] = Value(0.0);
                                }
                            }
                            break;
                        }
                        case OpCode::NEWTABLE: {
                            // NEWTABLE A B C: R(A) := {} (size = B,C)
                            u8 a = instr.getA();
                            if (a < registers.size()) {
                                // Create new table using make_gc_table
                                GCRef<Table> newTable = make_gc_table();
                                registers[a] = Value(newTable);
                            }
                            break;
                        }
                        case OpCode::GETTABLE:
                        case OpCode::GETTABLE_MM: {
                            // GETTABLE A B C: R(A) := R(B)[RK(C)]
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size() && b < registers.size()) {
                                Value table = registers[b];
                                // Correct RK decoding using ISK and INDEXK
                                Value key = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (table.isTable()) {
                                    // Use metamethod-aware table access
                                    registers[a] = CoreMetaMethods::handleIndex(this, table, key);
                                } else {
                                    registers[a] = Value(); // nil
                                }
                            }
                            break;
                        }
                        case OpCode::SETTABLE:
                        case OpCode::SETTABLE_MM: {
                            // SETTABLE A B C: R(A)[RK(B)] := RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                Value table = registers[a];
                                // Correct RK decoding using ISK and INDEXK
                                Value key = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value value = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (table.isTable()) {
                                    // Use metamethod-aware table assignment
                                    CoreMetaMethods::handleNewIndex(this, table, key, value);
                                }
                            }
                            break;
                        }
                        case OpCode::EQ:
                        case OpCode::EQ_MM: {
                            // EQ A B C: if ((RK(B) == RK(C)) ~= A) then pc++
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            // Correct RK decoding using ISK and INDEXK
                            Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                            Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                            bool equal = (vb == vc);
                            if (equal != static_cast<bool>(a)) {
                                pc++; // Skip next instruction
                            }
                            break;
                        }
                        case OpCode::LT:
                        case OpCode::LT_MM: {
                            // LT A B C: if ((RK(B) < RK(C)) ~= A) then pc++
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            // Correct RK decoding using ISK and INDEXK
                            Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                            Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                            bool less = false;
                            if (vb.isNumber() && vc.isNumber()) {
                                less = (vb.asNumber() < vc.asNumber());
                            }
                            if (less != static_cast<bool>(a)) {
                                pc++; // Skip next instruction
                            }
                            break;
                        }
                        case OpCode::LE:
                        case OpCode::LE_MM: {
                            // LE A B C: if ((RK(B) <= RK(C)) ~= A) then pc++
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            // Correct RK decoding using ISK and INDEXK
                            Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                            Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                            bool lessEqual = false;
                            if (vb.isNumber() && vc.isNumber()) {
                                lessEqual = (vb.asNumber() <= vc.asNumber());
                            }
                            if (lessEqual != static_cast<bool>(a)) {
                                pc++; // Skip next instruction
                            }
                            break;
                        }
                        case OpCode::TEST: {
                            // TEST A C: if not (R(A) <=> C) then pc++
                            u8 a = instr.getA();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                Value val = registers[a];
                                // In Lua, only nil and false are falsy
                                bool isTruthy = !val.isNil() && !(val.isBoolean() && !val.asBoolean());
                                if (isTruthy != static_cast<bool>(c)) {
                                    pc++; // Skip next instruction
                                }
                            }
                            break;
                        }
                        case OpCode::JMP: {
                            // JMP sBx: pc += sBx
                            i16 sbx = instr.getSBx();
                            pc += sbx;
                            continue; // Skip pc++ at end of loop
                        }
                        case OpCode::CALL:
                        case OpCode::CALL_MM: {
                            // CALL A B C: R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();

                            if (a < registers.size()) {
                                Value func = registers[a];
                                if (func.isFunction()) {
                                    Vec<Value> args;
                                    // Collect arguments from registers
                                    for (u8 i = 1; i < b && (a + i) < registers.size(); i++) {
                                        args.push_back(registers[a + i]);
                                    }

                                    Value result = call(func, args);

                                    // Store result(s)
                                    if (c > 1 && a < registers.size()) {
                                        registers[a] = result;
                                    }
                                }
                            }
                            break;
                        }
                        case OpCode::RETURN: {
                            // RETURN A B: return R(A), ... ,R(A+B-2)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (b == 1) {
                                return Value(); // Return nil
                            } else if (b == 2 && a < registers.size()) {
                                return registers[a]; // Return single value
                            } else {
                                return Value(); // Multiple returns not implemented yet
                            }
                        }
                        case OpCode::CLOSURE: {
                            // CLOSURE A Bx: R(A) := closure(KPROTO[Bx])
                            u8 a = instr.getA();
                            u16 bx = instr.getBx();
                            if (a < registers.size()) {
                                // For now, create a placeholder Lua function
                                // Full closure implementation requires call stack management
                                // which will be implemented in the next phase
                                GCRef<Function> closure = make_gc_function(static_cast<int>(Function::Type::Lua));
                                registers[a] = Value(closure);
                            }
                            break;
                        }
                        case OpCode::GETUPVAL: {
                            // GETUPVAL A B: R(A) := UpValue[B]
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size()) {
                                // For now, return nil - full upvalue implementation requires
                                // call stack management which will be implemented in the next phase
                                registers[a] = Value(); // nil
                            }
                            break;
                        }
                        case OpCode::SETUPVAL: {
                            // SETUPVAL A B: UpValue[B] := R(A)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            // For now, do nothing - full upvalue implementation requires
                            // call stack management which will be implemented in the next phase
                            break;
                        }
                        default:
                            // Skip unhandled instructions with warning
                            std::cerr << "Warning: Unhandled opcode " << static_cast<int>(op) << " at PC " << pc << std::endl;
                            break;
                    }

                    pc++;
                }

                return Value(); // Return nil if no explicit return
            } catch (const std::exception& e) {
                throw LuaException("VM execution failed: " + std::string(e.what()));
            }

            // Restore stack top after VM execution
            setTop(oldTop);

            return Value(); // Return nil
        } catch (const LuaException& e) {
            std::cerr << "LuaException in call: " << e.what() << std::endl;
            // CRITICAL FIX: Always re-throw LuaException for proper pcall error handling
            // pcall should be the only place that catches and converts exceptions to return values
            throw; // Re-throw all LuaExceptions to allow pcall to handle them properly
        }
    }

    CallResult State::callMultiple(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Native function call with multiple return values support
        if (function->getType() == Function::Type::Native) {
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("attempt to call a nil value");
                }

                // Legacy function call - convert to single return value
                Value result = nativeFnLegacy(this, static_cast<int>(args.size()));
                return CallResult(result);
            } else {
                auto nativeFn = function->getNative();
                if (!nativeFn) {
                    throw LuaException("attempt to call a nil value");
                }

                // Save current stack top
                int oldTop = top;

                // Push arguments onto stack
                for (size_t i = 0; i < args.size(); ++i) {
                    push(args[i]);
                }

                // Call new multi-return function
                i32 returnCount = nativeFn(this);

                // Collect return values from stack
                Vec<Value> results;
                for (i32 i = 0; i < returnCount; ++i) {
                    results.push_back(get(oldTop + i));
                }

                // Restore stack top
                setTop(oldTop);

                return CallResult(results);
            }
        }

        // For Lua function calls, use VM to execute with multiple return values
        try {
            // Save current state
            int oldTop = top;

            // Lua 5.1 calling convention:
            // Stack layout: [function] [arg1] [arg2] [arg3] ...
            // Register 0 = function, Register 1 = arg1, Register 2 = arg2, etc.

            // Push function onto stack first
            push(Value(function));

            // Push arguments onto stack
            for (const auto& arg : args) {
                push(arg);
            }

            // Create LuaState for VM execution with multiple return values
            // TODO: This is a simplified implementation
            // In a full implementation, we would need to properly set up LuaState
            // and use VM::call or VM::pcall with proper parameters

            // For now, return an empty CallResult to allow compilation
            CallResult result;  // empty result

            // Restore stack top after VM execution
            setTop(oldTop);

            return result;
        } catch (const LuaException& e) {
            std::cerr << "LuaException in callMultiple: " << e.what() << std::endl;
            // CRITICAL FIX: Always re-throw LuaException for proper pcall error handling
            // pcall should be the only place that catches and converts exceptions to return values
            throw; // Re-throw all LuaExceptions to allow pcall to handle them properly
        }
    }

    Value State::callSafe(const Value& func, const Vec<Value>& args) {
        // VM is now static, no need for context checking
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        // Use regular call method (VM is static)
        return call(func, args);
    }

    CallResult State::callSafeMultiple(const Value& func, const Vec<Value>& args) {
        // VM is now static, no need for context checking
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        // Use regular callMultiple method (VM is static)
        return callMultiple(func, args);
    }

    // Native function call with arguments already on stack (Lua 5.1 design)
    Value State::callNative(const Value& func, int nargs) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Only handle native functions
        if (function->getType() != Function::Type::Native) {
            throw LuaException("callNative can only call native functions");
        }

        // Check if it's a legacy function
        if (function->isNativeLegacy()) {
            auto nativeFnLegacy = function->getNativeLegacy();
            if (!nativeFnLegacy) {
                throw LuaException("attempt to call a nil value");
            }

            // Legacy function call - return single value
            Value result = nativeFnLegacy(this, nargs);
            return result;
        } else {
            // New multi-return function - call and return first value for compatibility
            i32 returnCount = callNativeMultiple(func, nargs);
            if (returnCount > 0) {
                return get(top - returnCount);  // Return first value
            } else {
                return Value();  // Return nil if no values
            }
        }
    }

    // Native function call with multiple return values (Lua 5.1 standard)
    i32 State::callNativeMultiple(const Value& func, int nargs) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Only handle native functions
        if (function->getType() != Function::Type::Native) {
            throw LuaException("callNativeMultiple can only call native functions");
        }

        // Store the stack position before arguments (0-based absolute index)
        const int stackBase = top - nargs;
        if (stackBase < 0) {
            throw LuaException("invalid argument count for native call");
        }

        i32 returnCount = 0;

        if (function->isNativeLegacy()) {
            auto nativeFnLegacy = function->getNativeLegacy();
            if (!nativeFnLegacy) {
                throw LuaException("attempt to call a nil value");
            }
            // Legacy: returns single Value; replace args with result in place
            Value result = nativeFnLegacy(this, nargs);
            top = stackBase;    // pop args
            push(result);       // push single result
            returnCount = 1;
        } else {
            auto nativeFn = function->getNative();
            if (!nativeFn) {
                throw LuaException("attempt to call a nil value");
            }
            // Lua 5.1 style: arguments are already at [stackBase .. top-1].
            // Native pushes return values on the same State stack and returns count.
            const int oldTop = top;
            returnCount = nativeFn(this);
            if (returnCount < 0) {
                throw LuaException("native function returned invalid return count");
            }
            // Ensure the native respected the protocol; if it pushed fewer values,
            // pad with nil to keep stack shape predictable.
            while (top < oldTop + returnCount) {
                push(Value());
            }
            // Move return segment to start at stackBase
            // 目标：把 [oldTop..oldTop+returnCount-1] 搬到从 stackBase 开始
            int srcStart = oldTop;
            int dstStart = stackBase;
            if (dstStart != srcStart && returnCount > 0) {
                // Simple in-place move when dst before src; use临时缓存，避免覆盖
                Vec<Value> tmp;
                tmp.reserve(returnCount);
                for (int i = 0; i < returnCount; ++i) tmp.push_back(stack[srcStart + i]);
                top = dstStart; // truncate to stackBase
                for (int i = 0; i < returnCount; ++i) push(tmp[i]);
            } else {
                // 如果正好在原位，则不需要移动；确保 top 指向返回段末尾
                top = oldTop + returnCount;
            }
        }

        return returnCount;
    }

    // Lua function call with arguments already on stack
    Value State::callLua(const Value& func, int nargs) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        auto function = func.asFunction();

        // Only handle Lua functions
        if (function->getType() != Function::Type::Lua) {
            throw LuaException("callLua can only call Lua functions");
        }

        // Lua 5.1官方函数调用实现
        try {
            // 保存当前栈状态
            int oldTop = top;

            

            // Lua 5.1调用约定修复：
            // 当前栈状态：参数已经在栈顶：[arg1] [arg2] [arg3] ...
            // 目标栈状态：[function] [arg1] [arg2] [arg3] ...

            // 关键修复：正确计算参数在栈上的位置
            // 参数位置：从 (top - nargs) 到 (top - 1)

            // 1. 保存参数到临时数组（彻底修复索引计算）
            Vec<Value> args;
            args.reserve(nargs);
            for (int i = 0; i < nargs; ++i) {
                // 彻底修复：参数在栈上的正确位置
                // 栈顶是最后一个参数，栈底是第一个参数
                // 第i个参数的位置：top - nargs + i
                int argIndex = top - nargs + i;

                // 边界检查
                if (argIndex >= 0 && argIndex < top) {
                    args.push_back(get(argIndex));
                } else {
                    args.push_back(Value()); // nil for invalid indices
                }
            }

            // 2. 清理栈顶的参数
            setTop(top - nargs);

            // 3. 按Lua 5.1标准顺序推送：函数 + 参数
            push(func);
            for (int i = 0; i < nargs; ++i) {
                push(args[i]);
            }

            // 验证：栈布局现在是 [function] [arg1] [arg2] [arg3] ...

            // 栈布局现在是：[function] [arg1] [arg2] [arg3] ...

            // 4. Execute Lua function using static VM
            // TODO: This is a simplified implementation
            // In a full implementation, we would need to properly set up LuaState
            // and use VM::call with proper parameters

            // For now, return a nil value to allow compilation
            Value result = Value();  // nil

            // 5. 恢复栈状态
            setTop(oldTop);

            return result;

        } catch (const LuaException& e) {
            std::cerr << "LuaException in callLua: " << e.what() << std::endl;
            // CRITICAL FIX: Always re-throw LuaException for proper pcall error handling
            // pcall should be the only place that catches and converts exceptions to return values
            throw; // Re-throw all LuaExceptions to allow pcall to handle them properly
        }
    }

    // Execute Lua code from string
    bool State::doString(const Str& code) {
        try {
            // 1. Parse code using our parser
            Parser parser(code);
            auto statements = parser.parse();

            // Check if there are errors in parsing phase
            if (parser.hasError()) {
                // Output parsing errors in Lua 5.1 format
                Str formattedErrors = parser.getFormattedErrors();
                if (!formattedErrors.empty()) {
                    std::cerr << formattedErrors << std::endl;
                }
                return false;
            }

            // 2. Generate bytecode using compiler
            Compiler compiler;
            GCRef<Function> function = compiler.compile(statements);

            if (!function) {
                return false;
            }

            // 3. Execute bytecode using static VM
            // Call the function with no arguments
            try {
                Vec<Value> args;  // No arguments
                call(Value(function), args);
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Execution error: " << e.what() << std::endl;
                return false;
            }
        } catch (const LuaException& e) {
            // Can handle or log errors here
            std::cerr << "Lua error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            // Handle other exceptions that may occur
            std::cerr << "Error executing Lua code: " << e.what() << std::endl;
            return false;
        }
    }

    // Execute Lua code from string and return result (for REPL)
    Value State::doStringWithResult(const Str& code) {
        try {
            // 1. Parse code using our parser
            Parser parser(code);
            auto statements = parser.parse();

            // Check if there are errors in parsing phase
            if (parser.hasError()) {
                // Output parsing errors in Lua 5.1 format for REPL
                Str formattedErrors = parser.getFormattedErrors();
                if (!formattedErrors.empty()) {
                    std::cerr << formattedErrors << std::endl;
                }
                throw LuaException("Parse error");
            }

            // 2. Generate bytecode using compiler
            Compiler compiler;
            GCRef<Function> function = compiler.compile(statements);

            if (!function) {
                throw LuaException("Compile error");
            }

            // 3. Execute bytecode using static VM
            // Call the function with no arguments and get result
            try {
                Vec<Value> args;  // No arguments
                Value result = call(Value(function), args);
                return result;
            } catch (const std::exception& e) {
                throw LuaException("Execution error: " + std::string(e.what()));
            }

        } catch (const LuaException& e) {
            // Re-throw LuaException for specific Lua errors
            throw LuaException("Lua error: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw LuaException("Error executing Lua code: " + std::string(e.what()));
        }
    }

    // Load and execute Lua code from file
    bool State::doFile(const Str& filename) {
        try {
            // 1. Open specified file
            std::ifstream file(filename);
            if (!file.is_open()) {
                return false;
            }

            // 2. Read file content to string
            std::stringstream buffer;
            buffer << file.rdbuf();

            // 3. Close file
            file.close();

            // 4. Call doString to execute the string
            return doString(buffer.str());
        } catch (const std::exception& e) {
            // File operations may throw various exceptions
            std::cerr << "Error reading file '" << filename << "': " << e.what() << std::endl;
            return false;
        }
    }

    // GCObject virtual function implementations
    void State::markReferences(GarbageCollector* gc) {
        // Mark all values on the stack
        for (const Value& value : stack) {
            value.markReferences(gc);
        }

        // Mark all global variables
        for (const auto& pair : globals) {
            pair.second.markReferences(gc);
        }
    }

    usize State::getSize() const {
        return sizeof(State);
    }

    usize State::getAdditionalSize() const {
        // Calculate additional memory used by vectors and maps
        usize stackSize = stack.capacity() * sizeof(Value);
        usize globalsSize = globals.size() * (sizeof(Str) + sizeof(Value));
        return stackSize + globalsSize;
    }
}
