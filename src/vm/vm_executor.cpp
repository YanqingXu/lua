#include "vm_executor.hpp"
#include "debug_hooks.hpp"
#include "table.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_string.hpp"
#include "../gc/core/string_pool.hpp"  // 添加字符串池支持
#include "../api/lua51_gc_api.hpp"
#include "../common/types.hpp"
#include <iostream>
#include <cmath>

namespace Lua {
    
    Value VMExecutor::execute(LuaState* L, GCRef<Function> func, const Vec<Value>& args) {
        if (!func || func->getType() != Function::Type::Lua) {
            throw LuaException("VMExecutor::execute: Invalid Lua function");
        }

        // Following official Lua 5.1 calling convention:
        // 1. Push function and arguments onto stack
        // 2. Call precall to setup CallInfo
        // 3. Execute VM loop
        // 4. Call postcall for cleanup

        // Step 1: Push function onto stack
        L->push(Value(func));

        // Step 2: Push arguments
        for (const auto& arg : args) {
            L->push(arg);
        }

        // Step 3: Setup call using precall (following Lua 5.1 pattern)
        // Get function pointer from stack (function is at top - args.size() - 1)
        Value* funcPtr = &L->get(L->getTop() - static_cast<i32>(args.size()) - 1);
        L->precall(funcPtr, 1);  // Expect 1 result

        // Step 4: Execute VM loop (this is the core VM execution)
        Value result = executeLoop(L);

        // Step 5: Post-call cleanup
        CallInfo* ci = L->getCurrentCI();
        L->postcall(ci ? ci->base : nullptr);

        return result;
    }
    
    Value VMExecutor::executeLoop(LuaState* L) {
        // Reentry point for Lua function calls (following Lua 5.1 pattern)
        reentry:

        // Get current call info
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->isLua()) {
            throw LuaException("VMExecutor::executeLoop: Invalid call info");
        }

        // Get function from call info
        Value funcVal = *(ci->func);
        if (!funcVal.isFunction()) {
            throw LuaException("VMExecutor::executeLoop: Function value is not a function");
        }

        auto func = funcVal.asFunction();
        if (!func || func->getType() != Function::Type::Lua) {
            throw LuaException("VMExecutor::executeLoop: Invalid Lua function in call info");
        }

        // Get function code and constants
        const auto& code = func->getCode();
        const auto& constants = func->getConstants();
        const auto& prototypes = func->getPrototypes();

        // Get base register address
        Value* base = ci->base;

        // Program counter (start from 0 for new calls, restore from saved PC for reentry)
        u32 pc = 0;
        if (ci->savedpc != nullptr) {
            // For reentry, calculate PC from saved instruction pointer
            pc = static_cast<u32>(ci->savedpc - reinterpret_cast<const u32*>(&code[0]));
        }

        // Main execution loop (optimized for performance)
        // Cache frequently accessed values
        const u32 codeSize = static_cast<u32>(code.size());

        while (pc < codeSize) {
            // Fetch instruction (optimized: direct access)
            const Instruction instr = code[pc];
            const OpCode op = instr.getOpCode();

            // Save current PC for potential reentry (only when necessary)
            ci->savedpc = reinterpret_cast<const u32*>(&code[pc]);

            // Performance optimization: Minimize debug overhead
            #ifdef DEBUG_VM_EXECUTION
            debugInstruction(L, instr, pc);
            #endif
            
            switch (op) {
                case OpCode::MOVE:
                    handleMove(L, instr, base);
                    break;
                    
                case OpCode::LOADK:
                    handleLoadK(L, instr, base, constants);
                    break;
                    
                case OpCode::LOADBOOL: {
                    u32 oldPc = pc;
                    handleLoadBool(L, instr, base, pc);
                    if (pc == oldPc) {
                        // PC没有被修改，正常递增
                        break;
                    } else {
                        // PC已经被修改（跳过下一条指令），不再递增
                        continue;
                    }
                }
                    
                case OpCode::LOADNIL:
                    handleLoadNil(L, instr, base);
                    break;
                    
                case OpCode::GETUPVAL:
                    handleGetUpval(L, instr, base);
                    break;
                    
                case OpCode::SETUPVAL:
                    handleSetUpval(L, instr, base);
                    break;
                    
                case OpCode::GETGLOBAL:
                    handleGetGlobal(L, instr, base, constants);
                    break;
                    
                case OpCode::SETGLOBAL:
                    handleSetGlobal(L, instr, base, constants);
                    break;
                    
                case OpCode::GETTABLE:
                    handleGetTable(L, instr, base, constants);
                    break;
                    
                case OpCode::SETTABLE:
                    handleSetTable(L, instr, base, constants);
                    break;
                    
                case OpCode::NEWTABLE:
                    handleNewTable(L, instr, base);
                    break;
                    
                // SELF instruction not implemented in current OpCode set
                    
                case OpCode::ADD:
                    handleAdd(L, instr, base, constants);
                    break;
                    
                case OpCode::SUB:
                    handleSub(L, instr, base, constants);
                    break;
                    
                case OpCode::MUL:
                    handleMul(L, instr, base, constants);
                    break;
                    
                case OpCode::DIV:
                    handleDiv(L, instr, base, constants);
                    break;
                    
                case OpCode::MOD:
                    handleMod(L, instr, base, constants);
                    break;
                    
                case OpCode::POW:
                    handlePow(L, instr, base, constants);
                    break;
                    
                case OpCode::UNM:
                    handleUnm(L, instr, base, constants);
                    break;
                    
                case OpCode::NOT:
                    handleNot(L, instr, base, constants);
                    break;
                    
                case OpCode::LEN:
                    handleLen(L, instr, base, constants);
                    break;
                    
                case OpCode::CONCAT:
                    handleConcat(L, instr, base, constants);
                    break;
                    
                case OpCode::JMP:
                    handleJmp(L, instr, pc);
                    continue; // pc has been modified
                    
                case OpCode::EQ:
                    handleEq(L, instr, base, constants, pc);
                    break;
                    
                case OpCode::LT:
                    handleLt(L, instr, base, constants, pc);
                    break;
                    
                case OpCode::LE:
                    handleLe(L, instr, base, constants, pc);
                    break;
                    
                case OpCode::TEST:
                    handleTest(L, instr, base, pc);
                    break;
                    
                // TESTSET instruction not implemented in current OpCode set
                    
                case OpCode::CALL:
                    // Debug hooks: Placeholder for call hook (simplified implementation)
                    // Full debug hook implementation will be added later

                    if (handleCall(L, instr, base, pc)) {
                        // Function call completed, continue execution
                        // Debug hooks: Placeholder for return hook
                        break; // 使用break而不是continue，让PC正常递增
                    } else {
                        // Reentry needed (for Lua function calls) - use goto instead of recursion
                        goto reentry;
                    }

                // TAILCALL instruction not implemented in current OpCode set
                    
                case OpCode::RETURN:
                    // Debug hooks: Placeholder for return hook (simplified implementation)
                    // Full debug hook implementation will be added later

                    if (handleReturn(L, instr, base)) {
                        // Function returned, get result from stack
                        return L->get(-1);
                    }
                    break;
                    
                case OpCode::CLOSURE:
                    handleClosure(L, instr, base, prototypes, pc);
                    break;
                    
                case OpCode::FORLOOP: {
                    u32 oldPc = pc;
                    handleForLoop(L, instr, base, pc);
                    if (pc != oldPc) {
                        // PC was modified (jump occurred), skip normal increment
                        continue;
                    } else {
                        // PC was not modified (loop ended), use normal increment
                        break;
                    }
                }
                    
                case OpCode::FORPREP:
                    handleForPrep(L, instr, base, pc);
                    continue; // pc has been modified
                    
                // === 官方Lua 5.1缺失的操作码实现 ===

                case OpCode::SELF:
                    handleSelf(L, instr, base, constants);
                    break;

                case OpCode::TESTSET:
                    handleTestSet(L, instr, base, pc);
                    break;

                case OpCode::TAILCALL:
                    if (handleTailCall(L, instr, base)) {
                        return L->get(-1);
                    }
                    break;

                case OpCode::TFORLOOP:
                    handleTForLoop(L, instr, base, pc);
                    break;

                case OpCode::SETLIST:
                    handleSetList(L, instr, base);
                    break;

                case OpCode::CLOSE:
                    handleClose(L, instr, base);
                    break;

                case OpCode::VARARG:
                    handleVararg(L, instr, base);
                    break;

                default:
                    vmError(L, "Unknown opcode");
                    break;
            }
            
            pc++;
        }
        
        // If we reach here without explicit return, return nil
        return Value();
    }
    
    // Helper function implementations
    Value* VMExecutor::getRK(Value* base, const Vec<Value>& constants, u16 rk) {
        if (isConstant(rk)) {
            u16 idx = getConstantIndex(rk);
            if (idx < constants.size()) {
                // Return pointer to constant (note: this is not ideal for constants)
                // In a real implementation, we'd return the value directly
                static Value temp;
                temp = constants[idx];
                return &temp;
            }
            return nullptr;
        } else {
            return &base[rk];
        }
    }
    
    void VMExecutor::vmError(LuaState* L, const char* msg) {
        throw LuaException(std::string("VM Error: ") + msg);
    }
    
    void VMExecutor::typeError(LuaState* L, const Value& val, const char* op) {
        std::string msg = "attempt to ";
        msg += op;
        msg += " a ";
        msg += val.getTypeName();
        msg += " value";
        vmError(L, msg.c_str());
    }
    
    void VMExecutor::debugInstruction(LuaState* L, Instruction instr, u32 pc) {
        #ifdef DEBUG_VM_EXECUTION
        std::cout << "[VM] PC=" << pc << " OpCode=" << static_cast<int>(instr.getOpCode())
                  << " A=" << static_cast<int>(instr.getA())
                  << " B=" << static_cast<int>(instr.getB())
                  << " C=" << static_cast<int>(instr.getC()) << std::endl;
        #endif
    }

    // Basic instruction implementations
    void VMExecutor::handleMove(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        base[a] = base[b];
    }

    void VMExecutor::handleLoadK(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u32 bx = instr.getBx();
        if (bx < constants.size()) {
            base[a] = constants[bx];
        } else {
            base[a] = Value(); // nil
        }
    }

    void VMExecutor::handleLoadBool(LuaState* L, Instruction instr, Value* base, u32& pc) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        base[a] = Value(b != 0);
        if (c != 0) {
            pc++; // Skip next instruction
        }
    }

    void VMExecutor::handleLoadNil(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        for (u8 i = 0; i <= b; i++) {
            base[a + i] = Value(); // nil
        }
    }

    void VMExecutor::handleGetGlobal(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u32 bx = instr.getBx();

        if (bx < constants.size()) {
            Value key = constants[bx];

            if (key.isString()) {
                // 使用Lua 5.1兼容的字符串创建函数进行优化
                const Str& keyStr = key.asString();
                GCString* gcKey = luaS_newlstr(L, keyStr.c_str(), keyStr.length());
                // 尝试从全局环境获取变量
                Value globalValue = L->getGlobal(gcKey);

                base[a] = globalValue;
            } else {
                base[a] = Value(); // nil
            }
        } else {
            base[a] = Value(); // nil
        }
    }

    void VMExecutor::handleSetGlobal(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 bx = instr.getBx();
        if (bx < constants.size()) {
            Value key = constants[bx];
            if (key.isString()) {
                // For now, do nothing since LuaState::setGlobal is not fully implemented
                // TODO: Implement proper global variable setting
                (void)base[a]; // Suppress unused variable warning
            }
        }
    }

    void VMExecutor::handleAdd(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (vb && vc && vb->isNumber() && vc->isNumber()) {
            base[a] = Value(vb->asNumber() + vc->asNumber());
        } else {
            typeError(L, vb ? *vb : Value(), "perform arithmetic on");
        }
    }

    bool VMExecutor::handleCall(LuaState* L, Instruction instr, Value* base, u32& pc) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value func = base[a];

        if (!func.isFunction()) {
            // Provide more detailed error information
            std::string errorMsg = "attempt to call a non-function value (";
            errorMsg += func.getTypeName();
            errorMsg += ")";
            if (func.isTable()) {
                errorMsg += " - this might be a register allocation issue in the compiler";
            }
            vmError(L, errorMsg.c_str());
            return false;
        }

        // 处理参数数量
        u16 actualArgCount;
        if (b == 0) {
            // B=0: 参数从寄存器a+1开始到栈顶
            actualArgCount = L->getTop() - a - 1;
        } else {
            actualArgCount = b - 1; // B包含函数本身，所以参数数量是B-1
        }

        // For now, implement basic function call
        // In a full implementation, this would use L->precall() and handle reentry
        Vec<Value> args;
        for (u16 i = 1; i <= actualArgCount; i++) {
            args.push_back(base[a + i]);
        }

        // Get function object
        auto functionObj = func.asFunction();
        if (!functionObj) {
            vmError(L, "invalid function object");
            return false;
        }

        // Handle different function types
        if (functionObj->getType() == Function::Type::Lua) {
            // Lua function: need to set up new call frame and continue execution
            // This requires proper precall setup and VM reentry
            // For now, return false to indicate reentry needed
            return false;
        } else if (functionObj->getType() == Function::Type::Native) {
            // Native function: call directly
            if (functionObj->isNativeLegacy()) {
                // Legacy native function (single return value)
                auto legacyFn = functionObj->getNativeLegacy();
                if (legacyFn) {
                    // 保存寄存器内容，因为栈操作可能会破坏它们
                    int maxRegister = a + actualArgCount + 5;  // 额外保存一些寄存器
                    Vec<Value> savedRegisters;
                    for (int i = 0; i < maxRegister; i++) {
                        savedRegisters.push_back(base[i]);
                    }

                    // 准备参数：将参数推入栈中
                    int oldTop = L->getTop();
                    for (u8 i = 1; i <= actualArgCount; i++) {
                        L->push(savedRegisters[a + i]);  // 使用保存的值
                    }

                    // 调用legacy函数
                    Value result = legacyFn(L, actualArgCount);

                    // 恢复寄存器内容
                    for (int i = 0; i < maxRegister; i++) {
                        base[i] = savedRegisters[i];
                    }

                    // 处理返回值
                    if (c == 0) {
                        // C=0: 返回值数量由函数确定，对于legacy函数通常是1个
                        base[a] = result;
                    } else if (c == 1) {
                        // C=1: 不需要返回值，丢弃结果
                    } else {
                        // C>1: 需要c-1个返回值
                        base[a] = result;
                        // 对于legacy函数，额外的返回值设为nil
                        for (u16 i = 1; i < c - 1; i++) {
                            base[a + i] = Value();
                        }
                    }

                    // 恢复栈状态
                    L->setTop(oldTop);
                    return true;
                }
            } else {
                // Modern native function (multiple return values)
                auto nativeFn = functionObj->getNative();
                if (nativeFn) {
                    // 保存寄存器内容
                    int maxRegister = a + actualArgCount + 5;
                    Vec<Value> savedRegisters;
                    for (int i = 0; i < maxRegister; i++) {
                        savedRegisters.push_back(base[i]);
                    }

                    // 准备参数：将参数推入栈中
                    int oldTop = L->getTop();
                    for (u8 i = 1; i <= actualArgCount; i++) {
                        L->push(savedRegisters[a + i]);
                    }

                    // 调用native函数
                    i32 nresults = nativeFn(L);

                    // 恢复寄存器内容
                    for (int i = 0; i < maxRegister; i++) {
                        base[i] = savedRegisters[i];
                    }

                    // 处理返回值
                    if (c == 0) {
                        // C=0: 返回值数量由函数确定
                        for (i32 i = 0; i < nresults; i++) {
                            base[a + i] = L->get(-nresults + i);
                        }
                    } else if (c == 1) {
                        // C=1: 不需要返回值，丢弃结果
                    } else {
                        // C>1: 需要c-1个返回值
                        i32 wantedResults = c - 1;
                        for (i32 i = 0; i < wantedResults; i++) {
                            if (i < nresults) {
                                base[a + i] = L->get(-nresults + i);
                            } else {
                                base[a + i] = Value(); // nil for missing results
                            }
                        }
                    }

                    // 恢复栈状态
                    L->setTop(oldTop);
                    return true;
                }
            }

            // 如果到这里，说明native函数调用失败
            vmError(L, "failed to call native function");
            return false;
        } else {
            // 未知函数类型
            vmError(L, "unknown function type");
            return false;
        }
    }

    bool VMExecutor::handleReturn(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();
        u16 b = instr.getB();

        if (b == 1) {
            // No return values
            L->push(Value()); // nil
        } else if (b == 2) {
            // Single return value
            L->push(base[a]);
        } else {
            // Multiple return values - for now, return first value
            L->push(base[a]);
        }

        return true; // Function completed
    }



    // Placeholder implementations for missing handle methods
    void VMExecutor::handleGetUpval(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();
        base[a] = Value(); // Return nil for now
    }

    void VMExecutor::handleSetUpval(LuaState* L, Instruction instr, Value* base) {
        // Do nothing for now
        (void)L; (void)instr; (void)base;
    }

    void VMExecutor::handleGetTable(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // Get table from register B
        Value table = base[b];
        if (!table.isTable()) {
            vmError(L, "attempt to index a non-table value");
            return;
        }

        // Get key from register/constant C
        Value* key = getRK(base, constants, c);
        if (!key) {
            vmError(L, "invalid key for table access");
            return;
        }

        // Get value from table
        Value result = table.asTable()->get(*key);
        base[a] = result;
    }

    void VMExecutor::handleSetTable(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // Get table from register A
        Value table = base[a];
        if (!table.isTable()) {
            // Error: attempt to index a non-table value
            throw LuaException("attempt to index a non-table value");
        }

        // Get key (B can be register or constant)
        Value* key = getRK(base, constants, b);
        if (!key) {
            throw LuaException("invalid key in SETTABLE");
        }

        // Get value (C can be register or constant)
        Value* value = getRK(base, constants, c);
        if (!value) {
            throw LuaException("invalid value in SETTABLE");
        }

        // Set the value in the table
        table.asTable()->set(*key, *value);

        (void)L; // Suppress unused parameter warning
    }

    void VMExecutor::handleNewTable(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();

        // 创建新表前检查GC - 表创建是重要的内存分配点
        luaC_checkGC(L);

        // 创建新的表对象
        GCRef<Table> newTable = make_gc_table();
        base[a] = Value(newTable);
    }

    void VMExecutor::handleSub(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        if (vb && vc && vb->isNumber() && vc->isNumber()) {
            base[a] = Value(vb->asNumber() - vc->asNumber());
        } else {
            base[a] = Value(); // nil
        }
        (void)L;
    }

    void VMExecutor::handleMul(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        if (vb && vc && vb->isNumber() && vc->isNumber()) {
            base[a] = Value(vb->asNumber() * vc->asNumber());
        } else {
            base[a] = Value(); // nil
        }
        (void)L;
    }

    void VMExecutor::handleDiv(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        if (vb && vc && vb->isNumber() && vc->isNumber() && vc->asNumber() != 0.0) {
            base[a] = Value(vb->asNumber() / vc->asNumber());
        } else {
            base[a] = Value(); // nil
        }
        (void)L;
    }

    void VMExecutor::handleMod(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        if (vb && vc && vb->isNumber() && vc->isNumber() && vc->asNumber() != 0.0) {
            base[a] = Value(std::fmod(vb->asNumber(), vc->asNumber()));
        } else {
            base[a] = Value(); // nil
        }
        (void)L;
    }

    void VMExecutor::handlePow(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        if (vb && vc && vb->isNumber() && vc->isNumber()) {
            base[a] = Value(std::pow(vb->asNumber(), vc->asNumber()));
        } else {
            base[a] = Value(); // nil
        }
        (void)L;
    }

    void VMExecutor::handleUnm(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        Value* vb = getRK(base, constants, b);
        if (vb && vb->isNumber()) {
            base[a] = Value(-vb->asNumber());
        } else {
            base[a] = Value(); // nil
        }
        (void)L;
    }

    void VMExecutor::handleNot(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        Value* vb = getRK(base, constants, b);
        if (vb) {
            base[a] = Value(!vb->asBoolean());
        } else {
            base[a] = Value(true); // not nil = true
        }
        (void)L;
    }

    void VMExecutor::handleLen(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        // OP_LEN: R(A) := length of R(B)
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        u16 b = instr.getB();
        Value* vb = getRK(base, constants, b);

        if (!vb) {
            base[a] = Value(0.0);
            return;
        }

        if (vb->isString()) {
            // String length
            base[a] = Value(static_cast<LuaNumber>(vb->asString().length()));
        } else if (vb->isTable()) {
            // Table length (array part)
            auto table = vb->asTable();
            if (table) {
                // Get the length of the array part of the table
                // In Lua 5.1, this is the number of consecutive integer keys starting from 1
                usize length = table->length();
                base[a] = Value(static_cast<LuaNumber>(length));
            } else {
                base[a] = Value(0.0);
            }
        } else {
            // For other types, try __len metamethod or return 0
            // For now, just return 0 (metamethods not fully implemented)
            base[a] = Value(0.0);
        }

        (void)L;
    }

    void VMExecutor::handleConcat(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        // Simple concatenation of two values
        if (b < 256 && c < 256) {
            Str result = base[b].toString() + base[c].toString();
            base[a] = Value(result);
        } else {
            base[a] = Value(""); // empty string
        }
        (void)L; (void)constants;
    }

    void VMExecutor::handleJmp(LuaState* L, Instruction instr, u32& pc) {
        i16 sbx = instr.getSBx();
        pc += sbx;
        (void)L;
    }

    void VMExecutor::handleEq(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc) {
        // EQ instruction for our compiler: R(A) = (RK(B) == RK(C))
        // This differs from standard Lua 5.1 EQ which is a conditional jump instruction
        // Our compiler expects the result to be stored in register A for subsequent TEST
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        bool equal = false;
        if (vb && vc) {
            if (vb->type() == vc->type()) {
                if (vb->isNumber()) equal = (vb->asNumber() == vc->asNumber());
                else if (vb->isString()) equal = (vb->asString() == vc->asString());
                else if (vb->isBoolean()) equal = (vb->asBoolean() == vc->asBoolean());
                else if (vb->isNil()) equal = true;
            }
        }

        // Store the comparison result in register A
        base[a] = Value(equal);

        (void)L; (void)pc;
    }

    void VMExecutor::handleLt(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc) {
        // LT instruction for our compiler: R(A) = (RK(B) < RK(C))
        // This differs from standard Lua 5.1 LT which is a conditional jump instruction
        // Our compiler expects the result to be stored in register A for subsequent TEST
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        bool less = false;
        if (vb && vc && vb->isNumber() && vc->isNumber()) {
            less = (vb->asNumber() < vc->asNumber());
        }

        // Store the comparison result in register A
        base[a] = Value(less);

        (void)L; (void)pc;
    }

    void VMExecutor::handleLe(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc) {
        // LE instruction for our compiler: R(A) = (RK(B) <= RK(C))
        // This differs from standard Lua 5.1 LE which is a conditional jump instruction
        // Our compiler expects the result to be stored in register A for subsequent TEST
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();
        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);
        bool lessEqual = false;
        if (vb && vc && vb->isNumber() && vc->isNumber()) {
            lessEqual = (vb->asNumber() <= vc->asNumber());
            std::cout << "[LE] " << vb->asNumber() << " <= " << vc->asNumber() << " = " << (lessEqual ? "true" : "false") << std::endl;
        }

        // Store the comparison result in register A
        base[a] = Value(lessEqual);
        std::cout << "[LE] Result stored in R(" << (int)a << ") = " << (lessEqual ? "true" : "false") << std::endl;

        (void)L; (void)pc;
    }

    void VMExecutor::handleTest(LuaState* L, Instruction instr, Value* base, u32& pc) {
        // OP_TEST: if not (R(A) <=> C) then pc++
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        u16 c = instr.getC();

        // Get the value to test
        Value& testValue = base[a];

        // Check if value is false/nil (l_isfalse equivalent)
        bool isFalse = testValue.isNil() || (testValue.isBoolean() && !testValue.asBoolean());

        std::cout << "[TEST] R(" << (int)a << ") = ";
        if (testValue.isBoolean()) {
            std::cout << (testValue.asBoolean() ? "true" : "false");
        } else if (testValue.isNil()) {
            std::cout << "nil";
        } else {
            std::cout << "other";
        }
        std::cout << ", isFalse=" << (isFalse ? "true" : "false") << ", C=" << c << std::endl;

        // For for-loops: we want to exit when condition is false
        // C=0 means: if condition is false, skip next instruction (skip JMP to exit)
        // So: if condition is false (isFalse=true), skip JMP (continue loop)
        //     if condition is true (isFalse=false), execute JMP (exit loop)
        bool shouldSkip = (isFalse == (c == 0));
        std::cout << "[TEST] shouldSkip=" << (shouldSkip ? "true" : "false") << std::endl;

        if (shouldSkip) {
            // Skip next instruction
            pc++;
            std::cout << "[TEST] Skipping next instruction, PC now=" << pc << std::endl;
        } else {
            std::cout << "[TEST] Executing next instruction" << std::endl;
        }

        (void)L;
    }

    void VMExecutor::handleClosure(LuaState* L, Instruction instr, Value* base, const Vec<GCRef<Function>>& prototypes, u32& pc) {
        u8 a = instr.getA();
        u16 bx = instr.getBx();
        if (bx < prototypes.size()) {
            base[a] = Value(prototypes[bx]);
        } else {
            base[a] = Value(); // nil
        }
        (void)L; (void)pc;
    }

    void VMExecutor::handleForLoop(LuaState* L, Instruction instr, Value* base, u32& pc) {
        // OP_FORLOOP: R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        i16 sbx = instr.getSBx();

        // Get loop variables: index, limit, step
        Value& index = base[a];      // R(A) - current index
        Value& limit = base[a + 1];  // R(A+1) - limit
        Value& step = base[a + 2];   // R(A+2) - step

        // Ensure all values are numbers
        if (!index.isNumber() || !limit.isNumber() || !step.isNumber()) {
            vmError(L, "for loop variables must be numbers");
            return;
        }

        // Increment index: R(A) += R(A+2)
        f64 newIndex = index.asNumber() + step.asNumber();
        index = Value(newIndex);

        // Check loop condition based on step direction
        bool continueLoop = false;
        f64 stepVal = step.asNumber();
        f64 limitVal = limit.asNumber();

        if (stepVal > 0) {
            // Positive step: continue if index <= limit
            continueLoop = (newIndex <= limitVal);
        } else {
            // Negative step: continue if index >= limit
            continueLoop = (newIndex >= limitVal);
        }

        if (continueLoop) {
            // Jump back to loop body: pc += sBx
            pc = static_cast<u32>(static_cast<i32>(pc) + sbx);
            // Update external index: R(A+3) = R(A)
            base[a + 3] = index;
        }
        // If loop ends, continue to next instruction (pc will be incremented normally)
    }

    void VMExecutor::handleForPrep(LuaState* L, Instruction instr, Value* base, u32& pc) {
        // OP_FORPREP: R(A)-=R(A+2); pc+=sBx
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        i16 sbx = instr.getSBx();

        // Get loop variables: initial, limit, step
        Value& initial = base[a];      // R(A) - initial value
        Value& limit = base[a + 1];    // R(A+1) - limit
        Value& step = base[a + 2];     // R(A+2) - step

        // Ensure all values are numbers (type coercion)
        if (!initial.isNumber()) {
            if (initial.isString()) {
                try {
                    f64 num = std::stod(initial.toString());
                    initial = Value(num);
                } catch (...) {
                    vmError(L, "for initial value must be a number");
                    return;
                }
            } else {
                vmError(L, "for initial value must be a number");
                return;
            }
        }

        if (!limit.isNumber()) {
            if (limit.isString()) {
                try {
                    f64 num = std::stod(limit.toString());
                    limit = Value(num);
                } catch (...) {
                    vmError(L, "for limit must be a number");
                    return;
                }
            } else {
                vmError(L, "for limit must be a number");
                return;
            }
        }

        if (!step.isNumber()) {
            if (step.isString()) {
                try {
                    f64 num = std::stod(step.toString());
                    step = Value(num);
                } catch (...) {
                    vmError(L, "for step must be a number");
                    return;
                }
            } else {
                vmError(L, "for step must be a number");
                return;
            }
        }

        // Prepare for loop: R(A) -= R(A+2)
        f64 preparedIndex = initial.asNumber() - step.asNumber();
        initial = Value(preparedIndex);

        // Jump to FORLOOP instruction: pc += sBx
        pc = static_cast<u32>(static_cast<i32>(pc) + sbx);
    }

    // === 新增的官方Lua 5.1操作码实现 ===

    void VMExecutor::handleSelf(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        // OP_SELF: R(A+1) := R(B); R(A) := R(B)[RK(C)]
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // R(A+1) := R(B)
        base[a + 1] = base[b];

        // R(A) := R(B)[RK(C)] - 简化实现，暂时返回nil
        base[a] = Value(); // nil

        (void)L; (void)constants; (void)c;
    }

    void VMExecutor::handleTestSet(LuaState* L, Instruction instr, Value* base, u32& pc) {
        // OP_TESTSET: if (R(B) <=> C) then R(A) := R(B) else pc++
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        bool test = base[b].asBoolean();
        if (test == (c != 0)) {
            base[a] = base[b];
        } else {
            pc++; // Skip next instruction
        }

        (void)L;
    }

    void VMExecutor::handleTForLoop(LuaState* L, Instruction instr, Value* base, u32& pc) {
        // OP_TFORLOOP: R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));
        //              if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++
        u8 a = instr.getA();
        u16 c = instr.getC();

        // 简化实现：暂时跳过下一条指令
        pc++;

        (void)L; (void)base; (void)a; (void)c;
    }

    void VMExecutor::handleSetList(LuaState* L, Instruction instr, Value* base) {
        // OP_SETLIST: R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // 简化实现：暂时不执行任何操作
        (void)L; (void)base; (void)a; (void)b; (void)c;
    }

    void VMExecutor::handleClose(LuaState* L, Instruction instr, Value* base) {
        // OP_CLOSE: close all variables in the stack up to (>=) R(A)
        u8 a = instr.getA();

        // 简化实现：暂时不执行任何操作
        (void)L; (void)base; (void)a;
    }

    void VMExecutor::handleVararg(LuaState* L, Instruction instr, Value* base) {
        // OP_VARARG: R(A), R(A+1), ..., R(A+B-1) = vararg
        u8 a = instr.getA();
        u16 b = instr.getB();

        // 简化实现：将所有目标寄存器设为nil
        if (b == 0) {
            // 如果B为0，使用实际的vararg数量
            // 暂时不实现，只设置一个nil
            base[a] = Value(); // nil
        } else {
            // 设置B-1个寄存器为nil
            for (int i = 0; i < b - 1; i++) {
                base[a + i] = Value(); // nil
            }
        }

        (void)L;
    }

    bool VMExecutor::handleTailCall(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        (void)L; (void)a; (void)b; (void)base;
        return true;
    }

} // namespace Lua
