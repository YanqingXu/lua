#include "vm_executor.hpp"
#include "debug_hooks.hpp"
#include "table.hpp"
#include "metamethod_manager.hpp"  // 添加元方法支持
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_string.hpp"
#include "../gc/core/string_pool.hpp"  // 添加字符串池支持
#include "../api/lua51_gc_api.hpp"
#include "../common/types.hpp"
#include <iostream>
#include <cmath>

namespace Lua {
    
    Value VMExecutor::executeInContext(LuaState* L, GCRef<Function> func, const Vec<Value>& args) {
        // 在当前执行上下文中执行函数，不干扰主执行栈的寄存器布局
        // 这是符合 Lua 5.1 标准的嵌套函数调用实现

        // 简化实现：直接执行函数并返回结果
        // 不使用复杂的栈操作，避免干扰主执行上下文
        try {
            // 使用原有的 execute 方法，但确保不干扰当前寄存器状态
            return execute(L, func, args);
        } catch (const std::exception& e) {
            throw;
        }
    }

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
        i32 funcIndex = L->getTop() - static_cast<i32>(args.size()) - 1;
        Value* funcPtr = &L->get(funcIndex);

        // Function and arguments setup completed

        L->precall(funcPtr, 1);  // Expect 1 result

        // Step 4: Execute VM loop (this is the core VM execution)
        Value result = executeLoop(L);

        // Step 5: Post-call cleanup
        // 关键修复：不需要在这里调用 postcall，因为 executeLoop 中的 RETURN 处理已经调用了
        // 双重调用 postcall 会破坏返回值的正确位置

        return result;
    }
    
    Value VMExecutor::executeLoop(LuaState* L) {
        // 按照 Lua 5.1 的方式跟踪嵌套调用深度
        // 初始值应该是 1，因为我们已经在执行一个 Lua 函数
        int nexeccalls = 1;

        // Reentry point for Lua function calls (following Lua 5.1 pattern)
        reentry:

        // Get current call info
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->isLua()) {
            std::cout << "[EXECUTE_LOOP] No Lua CI found, returning" << std::endl;
            throw LuaException("VMExecutor::executeLoop: Invalid call info");
        }

        // Call frame validation completed

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

        // Base register setup completed

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
                    // 关键修复：按照 Lua 5.1 的方式，在调用 precall 之前保存下一条指令的 PC
                    // 这样当函数返回时，能够从正确的位置继续执行
                    L->setSavedPC(reinterpret_cast<const u32*>(&code[pc + 1]));

                    if (handleCall(L, instr, base, pc)) {
                        // Function call completed, continue execution
                        // Debug hooks: Placeholder for return hook
                        break; // 使用break而不是continue，让PC正常递增
                    } else {
                        // Lua function call: 按照 Lua 5.1 的方式处理
                        nexeccalls++;  // 递增嵌套调用计数
                        goto reentry;  // 重新进入执行循环
                    }

                // TAILCALL instruction not implemented in current OpCode set
                    
                case OpCode::RETURN: {
                    // Debug hooks: Placeholder for return hook (simplified implementation)
                    // Full debug hook implementation will be added later

                    u8 a = instr.getA();
                    u16 b = instr.getB();

                    // Following Lua 5.1 OP_RETURN implementation
                    Value* ra = base + a;

                    // Debug output removed for cleaner execution

                    // Set top based on B parameter (following Lua 5.1 logic)
                    if (b != 0) {
                        // Set top to ra + b - 1 (following Lua 5.1: if (b != 0) L->top = ra+b-1;)
                        // This sets the top to point after the last return value
                        Value* newTop = ra + b - 1;
                        // Set top to point after the last return value

                        // Directly set the top pointer instead of using setTop
                        // This is more direct and matches Lua 5.1 exactly
                        L->setTopDirect(newTop);
                    }

                    // Call postcall to handle return values properly
                    // postcall will copy the return value from ra to the function position
                    L->postcall(ra);

                    // 按照 Lua 5.1 的方式处理返回
                    --nexeccalls;

                    if (nexeccalls == 0) {
                        // 最顶层调用，返回结果
                        if (L->getTop() > 0) {
                            return L->get(-1);  // Get the top value
                        }
                        return Value();  // No result
                    } else {
                        // 还有更多嵌套调用，继续执行
                        goto reentry;
                    }
                }
                    
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

                // Global variable retrieved successfully

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
                // Set global variable - use same method as GETGLOBAL
                const Str& keyStr = key.asString();
                GCString* gcKey = luaS_newlstr(L, keyStr.c_str(), keyStr.length());
                L->setGlobal(gcKey, base[a]);
            }
        }
    }

    void VMExecutor::handleAdd(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in ADD");
            return;
        }

        // Following official Lua 5.1 arith_op pattern:
        // First try direct numeric operation
        if (vb->isNumber() && vc->isNumber()) {
            double result = vb->asNumber() + vc->asNumber();
            base[a] = Value(result);
            return;
        }

        // If not both numbers, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Add, *vb, *vc);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
    }

    bool VMExecutor::handleCall(LuaState* L, Instruction instr, Value* base, u32& pc) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value func = base[a];

        // Function call setup

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
            // Lua function: 完全按照 Lua 5.1 官方实现
            // 参考 lvm.c 中 OP_CALL 的处理方式

            // 设置调用帧
            L->precall(&base[a], c == 0 ? -1 : c);

            // 按照 Lua 5.1 的方式：nexeccalls++ 然后 goto reentry
            // 这里我们返回 false 来表示需要重新进入执行循环
            // executeLoop 会处理 nexeccalls 的递增
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
        // This function is no longer used since RETURN is handled directly in executeLoop
        // Keeping it for compatibility but it should not be called
        (void)L; (void)instr; (void)base;
        return true;
    }



    // Upvalue system implementations
    void VMExecutor::handleGetUpval(LuaState* L, Instruction instr, Value* base) {
        // OP_GETUPVAL: R(A) := UpValue[B]
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        u16 b = instr.getB();

        // Get current function from call stack
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->func || !ci->func->isFunction()) {
            vmError(L, "invalid function context in GETUPVAL");
            return;
        }

        auto func = ci->func->asFunction();
        if (func->getType() != Function::Type::Lua) {
            vmError(L, "attempt to access upvalue in native function");
            return;
        }

        // Check upvalue index bounds
        if (b >= func->getUpvalueCount()) {
            vmError(L, "upvalue index out of range");
            return;
        }

        // Get the upvalue
        GCRef<Upvalue> upvalue = func->getUpvalue(b);
        if (!upvalue) {
            vmError(L, "invalid upvalue reference");
            return;
        }

        // Get value from upvalue (handles both open and closed states)
        base[a] = upvalue->getValue();
    }

    void VMExecutor::handleSetUpval(LuaState* L, Instruction instr, Value* base) {
        // OP_SETUPVAL: UpValue[B] := R(A)
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        u16 b = instr.getB();

        // Get current function from call stack
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->func || !ci->func->isFunction()) {
            vmError(L, "invalid function context in SETUPVAL");
            return;
        }

        auto func = ci->func->asFunction();
        if (func->getType() != Function::Type::Lua) {
            vmError(L, "attempt to access upvalue in native function");
            return;
        }

        // Check upvalue index bounds
        if (b >= func->getUpvalueCount()) {
            vmError(L, "upvalue index out of range");
            return;
        }

        // Get the upvalue
        GCRef<Upvalue> upvalue = func->getUpvalue(b);
        if (!upvalue) {
            vmError(L, "invalid upvalue reference");
            return;
        }

        // Set value to upvalue with write barrier support
        upvalue->setValueWithBarrier(base[a], L);
    }

    void VMExecutor::handleGetTable(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        // OP_GETTABLE: R(A) := R(B)[RK(C)]
        // Following official Lua 5.1 implementation with __index metamethod support
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

        // If result is nil, try __index metamethod
        if (result.isNil()) {
            try {
                // Try __index metamethod
                Value indexResult = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Index, table, *key);
                base[a] = indexResult;
            } catch (const LuaException& e) {
                // No __index metamethod, use the nil result
                base[a] = result;
            }
        } else {
            // Direct table access succeeded
            base[a] = result;
        }
    }

    void VMExecutor::handleSetTable(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        // OP_SETTABLE: R(A)[RK(B)] := RK(C)
        // Following official Lua 5.1 implementation with __newindex metamethod support
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // Get table from register A
        Value table = base[a];
        if (!table.isTable()) {
            vmError(L, "attempt to index a non-table value");
            return;
        }

        // Get key (B can be register or constant)
        Value* key = getRK(base, constants, b);
        if (!key) {
            vmError(L, "invalid key in SETTABLE");
            return;
        }

        // Get value (C can be register or constant)
        Value* value = getRK(base, constants, c);
        if (!value) {
            vmError(L, "invalid value in SETTABLE");
            return;
        }

        // Check if key already exists in table
        Value existingValue = table.asTable()->get(*key);

        if (existingValue.isNil()) {
            // Key doesn't exist, try __newindex metamethod
            try {
                // Try __newindex metamethod (using binary call with table and key, then set value)
                Value newIndexHandler = MetaMethodManager::getMetaMethod(table, MetaMethod::NewIndex);
                if (!newIndexHandler.isNil()) {
                    // Call the __newindex metamethod
                    // For now, simplified implementation - just fall through to direct assignment
                    // A full implementation would call the metamethod function
                }
            } catch (const LuaException& e) {
                // No __newindex metamethod, fall through to direct table assignment
            }
        }

        // Direct table assignment (either key exists or no __newindex metamethod)
        table.asTable()->set(*key, *value);

        // Check GC after table modification
        luaC_checkGC(L);
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

        if (!vb || !vc) {
            vmError(L, "invalid operands in SUB");
            return;
        }

        // Following official Lua 5.1 arith_op pattern:
        // First try direct numeric operation
        if (vb->isNumber() && vc->isNumber()) {
            double result = vb->asNumber() - vc->asNumber();
            base[a] = Value(result);
            return;
        }

        // If not both numbers, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Sub, *vb, *vc);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
    }

    void VMExecutor::handleMul(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in MUL");
            return;
        }

        // Following official Lua 5.1 arith_op pattern:
        // First try direct numeric operation
        if (vb->isNumber() && vc->isNumber()) {
            double result = vb->asNumber() * vc->asNumber();
            base[a] = Value(result);
            return;
        }

        // If not both numbers, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Mul, *vb, *vc);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
    }

    void VMExecutor::handleDiv(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in DIV");
            return;
        }

        // Following official Lua 5.1 arith_op pattern:
        // First try direct numeric operation
        if (vb->isNumber() && vc->isNumber()) {
            double divisor = vc->asNumber();
            if (divisor == 0.0) {
                vmError(L, "attempt to divide by zero");
                return;
            }
            double result = vb->asNumber() / divisor;
            base[a] = Value(result);
            return;
        }

        // If not both numbers, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Div, *vb, *vc);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
    }

    void VMExecutor::handleMod(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in MOD");
            return;
        }

        // Following official Lua 5.1 arith_op pattern:
        // First try direct numeric operation
        if (vb->isNumber() && vc->isNumber()) {
            double divisor = vc->asNumber();
            if (divisor == 0.0) {
                vmError(L, "attempt to perform modulo by zero");
                return;
            }
            double result = std::fmod(vb->asNumber(), divisor);
            base[a] = Value(result);
            return;
        }

        // If not both numbers, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Mod, *vb, *vc);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
    }

    void VMExecutor::handlePow(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in POW");
            return;
        }

        // Following official Lua 5.1 arith_op pattern:
        // First try direct numeric operation
        if (vb->isNumber() && vc->isNumber()) {
            double result = std::pow(vb->asNumber(), vc->asNumber());
            base[a] = Value(result);
            return;
        }

        // If not both numbers, try metamethod
        try {
            Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Pow, *vb, *vc);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
    }

    void VMExecutor::handleUnm(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        u8 a = instr.getA();
        u16 b = instr.getB();

        Value* vb = getRK(base, constants, b);

        if (!vb) {
            vmError(L, "invalid operand in UNM");
            return;
        }

        // Following official Lua 5.1 pattern:
        // First try direct numeric operation
        if (vb->isNumber()) {
            double result = -vb->asNumber();
            base[a] = Value(result);
            return;
        }

        // If not a number, try metamethod
        try {
            Value result = MetaMethodManager::callUnaryMetaMethod(L, MetaMethod::Unm, *vb);
            base[a] = result;
        } catch (const LuaException& e) {
            // No metamethod found or metamethod failed
            typeError(L, *vb, "perform arithmetic on");
        }
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
            // For other types, try __len metamethod
            try {
                Value result = MetaMethodManager::callUnaryMetaMethod(L, MetaMethod::Len, *vb);
                if (result.isNumber()) {
                    base[a] = result;
                } else {
                    // Metamethod didn't return a number, convert to number
                    base[a] = Value(0.0);
                }
            } catch (const LuaException& e) {
                // No metamethod found, cannot get length of this type
                typeError(L, *vb, "get length of");
                return;
            }
        }
    }

    void VMExecutor::handleConcat(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants) {
        // OP_CONCAT: R(A) := R(B) .. R(C)
        // 我们的编译器实现：二元字符串连接，不是官方的多元连接
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // 获取两个操作数
        Value& leftVal = base[b];
        Value& rightVal = base[c];

        // 转换为字符串并连接
        std::string leftStr = leftVal.toString();
        std::string rightStr = rightVal.toString();
        std::string result = leftStr + rightStr;

        // 检查GC
        luaC_checkGC(L);

        // 创建结果字符串
        Value stringValue(result);
        base[a] = stringValue;

        // 验证结果
        if (base[a].type() != ValueType::String) {
            vmError(L, "CONCAT result has invalid type");
            return;
        }

        (void)constants;
    }

    void VMExecutor::handleJmp(LuaState* L, Instruction instr, u32& pc) {
        i16 sbx = instr.getSBx();
        pc += sbx;
        (void)L;
    }

    void VMExecutor::handleEq(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc) {
        // OP_EQ: if ((RK(B) == RK(C)) ~= A) then pc++
        // Following official Lua 5.1 implementation with metamethod support
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in EQ");
            return;
        }

        bool equal = false;

        // First try direct comparison for same types
        if (vb->type() == vc->type()) {
            if (vb->isNumber()) equal = (vb->asNumber() == vc->asNumber());
            else if (vb->isString()) equal = (vb->asString() == vc->asString());
            else if (vb->isBoolean()) equal = (vb->asBoolean() == vc->asBoolean());
            else if (vb->isNil()) equal = true;
            else if (vb->isTable()) equal = (vb->asTable() == vc->asTable());
            else if (vb->isFunction()) equal = (vb->asFunction() == vc->asFunction());
        } else {
            // Different types - try metamethod
            try {
                Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Eq, *vb, *vc);
                equal = !result.isNil() && !(result.isBoolean() && !result.asBoolean());
            } catch (const LuaException& e) {
                // No metamethod found, different types are not equal
                equal = false;
            }
        }

        // Official Lua 5.1 conditional jump logic:
        // if ((equal) ~= A) then pc++ (skip next instruction which should be a JMP)
        if (equal != (a != 0)) {
            pc++; // Skip next instruction
        }
    }

    void VMExecutor::handleLt(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc) {
        // OP_LT: if ((RK(B) < RK(C)) ~= A) then pc++
        // Following official Lua 5.1 implementation with metamethod support
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in LT");
            return;
        }

        bool less = false;

        // First try direct comparison for compatible types
        if (vb->isNumber() && vc->isNumber()) {
            less = (vb->asNumber() < vc->asNumber());
        } else if (vb->isString() && vc->isString()) {
            less = (vb->asString() < vc->asString());
        } else {
            // Try metamethod for other types or mixed types
            try {
                Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Lt, *vb, *vc);
                less = !result.isNil() && !(result.isBoolean() && !result.asBoolean());
            } catch (const LuaException& e) {
                // No metamethod found, cannot compare these types
                typeError(L, *vb, "compare");
                return;
            }
        }

        // Official Lua 5.1 conditional jump logic:
        // if ((less) ~= A) then pc++ (skip next instruction which should be a JMP)
        if (less != (a != 0)) {
            pc++; // Skip next instruction
        }
    }

    void VMExecutor::handleLe(LuaState* L, Instruction instr, Value* base, const Vec<Value>& constants, u32& pc) {
        // OP_LE: if ((RK(B) <= RK(C)) ~= A) then pc++
        // Following official Lua 5.1 implementation with metamethod support
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        Value* vb = getRK(base, constants, b);
        Value* vc = getRK(base, constants, c);

        if (!vb || !vc) {
            vmError(L, "invalid operands in LE");
            return;
        }

        bool lessEqual = false;

        // First try direct comparison for compatible types
        if (vb->isNumber() && vc->isNumber()) {
            lessEqual = (vb->asNumber() <= vc->asNumber());
        } else if (vb->isString() && vc->isString()) {
            lessEqual = (vb->asString() <= vc->asString());
        } else {
            // Try metamethod for other types or mixed types
            try {
                Value result = MetaMethodManager::callBinaryMetaMethod(L, MetaMethod::Le, *vb, *vc);
                lessEqual = !result.isNil() && !(result.isBoolean() && !result.asBoolean());
            } catch (const LuaException& e) {
                // No metamethod found, cannot compare these types
                typeError(L, *vb, "compare");
                return;
            }
        }

        // Official Lua 5.1 conditional jump logic:
        // if ((lessEqual) ~= A) then pc++ (skip next instruction which should be a JMP)
        if (lessEqual != (a != 0)) {
            pc++; // Skip next instruction
        }
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

        // Official Lua 5.1 logic: if not (R(A) <=> C) then pc++
        // This means: if (isFalse != (C != 0)) then pc++
        if (isFalse != (c != 0)) {
            pc++; // Skip next instruction
        }

        (void)L;
    }

    void VMExecutor::handleClosure(LuaState* L, Instruction instr, Value* base, const Vec<GCRef<Function>>& prototypes, u32& pc) {
        // OP_CLOSURE: R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        u16 bx = instr.getBx();

        if (bx >= prototypes.size()) {
            vmError(L, "prototype index out of range in CLOSURE");
            return;
        }

        // Get the prototype function
        GCRef<Function> prototype = prototypes[bx];
        if (!prototype) {
            vmError(L, "invalid prototype in CLOSURE");
            return;
        }

        // Create a new closure (copy of the prototype)
        // For now, use the prototype directly as we don't have full closure copying
        GCRef<Function> closure = prototype;

        if (!closure) {
            vmError(L, "failed to create closure");
            return;
        }

        // Set up upvalues for the closure
        // For now, simplified implementation without upvalue binding
        // A full implementation would process upvalue binding instructions
        // that follow the CLOSURE instruction

        // Note: In a complete implementation, we would:
        // 1. Read the next N instructions (where N = upvalue count)
        // 2. Each instruction contains upvalue binding information
        // 3. Create or reference upvalues based on the binding info
        // 4. Set the upvalues in the closure

        // For now, we skip this complex part and just use the prototype

        // Store the closure in the target register
        base[a] = Value(closure);
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
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // R(A+1) := R(B) - copy the table/object to A+1
        base[a + 1] = base[b];

        // R(A) := R(B)[RK(C)] - get the method from the table
        Value table = base[b];
        if (!table.isTable()) {
            vmError(L, "attempt to index a non-table value in SELF");
            return;
        }

        Value* key = getRK(base, constants, c);
        if (!key) {
            vmError(L, "invalid key in SELF");
            return;
        }

        // Get the method from the table
        // This is equivalent to luaV_gettable in official Lua 5.1
        Value method = table.asTable()->get(*key);
        base[a] = method;

        // Note: In a full implementation, this should also handle __index metamethod
        // For now, we implement basic table access
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
        // Following official Lua 5.1 implementation for generic for loops
        u8 a = instr.getA();
        u16 c = instr.getC();

        // R(A) = iterator function
        // R(A+1) = state
        // R(A+2) = control variable
        Value iteratorFunc = base[a];
        Value state = base[a + 1];
        Value control = base[a + 2];

        if (!iteratorFunc.isFunction()) {
            vmError(L, "attempt to call a non-function value in for loop");
            return;
        }

        // Call the iterator function: R(A)(R(A+1), R(A+2))
        // Set up function call
        base[a + 3] = iteratorFunc;  // Function to call
        base[a + 4] = state;         // First argument
        base[a + 5] = control;       // Second argument

        // Perform the function call
        try {
            auto func = iteratorFunc.asFunction();
            if (func->getType() == Function::Type::Native) {
                // Call native function
                auto nativeFunc = func->getNative();
                // For now, simplified call - just return nil
                base[a + 3] = Value(); // nil

                // Set remaining return values to nil
                for (int i = 1; i <= c; i++) {
                    if (i == 1) continue; // Skip first result
                    base[a + 2 + i] = Value(); // nil
                }
            } else {
                // For Lua functions, we would need to set up a proper call frame
                // For now, simplified implementation
                base[a + 3] = Value(); // nil (end iteration)
            }
        } catch (const LuaException& e) {
            vmError(L, "error in iterator function");
            return;
        }

        // Check if iteration should continue
        Value firstResult = base[a + 3];
        if (!firstResult.isNil()) {
            // Continue iteration: R(A+2) = R(A+3)
            base[a + 2] = firstResult;
            // pc will be incremented normally, continuing the loop
        } else {
            // End iteration: skip next instruction (which should be a JMP back to loop start)
            pc++;
        }
    }

    void VMExecutor::handleSetList(LuaState* L, Instruction instr, Value* base) {
        // OP_SETLIST: R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
        // Following official Lua 5.1 implementation for table list initialization
        u8 a = instr.getA();
        u16 b = instr.getB();
        u16 c = instr.getC();

        // FPF (Fields Per Flush) is typically 50 in Lua 5.1
        const int FPF = 50;

        // Get the table from R(A)
        Value tableValue = base[a];
        if (!tableValue.isTable()) {
            vmError(L, "attempt to set list elements on non-table value");
            return;
        }

        auto table = tableValue.asTable();

        // Calculate the starting index
        int startIndex = (c - 1) * FPF + 1;

        // Set elements R(A+1) to R(A+B) into the table
        for (int i = 1; i <= b; i++) {
            int tableIndex = startIndex + i - 1;
            Value element = base[a + i];

            // Set table[tableIndex] = element
            Value indexValue(static_cast<double>(tableIndex));
            table->set(indexValue, element);
        }

        // Check GC after table modifications
        luaC_checkGC(L);
    }

    void VMExecutor::handleClose(LuaState* L, Instruction instr, Value* base) {
        // OP_CLOSE: close all variables in the stack up to (>=) R(A)
        // Following official Lua 5.1 implementation
        u8 a = instr.getA();

        // Get the stack level to close up to
        Value* closeLevel = base + a;

        // Get current function from call stack
        CallInfo* ci = L->getCurrentCI();
        if (!ci || !ci->func || !ci->func->isFunction()) {
            // No function context, nothing to close
            return;
        }

        auto func = ci->func->asFunction();
        if (func->getType() != Function::Type::Lua) {
            // Native function, no upvalues to close
            return;
        }

        // Close all open upvalues that point to stack locations >= closeLevel
        for (usize i = 0; i < func->getUpvalueCount(); i++) {
            GCRef<Upvalue> upvalue = func->getUpvalue(i);
            if (upvalue && upvalue->isOpen()) {
                Value* stackLoc = upvalue->getStackLocation();
                if (stackLoc && stackLoc >= closeLevel) {
                    // Close this upvalue with write barrier support
                    upvalue->closeWithBarrier(L);
                }
            }
        }

        // Note: In a full implementation, this should also handle the global
        // open upvalue list maintained by the Lua state, but for now we
        // handle upvalues per function
    }

    void VMExecutor::handleVararg(LuaState* L, Instruction instr, Value* base) {
        // OP_VARARG: R(A), R(A+1), ..., R(A+B-1) = vararg
        // Following official Lua 5.1 implementation for variadic arguments
        u8 a = instr.getA();
        u16 b = instr.getB();

        // Get current call info to access varargs
        CallInfo* ci = L->getCurrentCI();
        if (!ci) {
            vmError(L, "no call context for vararg");
            return;
        }

        // For now, simplified implementation without full vararg support
        // In a complete implementation, we would track the number of arguments
        // passed to the function and calculate varargs from that
        int nvargs = 0; // No varargs available in simplified implementation

        if (b == 0) {
            // B == 0 means copy all available varargs
            // For now, we'll limit to a reasonable number to avoid stack overflow
            int maxVargs = std::min(nvargs, 10); // Limit to 10 varargs for safety

            for (int i = 0; i < maxVargs; i++) {
                // In a full implementation, we would get varargs from the call frame
                // For now, set to nil as we don't have full vararg support yet
                base[a + i] = Value(); // nil
            }
        } else {
            // B > 0 means copy exactly B-1 varargs
            int vargsToCopy = b - 1;

            for (int i = 0; i < vargsToCopy; i++) {
                if (i < nvargs) {
                    // In a full implementation, we would get the actual vararg value
                    // For now, set to nil as we don't have full vararg support yet
                    base[a + i] = Value(); // nil
                } else {
                    // No more varargs available, set to nil
                    base[a + i] = Value(); // nil
                }
            }
        }

        // Note: This is a simplified implementation. A full implementation would
        // need to properly track varargs in the call frame and stack management.
    }

    bool VMExecutor::handleTailCall(LuaState* L, Instruction instr, Value* base) {
        u8 a = instr.getA();
        u16 b = instr.getB();
        (void)L; (void)a; (void)b; (void)base;
        return true;
    }

} // namespace Lua
