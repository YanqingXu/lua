# Package Library — 包/模块库

## require()

```lua
-- 按顺序搜索 loaders:
-- 1. preload 表
-- 2. Lua 文件 (通过 package.path)
-- 3. C 模块 (通过 package.cpath)
local mod = require("module_name")
```

## module()

```lua
-- 创建模块
module("mymodule", package.seeall)
-- 等价于:
--   local mod = {}
--   _G["mymodule"] = mod
--   setmetatable(mod, {__index = _G})
--   setfenv(1, mod)
```

## package 表

| 字段 | 说明 |
|------|------|
| `package.loaded` | 已加载模块缓存 |
| `package.preload` | preload 函数 |
| `package.loaders` | loader 函数数组 |
| `package.path` | Lua 文件搜索路径 |
| `package.cpath` | C 模块搜索路径 |
| `package.loadlib(lib, func)` | 动态加载 C 函数 |
| `package.seeall` | 给 module() 使用的选项 |
