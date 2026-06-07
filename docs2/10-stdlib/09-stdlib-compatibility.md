# Standard Library Compatibility Matrix

## 1. Base Library

| 函数 | Lua 5.1 | 当前项目 | 状态 |
|------|---------|---------|------|
| print | ✅ | ✅ | done |
| type | ✅ | ✅ | done |
| tostring | ✅ | ✅ | done |
| tonumber | ✅ | ✅ | done |
| error | ✅ | ✅ | done |
| assert | ✅ | ✅ | done |
| next | ✅ | ✅ | done |
| pairs | ✅ | ✅ | done |
| ipairs | ✅ | ✅ | done |
| rawget/rawset | ✅ | ✅ | done |
| rawequal | ✅ | ✅ | done |
| select | ✅ | ✅ | done |
| pcall | ✅ | ✅ | done |
| xpcall | ✅ | ✅ | done |
| loadstring | ✅ | ✅ | done |
| loadfile | ✅ | ✅ | done |
| dofile | ✅ | ✅ | done |
| load | ✅ | ✅ | done |
| getfenv/setfenv | ✅ | ✅ | done |
| getmetatable | ✅ | ✅ | done |
| setmetatable | ✅ | ✅ | done |
| collectgarbage | ✅ | ✅ | done |
| gcinfo | ✅ | ✅ | done |
| unpack | ✅ | ✅ | done |
| newproxy | ✅ | ✅ | done |

## 2. Math Library

| 函数 | 状态 |
|------|------|
| abs, floor, ceil, sqrt | ✅ done |
| sin, cos, tan, asin, acos, atan, atan2 | ✅ done |
| sinh, cosh, tanh | ✅ done |
| exp, log, log10 | ✅ done |
| pow, mod (fmod), frexp, ldexp | ✅ done |
| random, randomseed | ✅ done |
| pi, huge | ✅ done |

## 3. 剩余工作

| 项目 | 说明 |
|------|------|
| 官方 testC helper | 未接入，api.lua/code.lua 走 skip 分支 |
| 官方 binary chunk | 使用项目本地格式 |
| debug 库精细边界 | 94% 完成，部分 hook 边界待对齐 |
