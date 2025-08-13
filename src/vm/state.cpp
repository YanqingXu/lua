#include "state.hpp"
#include "global_state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "upvalue.hpp"
#include "instruction.hpp"
#include "core_metamethods.hpp"
#include "metamethod_manager.hpp"
#include "../common/defines.hpp"
#include "../parser/parser.hpp"
#include "../compiler/compiler.hpp"
#include <cmath>
#include "../vm/vm.hpp"
#include "../gc/core/garbage_collector.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

namespace Lua {
    State::State() : GCObject(GCObjectType::State, sizeof(State)),
                     ownedGlobalState_(nullptr), globalState_(nullptr), luaState_(nullptr),
                     top(0), useGlobalState_(true), currentFunction_(nullptr), fullyMigrated_(false),
                     isCoroutine_(false), parentState_(nullptr), currentCoroutine_(nullptr), nextClosureId_(1),
                     debugInfo_(nullptr),
                     debugCallStack_(nullptr),
                     currentSourceLine_(-1) {
        // Initialize legacy stack space for compatibility
        stack.resize(LUAI_MAXSTACK);

        // Initialize debug components (disabled for now)
        // debugCallStack_->setDebugInfoManager(debugInfo_.get());

        // Initialize Lua 5.1 architecture
        initializeLua5_1Architecture_();
    }

    State::State(GlobalState* globalState) : GCObject(GCObjectType::State, sizeof(State)),
                                             ownedGlobalState_(nullptr), globalState_(globalState), luaState_(nullptr),
                                             top(0), useGlobalState_(true), currentFunction_(nullptr), fullyMigrated_(false),
                                             isCoroutine_(false), parentState_(nullptr), currentCoroutine_(nullptr), nextClosureId_(1),
                                             debugInfo_(nullptr),
                                             debugCallStack_(nullptr),
                                             currentSourceLine_(-1) {
        // Initialize legacy stack space for compatibility
        stack.resize(LUAI_MAXSTACK);

        // Initialize debug components (disabled for now)
        // debugCallStack_->setDebugInfoManager(debugInfo_.get());

        // Initialize Lua 5.1 architecture with provided GlobalState
        if (globalState_) {
            luaState_ = globalState_->newThread();
        } else {
            initializeLua5_1Architecture_();
        }
    }

    State::~State() {
        // Clean up Lua 5.1 architecture
        cleanupLua5_1Architecture_();
    }

    void State::push(const Value& value) {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            luaState_->push(value);
        } else {
            // Legacy implementation for compatibility
            if (top >= LUAI_MAXSTACK) {
                throw LuaException("stack overflow");
            }
            stack[top++] = value;
        }
    }

    Value State::pop() {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            return luaState_->pop();
        } else {
            // Legacy implementation for compatibility
            if (top <= 0) {
                throw LuaException("stack underflow");
            }
            return stack[--top];
        }
    }

    Value& State::get(int idx) {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            return luaState_->get(idx);
        } else {
            // Legacy implementation for compatibility
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
    }

    void State::set(int idx, const Value& value) {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            luaState_->set(idx, value);
        } else {
            // Legacy implementation for compatibility
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

    // Stack management methods
    int State::getTop() const {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            return luaState_->getTop();
        } else {
            // Legacy implementation
            return top;
        }
    }

    void State::setTop(int newTop) {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            luaState_->setTop(newTop);
        } else {
            // Legacy implementation
            if (newTop > top) {
                // Fill new positions with nil when increasing stack size
                for (int i = top; i < newTop; i++) {
                    stack[i] = Value();  // Default Value constructor creates nil
                }
            }
            top = newTop;
        }
    }

    void State::clearStack() {
        if (fullyMigrated_ && luaState_) {
            // Delegate to LuaState
            luaState_->setTop(0);
        } else {
            // Legacy implementation
            top = 0;
        }
    }

    // Function call (Lua 5.1官方设计) with stack overflow protection
    Value State::call(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }

        // Simple stack overflow protection
        static thread_local int callDepth = 0;
        constexpr int MAX_CALL_DEPTH = 200; // Conservative limit

        if (callDepth >= MAX_CALL_DEPTH) {
            throw LuaException("stack overflow (too many nested function calls)");
        }

        callDepth++;

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

        // For Lua function calls, use optimized CallInfo mechanism
        // Set current function for closure creation
        GCRef<Function> previousFunction = currentFunction_;
        currentFunction_ = function;

        try {
            // Use Lua 5.1 CallInfo optimization if available
            if (false && fullyMigrated_ && luaState_) {  // Temporarily disabled for debugging
                return callWithCallInfo_(function, args);
            }

            // Fallback to legacy implementation
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

                // Copy arguments from stack to registers
                // Register 0 = function (already on stack at oldTop+1)
                // Register 1 = arg1, Register 2 = arg2, etc.
                for (size_t i = 0; i < args.size() && i + 1 < registers.size(); ++i) {
                    registers[i + 1] = args[i];  // args[0] goes to register[1], etc.
                }

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
                        case OpCode::POW:
                        case OpCode::POW_MM: {
                            // POW A B C: R(A) := RK(B) ^ RK(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                                if (vb.isNumber() && vc.isNumber()) {
                                    registers[a] = Value(std::pow(vb.asNumber(), vc.asNumber()));
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
                        case OpCode::CONCAT:
                        case OpCode::CONCAT_MM: {
                            // CONCAT A B C: R(A) := R(B) .. R(C)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            if (a < registers.size()) {
                                // Correct RK decoding using ISK and INDEXK
                                Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                                Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];

                                // Convert both operands to strings and concatenate
                                std::string result = vb.toString() + vc.toString();
                                registers[a] = Value(result);
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
                            // Check if this is a comparison for boolean result or conditional jump
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            // Correct RK decoding using ISK and INDEXK
                            Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                            Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                            bool equal = (vb == vc);

                            // If A is a register (< 250), store boolean result
                            // If A is 0 or 1, it's a conditional jump instruction
                            if (a < 250) {
                                // Store boolean result in register A
                                registers[a] = Value(equal);
                            } else {
                                // Traditional conditional jump behavior
                                if (equal != static_cast<bool>(a & 1)) {
                                    pc++; // Skip next instruction
                                }
                            }
                            break;
                        }
                        case OpCode::LT:
                        case OpCode::LT_MM: {
                            // LT A B C: comparison with dual behavior
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            // Correct RK decoding using ISK and INDEXK
                            Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                            Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                            bool less = false;
                            if (vb.isNumber() && vc.isNumber()) {
                                less = (vb.asNumber() < vc.asNumber());
                            } else if (vb.isString() && vc.isString()) {
                                less = (vb.toString() < vc.toString());
                            }

                            // If A is a register (< 250), store boolean result
                            // If A is 0 or 1, it's a conditional jump instruction
                            if (a < 250) {
                                // Store boolean result in register A
                                registers[a] = Value(less);
                            } else {
                                // Traditional conditional jump behavior
                                if (less != static_cast<bool>(a & 1)) {
                                    pc++; // Skip next instruction
                                }
                            }
                            break;
                        }
                        case OpCode::LE:
                        case OpCode::LE_MM: {
                            // LE A B C: comparison with dual behavior
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            u8 c = instr.getC();
                            // Correct RK decoding using ISK and INDEXK
                            Value vb = ISK(b) ? constants[INDEXK(b)] : registers[b];
                            Value vc = ISK(c) ? constants[INDEXK(c)] : registers[c];
                            bool lessEqual = false;
                            if (vb.isNumber() && vc.isNumber()) {
                                lessEqual = (vb.asNumber() <= vc.asNumber());
                            } else if (vb.isString() && vc.isString()) {
                                lessEqual = (vb.toString() <= vc.toString());
                            }

                            // If A is a register (< 250), store boolean result
                            // If A is 0 or 1, it's a conditional jump instruction
                            if (a < 250) {
                                // Store boolean result in register A
                                registers[a] = Value(lessEqual);
                            } else {
                                // Traditional conditional jump behavior
                                if (lessEqual != static_cast<bool>(a & 1)) {
                                    pc++; // Skip next instruction
                                }
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
                        case OpCode::FORPREP: {
                            // FORPREP A sBx: R(A) -= R(A+2); pc += sBx
                            // Prepare for numeric for loop
                            u8 a = instr.getA();
                            i16 sbx = instr.getSBx();

                            if (a + 2 < registers.size()) {
                                // R(A) = initial value, R(A+1) = limit, R(A+2) = step
                                // Subtract step from initial value for first iteration
                                if (registers[a].isNumber() && registers[a + 2].isNumber()) {
                                    registers[a] = Value(registers[a].asNumber() - registers[a + 2].asNumber());
                                }
                                pc += sbx;
                                continue; // Skip pc++ at end of loop
                            }
                            break;
                        }
                        case OpCode::FORLOOP: {
                            // FORLOOP A sBx: R(A) += R(A+2); if R(A) <= R(A+1) then { pc += sBx; R(A+3) = R(A) }
                            // Execute numeric for loop iteration
                            u8 a = instr.getA();
                            i16 sbx = instr.getSBx();

                            if (a + 3 < registers.size()) {
                                // R(A) = current value, R(A+1) = limit, R(A+2) = step, R(A+3) = loop variable
                                if (registers[a].isNumber() && registers[a + 1].isNumber() && registers[a + 2].isNumber()) {
                                    // Add step to current value
                                    double current = registers[a].asNumber() + registers[a + 2].asNumber();
                                    registers[a] = Value(current);

                                    // Check loop condition (step > 0 ? current <= limit : current >= limit)
                                    double limit = registers[a + 1].asNumber();
                                    double step = registers[a + 2].asNumber();
                                    bool continue_loop = (step > 0) ? (current <= limit) : (current >= limit);

                                    if (continue_loop) {
                                        // Set loop variable and jump back
                                        registers[a + 3] = Value(current);
                                        pc += sbx;
                                        continue; // Skip pc++ at end of loop
                                    }
                                    // If loop ends, fall through to next instruction
                                }
                            }
                            break;
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
                                    // Use original function call mechanism
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
                                } else {
                                    throw LuaException("attempt to call a non-function value");
                                }
                            }
                            break;
                        }
                        case OpCode::RETURN: {
                            // RETURN A B: return R(A), ... ,R(A+B-2)
                            u8 a = instr.getA();
                            u8 b = instr.getB();

                            // Decrement call depth before returning
                            callDepth--;

                            // Optimized return handling
                            if (b == 1) {
                                // Return no values (nil)
                                return Value();
                            } else if (b == 2 && a < registers.size()) {
                                // Return single value
                                return registers[a];
                            } else if (b == 0) {
                                // Return all values from A to top of stack
                                // For now, return the first value
                                return (a < registers.size()) ? registers[a] : Value();
                            } else {
                                // Return multiple values: R(A), R(A+1), ..., R(A+B-2)
                                // For now, return the first value (single return compatibility)
                                return (a < registers.size()) ? registers[a] : Value();
                            }
                        }
                        case OpCode::CLOSURE: {
                            // CLOSURE A Bx: R(A) := closure(KPROTO[Bx])
                            u8 a = instr.getA();
                            u16 bx = instr.getBx();
                            if (a < registers.size() && currentFunction_ && currentFunction_->getType() == Function::Type::Lua) {
                                // Get the prototype from the current function's prototypes
                                const auto& prototypes = currentFunction_->getPrototypes();
                                if (bx < prototypes.size()) {
                                    GCRef<Function> prototype = prototypes[bx];
                                    if (prototype) {
                                        // Create a new closure instance from the prototype
                                        GCRef<Function> closureInstance = createClosureFromPrototype(prototype);
                                        registers[a] = Value(closureInstance);

                                        // Create unique closure instance ID for this closure
                                        usize closureId = nextClosureId_++;
                                        void* closureKey = reinterpret_cast<void*>(closureId);

                                        // Store the mapping from this closure instance to its ID
                                        functionToClosureId_[closureInstance.get()] = closureId;

                                        // Process upvalue binding instructions that follow
                                        // The compiler generates upvalue binding instructions after CLOSURE
                                        u8 nupvalues = prototype->getUpvalueCount();
                                        for (u8 i = 0; i < nupvalues; ++i) {
                                            // Read next instruction for upvalue binding
                                            if (pc + 1 < code.size()) {
                                                pc++; // Move to next instruction
                                                Instruction bindInstr = code[pc];

                                                u8 isLocal = bindInstr.getA();
                                                u8 sourceIndex = bindInstr.getB();

                                                // Bind upvalue based on source
                                                if (isLocal == 1) {
                                                    // Source is a local variable (register)
                                                    if (sourceIndex < registers.size()) {
                                                        // Use the closure ID that was already created
                                                        void* closureKey = reinterpret_cast<void*>(closureId);

                                                        // Initialize upvalue storage for this closure instance
                                                        if (functionUpvalues_.find(closureKey) == functionUpvalues_.end()) {
                                                            functionUpvalues_[closureKey] = Vec<Value>(256, Value());
                                                        }

                                                        functionUpvalues_[closureKey][i] = registers[sourceIndex];
                                                    }
                                                }
                                                // TODO: Handle upvalue-to-upvalue binding (isLocal == 0)
                                            }
                                        }
                                    } else {
                                        registers[a] = Value(); // nil
                                    }
                                } else {
                                    registers[a] = Value(); // nil
                                }
                            } else {
                                registers[a] = Value(); // nil
                            }
                            break;
                        }
                        case OpCode::GETUPVAL: {
                            // GETUPVAL A B: R(A) := UpValue[B]
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size() && currentFunction_) {
                                // Get closure instance ID for current function
                                void* funcPtr = currentFunction_.get();
                                auto it = functionToClosureId_.find(funcPtr);

                                if (it != functionToClosureId_.end()) {
                                    // Found closure instance ID
                                    usize closureId = it->second;
                                    void* closureKey = reinterpret_cast<void*>(closureId);

                                    // Initialize upvalue storage for this closure instance if not exists
                                    if (functionUpvalues_.find(closureKey) == functionUpvalues_.end()) {
                                        functionUpvalues_[closureKey] = Vec<Value>(256, Value(0.0)); // Initialize with 0
                                    }

                                    if (b < functionUpvalues_[closureKey].size()) {
                                        registers[a] = functionUpvalues_[closureKey][b];
                                    } else {
                                        registers[a] = Value(); // nil
                                    }
                                } else {
                                    // No closure instance found, return nil
                                    registers[a] = Value(); // nil
                                }
                            } else {
                                registers[a] = Value(); // nil
                            }
                            break;
                        }
                        case OpCode::SETUPVAL: {
                            // SETUPVAL A B: UpValue[B] := R(A)
                            u8 a = instr.getA();
                            u8 b = instr.getB();
                            if (a < registers.size() && currentFunction_) {
                                // Get closure instance ID for current function
                                void* funcPtr = currentFunction_.get();
                                auto it = functionToClosureId_.find(funcPtr);

                                if (it != functionToClosureId_.end()) {
                                    // Found closure instance ID
                                    usize closureId = it->second;
                                    void* closureKey = reinterpret_cast<void*>(closureId);

                                    // Initialize upvalue storage for this closure instance if not exists
                                    if (functionUpvalues_.find(closureKey) == functionUpvalues_.end()) {
                                        functionUpvalues_[closureKey] = Vec<Value>(256, Value(0.0)); // Initialize with 0
                                    }

                                    if (b < functionUpvalues_[closureKey].size()) {
                                        functionUpvalues_[closureKey][b] = registers[a];
                                    }
                                }
                            }
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

            // Restore previous function and decrement call depth
            currentFunction_ = previousFunction;
            callDepth--;

            return Value(); // Return nil
        } catch (const LuaException& e) {
            // Restore previous function in case of exception and decrement call depth
            currentFunction_ = previousFunction;
            callDepth--;
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

    // Lua 5.1 architecture initialization and cleanup
    void State::initializeLua5_1Architecture_() {
        try {
            // Create GlobalState if not provided
            if (!globalState_) {
                ownedGlobalState_ = std::make_unique<GlobalState>();
                globalState_ = ownedGlobalState_.get();
            }

            // Create main thread LuaState
            luaState_ = globalState_->newThread();

            // Mark as fully migrated
            fullyMigrated_ = true;
            useGlobalState_ = true;

            // Initialize C++20 coroutine manager
            coroutineManager_ = std::make_unique<CoroutineManager>();

        } catch (const std::exception& e) {
            // Fallback to legacy implementation if Lua 5.1 initialization fails
            fullyMigrated_ = false;
            std::cerr << "Warning: Failed to initialize Lua 5.1 architecture, falling back to legacy: "
                      << e.what() << std::endl;
        }
    }

    void State::migrateLegacyState_() {
        if (!fullyMigrated_ || !luaState_) return;

        // Migrate legacy stack to LuaState
        for (int i = 0; i < top; i++) {
            luaState_->push(stack[i]);
        }

        // Migrate legacy globals to GlobalState
        for (const auto& pair : globals) {
            globalState_->setGlobal(pair.first, pair.second);
        }

        // Clear legacy data
        stack.clear();
        globals.clear();
        top = 0;
    }

    void State::cleanupLua5_1Architecture_() {
        if (luaState_ && globalState_ && ownedGlobalState_) {
            // Close the thread if we own the GlobalState
            globalState_->closeThread(luaState_);
        }
        luaState_ = nullptr;

        // Reset owned GlobalState
        ownedGlobalState_.reset();
        globalState_ = nullptr;
    }

    // Stack delegation helpers
    Value* State::indexToLuaStackAddr_(int idx) {
        if (!fullyMigrated_ || !luaState_) return nullptr;
        return luaState_->index2addr(idx);
    }

    int State::luaStackAddrToIndex_(Value* addr) {
        if (!fullyMigrated_ || !luaState_) return -1;
        // This would require additional LuaState methods to implement properly
        return -1; // Placeholder
    }

    // CallInfo optimized function call implementation
    Value State::callWithCallInfo_(GCRef<Function> function, const Vec<Value>& args) {
        if (!luaState_ || !function) {
            return Value(); // nil
        }

        try {
            // Push function onto LuaState stack
            luaState_->push(Value(function));

            // Push arguments onto LuaState stack
            for (const auto& arg : args) {
                luaState_->push(arg);
            }

            // Use LuaState's optimized call mechanism with CallInfo
            i32 nargs = static_cast<i32>(args.size());
            i32 nresults = 1; // Expect single return value

            // Get function position on stack
            i32 funcIndex = luaState_->getTop() - nargs - 1;
            Value* func = luaState_->index2addr(funcIndex);

            // Prepare call using CallInfo mechanism
            luaState_->precall(func, nresults);

            // Execute function body using VM
            if (function->getType() == Function::Type::Lua) {
                Value result = executeLuaFunctionWithCallInfo_(function);

                // Post-call cleanup
                luaState_->postcall(&result);

                return result;
            } else {
                // For native functions, call directly
                // Post-call cleanup with nil result
                Value nilResult;
                luaState_->postcall(&nilResult);
                return nilResult;
            }

        } catch (const std::exception& e) {
            // Error handling - clean up stack and return nil
            std::cerr << "Error in CallInfo function call: " << e.what() << std::endl;
            return Value(); // nil
        }
    }

    // Execute Lua function with CallInfo context
    Value State::executeLuaFunctionWithCallInfo_(GCRef<Function> function) {
        if (!function || function->getType() != Function::Type::Lua) {
            return Value(); // nil
        }

        try {
            // Get function code and constants
            const auto& code = function->getCode();
            const auto& constants = function->getConstants();

            // Use CallInfo-aware register management
            CallInfo* ci = luaState_->getCurrentCI();
            if (!ci) {
                return Value(); // nil
            }

            // Simple register file (enhanced with CallInfo context)
            Vec<Value> registers(256, Value()); // 256 registers initialized to nil

            // Copy arguments from stack to registers
            Value* base = ci->base;
            for (i32 i = 0; base + i < ci->top; i++) {
                if (i < static_cast<i32>(registers.size())) {
                    registers[i] = *(base + i);
                }
            }

            // VM execution loop with CallInfo context
            size_t pc = 0;
            while (pc < code.size()) {
                Instruction instr = code[pc];
                OpCode op = instr.getOpCode();

                // Execute instruction (reuse existing VM logic)
                Value result = executeInstructionWithCallInfo_(instr, registers, constants, pc);
                if (!result.isNil()) {
                    // Function returned a value
                    return result;
                }

                pc++;
            }

            return Value(); // Return nil if no explicit return

        } catch (const std::exception& e) {
            std::cerr << "Error executing Lua function with CallInfo: " << e.what() << std::endl;
            return Value(); // nil
        }
    }

    // Execute single instruction with CallInfo context
    Value State::executeInstructionWithCallInfo_(const Instruction& instr, Vec<Value>& registers,
                                                const Vec<Value>& constants, size_t& pc) {
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
                // LOADK A Bx: R(A) := K(Bx)
                u8 a = instr.getA();
                u16 bx = instr.getBx();
                if (a < registers.size() && bx < constants.size()) {
                    registers[a] = constants[bx];
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

            case OpCode::RETURN: {
                // RETURN A B: return R(A), ..., R(A+B-2)
                u8 a = instr.getA();
                u8 b = instr.getB();

                if (b == 0) {
                    // Return all values from A to top
                    return registers[a];
                } else if (b == 1) {
                    // Return no values
                    return Value();
                } else {
                    // Return specific number of values (for now, just return first)
                    if (a < registers.size()) {
                        return registers[a];
                    }
                }
                return Value();
            }

            case OpCode::CLOSURE: {
                // CLOSURE A Bx: R(A) := closure(KPROTO[Bx], R(A), ..., R(A+n))
                u8 a = instr.getA();
                u16 bx = instr.getBx();

                // Get the prototype from current function's prototypes
                if (currentFunction_ && bx < currentFunction_->getPrototypes().size()) {
                    GCRef<Function> prototype = currentFunction_->getPrototypes()[bx];
                    if (prototype && a < registers.size()) {
                        registers[a] = Value(prototype);
                    }
                }
                break;
            }

            // Add more instruction cases as needed...
            default:
                // For unimplemented instructions, continue execution
                break;
        }

        return Value(); // Continue execution (no return)
    }

    // C++20 coroutine implementation methods
    LuaCoroutine* State::createCoroutine(GCRef<Function> func) {
        if (!coroutineManager_) {
            std::cerr << "Error: CoroutineManager not initialized" << std::endl;
            return nullptr;
        }

        try {
            // Create new LuaState for the coroutine
            LuaState* newThread = nullptr;
            if (globalState_) {
                newThread = globalState_->newThread();
            }

            // Create C++20 coroutine
            LuaCoroutine* coro = coroutineManager_->createCoroutine(this, newThread);
            if (!coro) {
                std::cerr << "Error: Failed to create LuaCoroutine" << std::endl;
                return nullptr;
            }

            return coro;

        } catch (const std::exception& e) {
            std::cerr << "Error creating C++20 coroutine: " << e.what() << std::endl;
            return nullptr;
        }
    }

    CoroutineResult State::resumeCoroutine(LuaCoroutine* coro, const Vec<Value>& args) {
        if (!coro) {
            return CoroutineResult{false, CoroutineStatus::DEAD};
        }

        try {
            // Set current coroutine
            if (coroutineManager_) {
                coroutineManager_->setCurrentCoroutine(coro);
            }
            currentCoroutine_ = coro;

            // Resume the C++20 coroutine
            CoroutineResult result = coro->resume(args);

            // Update current coroutine if finished
            if (result.status == CoroutineStatus::DEAD) {
                currentCoroutine_ = nullptr;
                if (coroutineManager_) {
                    coroutineManager_->setCurrentCoroutine(nullptr);
                }
            }

            return result;

        } catch (const std::exception& e) {
            std::cerr << "Error resuming C++20 coroutine: " << e.what() << std::endl;
            return CoroutineResult{false, CoroutineStatus::DEAD};
        }
    }

    CoroutineResult State::yieldFromCoroutine(const Vec<Value>& values) {
        if (!currentCoroutine_) {
            return CoroutineResult{false, CoroutineStatus::DEAD};
        }

        try {
            return currentCoroutine_->yield(values);
        } catch (const std::exception& e) {
            std::cerr << "Error yielding from C++20 coroutine: " << e.what() << std::endl;
            return CoroutineResult{false, CoroutineStatus::DEAD};
        }
    }

    CoroutineStatus State::getCoroutineStatus(LuaCoroutine* coro) {
        if (!coro) {
            return CoroutineStatus::DEAD;
        }
        return coro->getStatus();
    }

    // Legacy coroutine methods for backward compatibility
    State* State::newCoroutine() {
        // Create a legacy State-based coroutine for backward compatibility
        if (!fullyMigrated_ || !globalState_) {
            // Fallback: create traditional coroutine
            State* coro = new State(globalState_);
            coro->isCoroutine_ = true;
            coro->parentState_ = this;
            childCoroutines_.push_back(coro);
            return coro;
        }

        try {
            // Create new coroutine using GlobalState thread management
            LuaState* newThread = globalState_->newThread();

            // Create State wrapper for the new thread
            State* coro = new State(globalState_);
            coro->luaState_ = newThread;
            coro->fullyMigrated_ = true;
            coro->isCoroutine_ = true;
            coro->parentState_ = this;

            // Add to child coroutines list
            childCoroutines_.push_back(coro);

            return coro;

        } catch (const std::exception& e) {
            std::cerr << "Error creating legacy coroutine: " << e.what() << std::endl;
            return nullptr;
        }
    }

    Value State::resumeCoroutine(State* coro, const Vec<Value>& args) {
        if (!coro || !coro->isCoroutine_) {
            throw LuaException("attempt to resume a non-coroutine");
        }

        try {
            if (coro->fullyMigrated_ && coro->luaState_) {
                // Use Lua 5.1 coroutine mechanism
                LuaState* coroState = coro->luaState_;

                // Push arguments onto coroutine stack
                for (const auto& arg : args) {
                    coroState->push(arg);
                }

                // Set coroutine status to running
                coroState->setStatus(LUA_OK);

                // For now, return a placeholder result
                // In full implementation, this would execute the coroutine
                return Value(42); // Placeholder

            } else {
                // Fallback to legacy coroutine handling
                return Value(); // nil
            }

        } catch (const std::exception& e) {
            std::cerr << "Error resuming legacy coroutine: " << e.what() << std::endl;
            return Value(); // nil
        }
    }

    bool State::yieldCoroutine(const Vec<Value>& values) {
        // Try C++20 coroutine yield first
        if (currentCoroutine_) {
            CoroutineResult result = yieldFromCoroutine(values);
            return result.success;
        }

        // Fallback to legacy yield
        if (!isCoroutine_ || !fullyMigrated_ || !luaState_) {
            return false;
        }

        try {
            // Set coroutine status to yielded
            luaState_->setStatus(LUA_YIELD);

            // Push yield values onto stack
            for (const auto& value : values) {
                luaState_->push(value);
            }

            return true;

        } catch (const std::exception& e) {
            std::cerr << "Error yielding legacy coroutine: " << e.what() << std::endl;
            return false;
        }
    }

    bool State::isCoroutine() const {
        return isCoroutine_;
    }

    GCRef<Function> State::createClosureFromPrototype(GCRef<Function> prototype) {
        if (!prototype || prototype->getType() != Function::Type::Lua) {
            return GCRef<Function>();
        }

        // Create a new function instance as a closure
        // This ensures each closure has independent upvalue state
        // Create a shared_ptr copy of the code for the new closure
        auto codePtr = std::make_shared<Vec<Instruction>>(prototype->getCode());

        GCRef<Function> closure = Function::createLua(
            codePtr,
            prototype->getConstants(),
            prototype->getPrototypes(),
            prototype->getParamCount(),
            prototype->getLocalCount(),
            prototype->getUpvalueCount(),
            prototype->getIsVariadic()
        );

        // Initialize upvalues for this closure instance
        // Each closure gets its own independent upvalue storage
        u8 nupvalues = prototype->getUpvalueCount();
        for (u8 i = 0; i < nupvalues; ++i) {
            // Create a new upvalue for each closure instance
            // Initialize with nil value - will be set by SETUPVAL instructions
            Value* upvalueLocation = new Value(); // Allocate storage for this upvalue
            GCRef<Upvalue> upvalue = Upvalue::create(upvalueLocation);
            closure->setUpvalue(i, upvalue);
        }

        return closure;
    }

    // Optimized function call for VM CALL instruction
    Value State::callOptimized_(const Value& func, u8 a, u8 b, u8 c, Vec<Value>& registers) {
        if (!func.isFunction()) {
            return Value(); // nil
        }

        auto function = func.asFunction();
        if (!function) {
            return Value(); // nil
        }

        // Fast path for simple function calls
        if (function->getType() == Function::Type::Native) {
            // Native function call - use existing mechanism
            Vec<Value> args;
            // Collect arguments efficiently without extra allocations
            args.reserve(b > 1 ? b - 1 : 0);
            for (u8 i = 1; i < b && (a + i) < registers.size(); i++) {
                args.push_back(registers[a + i]);
            }
            return call(func, args);
        } else {
            // Lua function call - use optimized register-based approach
            return callLuaOptimized_(function, a, b, c, registers);
        }
    }

    // Optimized Lua function call using register-based approach
    Value State::callLuaOptimized_(GCRef<Function> function, u8 a, u8 b, u8 c, Vec<Value>& registers) {
        if (!function || function->getType() != Function::Type::Lua) {
            return Value(); // nil
        }

        // Check for stack overflow (simple depth check)
        static thread_local int callDepth = 0;
        constexpr int MAX_CALL_DEPTH = 1000;

        if (callDepth >= MAX_CALL_DEPTH) {
            throw LuaException("stack overflow (too many nested function calls)");
        }

        // Increment call depth
        callDepth++;

        // Save current execution context
        auto savedFunction = currentFunction_;
        currentFunction_ = function;

        try {
            // Get function code and constants
            const auto& code = function->getCode();
            const auto& constants = function->getConstants();

            // Create optimized register file for the called function
            Vec<Value> calleeRegisters(256, Value()); // Initialize to nil

            // Copy arguments directly from caller registers to callee registers
            // Arguments start at register a+1 in caller, become registers 0,1,2... in callee
            for (u8 i = 1; i < b && (a + i) < registers.size(); i++) {
                calleeRegisters[i - 1] = registers[a + i];
            }

            // Execute function with optimized register context
            // Use the existing VM execution loop
            size_t pc = 0;
            Value result;

            while (pc < code.size()) {
                Instruction instr = code[pc];
                OpCode op = instr.getOpCode();

                // Handle RETURN instruction specially
                if (op == OpCode::RETURN) {
                    u8 a = instr.getA();
                    u8 b = instr.getB();

                    if (b == 1) {
                        result = Value(); // Return nil
                    } else if (b == 2 && a < calleeRegisters.size()) {
                        result = calleeRegisters[a]; // Return single value
                    } else if (a < calleeRegisters.size()) {
                        result = calleeRegisters[a]; // Return first value
                    } else {
                        result = Value(); // Return nil
                    }
                    break;
                }

                // Execute other instructions using existing VM logic
                // This is a simplified version - in a full implementation,
                // we would need to handle all instructions properly
                try {
                    Value instrResult = executeInstructionWithCallInfo_(instr, calleeRegisters, constants, pc);
                    if (!instrResult.isNil() && op == OpCode::RETURN) {
                        result = instrResult;
                        break;
                    }
                } catch (...) {
                    // If instruction execution fails, continue with basic handling
                    // This is a fallback for instructions not handled by executeInstructionWithCallInfo_
                }

                pc++;
            }

            // Restore execution context and decrement call depth
            currentFunction_ = savedFunction;
            callDepth--;

            return result;

        } catch (const LuaException& e) {
            // Restore context on error and decrement call depth
            currentFunction_ = savedFunction;
            callDepth--;
            throw;
        } catch (...) {
            // Restore context on any error and decrement call depth
            currentFunction_ = savedFunction;
            callDepth--;
            throw LuaException("unknown error in function call");
        }
    }

    // Inline Lua function execution to avoid nested VM loops
    Value State::executeLuaFunctionInline_(GCRef<Function> function, const Vec<Value>& args) {
        if (!function || function->getType() != Function::Type::Lua) {
            return Value(); // nil
        }

        // Save current execution context
        auto savedFunction = currentFunction_;
        currentFunction_ = function;

        try {
            // Get function code and constants
            const auto& code = function->getCode();
            const auto& constants = function->getConstants();

            // Create new register file for this function call
            Vec<Value> calleeRegisters(256, Value()); // Initialize to nil

            // Copy arguments to registers (R1, R2, R3, ...)
            for (size_t i = 0; i < args.size() && i + 1 < calleeRegisters.size(); ++i) {
                calleeRegisters[i + 1] = args[i];
            }

            // Execute function with its own register context
            size_t pc = 0;
            while (pc < code.size()) {
                Instruction instr = code[pc];
                OpCode op = instr.getOpCode();

                // Handle RETURN instruction specially
                if (op == OpCode::RETURN) {
                    u8 a = instr.getA();
                    u8 b = instr.getB();

                    // Restore execution context
                    currentFunction_ = savedFunction;

                    if (b == 1) {
                        return Value(); // Return nil
                    } else if (b == 2 && a < calleeRegisters.size()) {
                        return calleeRegisters[a]; // Return single value
                    } else if (a < calleeRegisters.size()) {
                        return calleeRegisters[a]; // Return first value
                    } else {
                        return Value(); // Return nil
                    }
                }

                // Execute other instructions using the same VM logic but with callee registers
                try {
                    Value instrResult = executeInstructionWithCallInfo_(instr, calleeRegisters, constants, pc);
                    if (!instrResult.isNil() && op == OpCode::RETURN) {
                        // Restore execution context
                        currentFunction_ = savedFunction;
                        return instrResult;
                    }
                } catch (...) {
                    // Continue with basic instruction handling
                }

                pc++;
            }

            // Restore execution context
            currentFunction_ = savedFunction;
            return Value(); // Return nil if no explicit return

        } catch (const LuaException& e) {
            // Restore context on error
            currentFunction_ = savedFunction;
            throw;
        } catch (...) {
            // Restore context on any error
            currentFunction_ = savedFunction;
            throw LuaException("unknown error in inline function execution");
        }
    }

    // Enhanced error handling and debugging methods
    void State::setCurrentSourceLocation(const std::string& filename, i32 line) {
        currentSourceFile_ = filename;
        currentSourceLine_ = line;
        if (debugInfo_) {
            debugInfo_->setCurrentFile(filename);
        }
    }

    void State::throwError(const std::string& message) {
        if (!currentSourceFile_.empty() && currentSourceLine_ >= 0) {
            throwError(message, currentSourceFile_, currentSourceLine_);
        } else {
            throw LuaException(message);
        }
    }

    void State::throwError(const std::string& message, const std::string& filename, i32 line) {
        // Generate call stack trace
        std::vector<std::string> callStack;
        if (debugCallStack_) {
            callStack = debugCallStack_->generateStackTrace();
        }

        // Create enhanced exception with full context
        LuaException ex(message, filename, line, debugInfo_->getCurrentFunction(), callStack);
        throw ex;
    }

    void State::throwErrorWithContext(const std::string& message, const std::string& context) {
        std::string enhancedMessage = message;
        if (!context.empty()) {
            enhancedMessage += " (" + context + ")";
        }

        // Generate call stack trace
        std::vector<std::string> callStack;
        if (debugCallStack_) {
            callStack = debugCallStack_->generateStackTrace();
        }

        // Create enhanced exception
        LuaException ex(enhancedMessage, currentSourceFile_, currentSourceLine_,
                       debugInfo_->getCurrentFunction(), callStack);
        ex.setContextInfo(context);
        throw ex;
    }

    void State::pushDebugFrame(const std::string& functionName, const std::string& filename, i32 line) {
        if (debugCallStack_) {
            DebugSourceLocation location(filename.empty() ? currentSourceFile_ : filename,
                                       line < 0 ? currentSourceLine_ : line, -1, functionName);
            debugCallStack_->pushFrame(location, 0); // TODO: Add instruction address
        }

        if (debugInfo_) {
            debugInfo_->setCurrentFunction(functionName);
        }
    }

    void State::popDebugFrame() {
        if (debugCallStack_) {
            debugCallStack_->popFrame();
        }
    }

    void State::setLocalVariableDebugInfo(const std::string& name, const Value& value) {
        if (debugCallStack_) {
            debugCallStack_->setLocalVariable(name, value.toString());
        }
    }
}
