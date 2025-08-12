#include "vm.hpp"
#include "lua_state.hpp"
#include "function.hpp"
#include "table.hpp"
#include "upvalue.hpp"
#include "../common/defines.hpp"
#include "../common/opcodes.hpp"
#include "../gc/core/garbage_collector.hpp"
#include <iostream>
#include <stdexcept>

namespace Lua {

    // Static VM implementation - no instance state

    void VM::execute(LuaState* L) {
        if (!L) {
            throwError(L, "LuaState is null");
            return;
        }

        // This is a simplified implementation
        // In a full implementation, this would execute the current function
        // For now, we'll just ensure the basic structure is in place

        // TODO: Implement full VM execution loop
        // 1. Get current function from CallInfo
        // 2. Execute instructions in a loop
        // 3. Handle function calls, returns, etc.
    }

    void VM::call(LuaState* L, i32 nargs, i32 nresults) {
        if (!L) {
            throwError(L, "LuaState is null");
            return;
        }

        // Use LuaState's call method
        L->call(nargs, nresults);
    }

    i32 VM::pcall(LuaState* L, i32 nargs, i32 nresults, i32 errfunc) {
        if (!L) {
            return LUA_ERRRUN;
        }

        try {
            call(L, nargs, nresults);
            return LUA_OK;
        } catch (const LuaException& e) {
            // Handle Lua exception
            L->push(Value(e.what()));
            return LUA_ERRRUN;
        } catch (const std::exception& e) {
            // Handle other exceptions
            L->push(Value(e.what()));
            return LUA_ERRMEM;
        }
    }

    void VM::executeInstruction(LuaState* L, Instruction i) {
        if (!L) {
            throwError(L, "LuaState is null");
            return;
        }

        // Decode opcode
        OpCode op = i.getOpCode();

        // Execute instruction based on opcode
        switch (op) {
            case OpCode::MOVE:
                op_move(L, i);
                break;
            case OpCode::LOADK:
                op_loadk(L, i);
                break;
            case OpCode::LOADBOOL:
                op_loadbool(L, i);
                break;
            case OpCode::LOADNIL:
                op_loadnil(L, i);
                break;
            case OpCode::GETGLOBAL:
                op_getglobal(L, i);
                break;
            case OpCode::SETGLOBAL:
                op_setglobal(L, i);
                break;
            case OpCode::GETTABLE:
                op_gettable(L, i);
                break;
            case OpCode::SETTABLE:
                op_settable(L, i);
                break;
            case OpCode::NEWTABLE:
                op_newtable(L, i);
                break;
            case OpCode::ADD:
                op_add(L, i);
                break;
            case OpCode::SUB:
                op_sub(L, i);
                break;
            case OpCode::MUL:
                op_mul(L, i);
                break;
            case OpCode::DIV:
                op_div(L, i);
                break;
            case OpCode::MOD:
                op_mod(L, i);
                break;
            case OpCode::POW:
                op_pow(L, i);
                break;
            case OpCode::UNM:
                op_unm(L, i);
                break;
            case OpCode::NOT:
                op_not(L, i);
                break;
            case OpCode::LEN:
                op_len(L, i);
                break;
            case OpCode::CONCAT:
                op_concat(L, i);
                break;
            case OpCode::EQ:
                op_eq(L, i);
                break;
            case OpCode::LT:
                op_lt(L, i);
                break;
            case OpCode::LE:
                op_le(L, i);
                break;
            case OpCode::JMP:
                op_jmp(L, i);
                break;
            case OpCode::TEST:
                op_test(L, i);
                break;
            case OpCode::CALL:
                op_call(L, i);
                break;
            case OpCode::RETURN:
                op_return(L, i);
                break;
            case OpCode::VARARG:
                op_vararg(L, i);
                break;
            case OpCode::CLOSURE:
                op_closure(L, i);
                break;
            case OpCode::GETUPVAL:
                op_getupval(L, i);
                break;
            case OpCode::SETUPVAL:
                op_setupval(L, i);
                break;
            default:
                throwError(L, "Unknown opcode");
                break;
        }
    }

    // RegisterFile operations implementation
    RegisterFile VM::createRegisterFile(LuaState* L) {
        return RegisterFile(L);
    }

    Value& VM::getRegister(RegisterFile& rf, i32 reg) {
        return rf.get(reg);
    }

    void VM::setRegister(RegisterFile& rf, i32 reg, const Value& val) {
        rf.set(reg, val);
    }

    void VM::adjustResults(LuaState* L, i32 nresults) {
        if (nresults >= 0) {
            L->setTop(nresults);
        }
    }

    Value VM::getConstant(LuaState* L, u32 idx) {
        // TODO: Implement constant access from current function
        return Value();  // Return nil for now
    }

    Function* VM::getCurrentFunction(LuaState* L) {
        // TODO: Get current function from CallInfo
        return nullptr;
    }

    void VM::setCurrentFunction(LuaState* L, Function* func) {
        // TODO: Set current function in CallInfo
    }

    void VM::throwError(LuaState* L, const Str& message) {
        throw LuaException(message.c_str());
    }

    // Instruction implementations using RegisterFile
    void VM::op_move(LuaState* L, Instruction i) {
        RegisterFile rf = createRegisterFile(L);

        u8 a = i.getA();
        u8 b = i.getB();

        // MOVE instruction: R[A] = R[B]
        rf.move(a, b);
    }

    void VM::op_loadk(LuaState* L, Instruction i) {
        // TODO: Implement LOADK instruction
    }

    void VM::op_loadbool(LuaState* L, Instruction i) {
        // TODO: Implement LOADBOOL instruction
    }

    void VM::op_loadnil(LuaState* L, Instruction i) {
        RegisterFile rf = createRegisterFile(L);

        u8 a = i.getA();
        u8 b = i.getB();

        // LOADNIL instruction: R[A] to R[A+B] = nil
        rf.fillNil(a, b + 1);
    }

    void VM::op_getglobal(LuaState* L, Instruction i) {
        // TODO: Implement GETGLOBAL instruction
    }

    void VM::op_setglobal(LuaState* L, Instruction i) {
        // TODO: Implement SETGLOBAL instruction
    }

    void VM::op_gettable(LuaState* L, Instruction i) {
        // TODO: Implement GETTABLE instruction
    }

    void VM::op_settable(LuaState* L, Instruction i) {
        // TODO: Implement SETTABLE instruction
    }

    void VM::op_newtable(LuaState* L, Instruction i) {
        // TODO: Implement NEWTABLE instruction
    }

    void VM::op_add(LuaState* L, Instruction i) {
        // TODO: Implement ADD instruction
    }

    void VM::op_sub(LuaState* L, Instruction i) {
        // TODO: Implement SUB instruction
    }

    void VM::op_mul(LuaState* L, Instruction i) {
        // TODO: Implement MUL instruction
    }

    void VM::op_div(LuaState* L, Instruction i) {
        // TODO: Implement DIV instruction
    }

    void VM::op_mod(LuaState* L, Instruction i) {
        // TODO: Implement MOD instruction
    }

    void VM::op_pow(LuaState* L, Instruction i) {
        // TODO: Implement POW instruction
    }

    void VM::op_unm(LuaState* L, Instruction i) {
        // TODO: Implement UNM instruction
    }

    void VM::op_not(LuaState* L, Instruction i) {
        // TODO: Implement NOT instruction
    }

    void VM::op_len(LuaState* L, Instruction i) {
        // TODO: Implement LEN instruction
    }

    void VM::op_concat(LuaState* L, Instruction i) {
        // TODO: Implement CONCAT instruction
    }

    void VM::op_eq(LuaState* L, Instruction i) {
        // TODO: Implement EQ instruction
    }

    void VM::op_lt(LuaState* L, Instruction i) {
        // TODO: Implement LT instruction
    }

    void VM::op_le(LuaState* L, Instruction i) {
        // TODO: Implement LE instruction
    }

    void VM::op_jmp(LuaState* L, Instruction i) {
        // TODO: Implement JMP instruction
    }

    void VM::op_test(LuaState* L, Instruction i) {
        // TODO: Implement TEST instruction
    }

    void VM::op_call(LuaState* L, Instruction i) {
        // TODO: Implement CALL instruction
    }

    void VM::op_return(LuaState* L, Instruction i) {
        // TODO: Implement RETURN instruction
    }

    void VM::op_vararg(LuaState* L, Instruction i) {
        // TODO: Implement VARARG instruction
    }

    void VM::op_closure(LuaState* L, Instruction i) {
        // TODO: Implement CLOSURE instruction
    }

    void VM::op_getupval(LuaState* L, Instruction i) {
        // TODO: Implement GETUPVAL instruction
    }

    void VM::op_setupval(LuaState* L, Instruction i) {
        // TODO: Implement SETUPVAL instruction
    }

} // namespace Lua
