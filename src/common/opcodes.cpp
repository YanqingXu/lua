#include "opcodes.hpp"

namespace Lua {
    // === 官方Lua 5.1操作码名称数组 ===
    // 基于lua-5.1.5/src/lopcodes.c的luaP_opnames数组
    const char* const luaP_opnames[NUM_OPCODES + 1] = {
        "MOVE",
        "LOADK",
        "LOADBOOL",
        "LOADNIL",
        "GETUPVAL",
        "GETGLOBAL",
        "GETTABLE",
        "SETGLOBAL",
        "SETUPVAL",
        "SETTABLE",
        "NEWTABLE",
        "SELF",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "MOD",
        "POW",
        "UNM",
        "NOT",
        "LEN",
        "CONCAT",
        "JMP",
        "EQ",
        "LT",
        "LE",
        "TEST",
        "TESTSET",
        "CALL",
        "TAILCALL",
        "RETURN",
        "FORLOOP",
        "FORPREP",
        "TFORLOOP",
        "SETLIST",
        "CLOSE",
        "CLOSURE",
        "VARARG",
        nullptr
    };

    // === 官方Lua 5.1指令模式数组 ===
    // 基于lua-5.1.5/src/lopcodes.c的luaP_opmodes数组
    // 每个字节编码一个指令的模式信息
    const u8 luaP_opmodes[NUM_OPCODES] = {
        /*       T  A    B       C     mode		   opcode	*/
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iABC),   /* OP_MOVE */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgN, OpMode::iABx),   /* OP_LOADK */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgU, OpMode::iABC),   /* OP_LOADBOOL */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iABC),   /* OP_LOADNIL */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgN, OpMode::iABC),   /* OP_GETUPVAL */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgN, OpMode::iABx),   /* OP_GETGLOBAL */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgK, OpMode::iABC),   /* OP_GETTABLE */
        opmode(0, 0, OpArgMask::OpArgK, OpArgMask::OpArgN, OpMode::iABx),   /* OP_SETGLOBAL */
        opmode(0, 0, OpArgMask::OpArgU, OpArgMask::OpArgN, OpMode::iABC),   /* OP_SETUPVAL */
        opmode(0, 0, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_SETTABLE */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgU, OpMode::iABC),   /* OP_NEWTABLE */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgK, OpMode::iABC),   /* OP_SELF */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_ADD */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_SUB */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_MUL */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_DIV */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_MOD */
        opmode(0, 1, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_POW */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iABC),   /* OP_UNM */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iABC),   /* OP_NOT */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iABC),   /* OP_LEN */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgR, OpMode::iABC),   /* OP_CONCAT */
        opmode(0, 0, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iAsBx),  /* OP_JMP */
        opmode(1, 0, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_EQ */
        opmode(1, 0, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_LT */
        opmode(1, 0, OpArgMask::OpArgK, OpArgMask::OpArgK, OpMode::iABC),   /* OP_LE */
        opmode(1, 1, OpArgMask::OpArgR, OpArgMask::OpArgU, OpMode::iABC),   /* OP_TEST */
        opmode(1, 1, OpArgMask::OpArgR, OpArgMask::OpArgU, OpMode::iABC),   /* OP_TESTSET */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgU, OpMode::iABC),   /* OP_CALL */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgU, OpMode::iABC),   /* OP_TAILCALL */
        opmode(0, 0, OpArgMask::OpArgU, OpArgMask::OpArgN, OpMode::iABC),   /* OP_RETURN */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iAsBx),  /* OP_FORLOOP */
        opmode(0, 1, OpArgMask::OpArgR, OpArgMask::OpArgN, OpMode::iAsBx),  /* OP_FORPREP */
        opmode(1, 0, OpArgMask::OpArgN, OpArgMask::OpArgU, OpMode::iABC),   /* OP_TFORLOOP */
        opmode(0, 0, OpArgMask::OpArgU, OpArgMask::OpArgU, OpMode::iABC),   /* OP_SETLIST */
        opmode(0, 0, OpArgMask::OpArgN, OpArgMask::OpArgN, OpMode::iABC),   /* OP_CLOSE */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgN, OpMode::iABx),   /* OP_CLOSURE */
        opmode(0, 1, OpArgMask::OpArgU, OpArgMask::OpArgN, OpMode::iABC)    /* OP_VARARG */
    };

} // namespace Lua
