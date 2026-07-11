# Metatable Overview — 元表系统

## 1. 元表 API

```lua
-- 设置/获取元表
setmetatable(t, mt)
getmetatable(t)

-- 元表保护 (__metatable 字段)
-- 设置后 getmetatable 返回保护值而非真正的元表
```

## 2. 元方法分类

### 算术元方法
| 元方法 | 触发操作 | 示例 |
|--------|---------|------|
| `__add` | `+` | `a + b` |
| `__sub` | `-` | `a - b` |
| `__mul` | `*` | `a * b` |
| `__div` | `/` | `a / b` |
| `__mod` | `%` | `a % b` |
| `__pow` | `^` | `a ^ b` |
| `__unm` | `-x` (一元) | `-a` |

### 关系元方法
| 元方法 | 触发操作 | 示例 |
|--------|---------|------|
| `__eq` | `==` | `a == b` |
| `__lt` | `<` | `a < b` |
| `__le` | `<=` | `a <= b` |

### 访问元方法
| 元方法 | 触发操作 | 示例 |
|--------|---------|------|
| `__index` | 读不存在的 key | `t.x` |
| `__newindex` | 写不存在的 key | `t.x = 1` |
| `__call` | 当函数调用 | `t()` |

### 其他元方法
| 元方法 | 触发操作 |
|--------|---------|
| `__concat` | `..` |
| `__len` | `#` |
| `__tostring` | `tostring()` |
| `__gc` | GC 回收时 |
| `__mode` | 弱表标记 |
| `__metatable` | 保护元表 |

## 3. 基础类型元表

```lua
-- 可以为基础类型设置全局元表
-- string 已安装 __index = string
debug.setmetatable(0, { __index = math })  -- 数字也可以有方法
```

## 4. 元方法调用链

```
C 侧:
  callTM(t, k, args...)
    → 查找 t 的 metatable[k]
    → 调用该函数
    
  callTMWithResult(t, k, args...)
    → 同上，但保留返回值 (用于 __add 等需返回值的场景)

Lua 侧:
  getMetamethodByObject(obj, methodName)
    → 统一入口，支持 table/userdata/基础类型
```
