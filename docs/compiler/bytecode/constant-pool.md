# Constant Pool — 常量池

## 1. 这个模块解决什么问题？

编译时如何管理常量表（数字、字符串、nil、boolean）。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/codegen.cpp` | 常量管理 |
| `src/core/function.hpp` | ConstantKey 去重类型 |

## 3. 常量池结构

```cpp
// Proto 中的常量表
Vec<Value> constants;

// 支持的常量类型:
//   - nil (std::monostate)
//   - boolean (true/false)
//   - number (double)
//   - string (GCString*)
```

## 4. 常量去重

```cpp
// 使用 ConstantKey 进行去重
HashMap<ConstantKey, i32> constantMap;

i32 addConstant(const Value& value) {
    ConstantKey key = ConstantKey::fromValue(value);
    
    // 检查是否已存在
    auto it = constantMap.find(key);
    if (it != constantMap.end()) {
        return it->second;  // 已存在，返回索引
    }
    
    // 新常量
    i32 index = constants.size();
    constants.push_back(value);
    constantMap[key] = index;
    return index;
}
```

## 5. RK 编码与常量索引

```
常量索引在指令中的编码:

如果常量索引 < 256:
  → 可以使用 RK 编码: RKASK(index) = index | BITRK
  → 出现在 B 或 C 位置

如果常量索引 >= 256:
  → 超过 RK 范围，需要特殊处理
  → 例如: 先用 LOADK 加载到寄存器，再用寄存器操作

最大 RK 常量索引: 255
最大 LOADK Bx 常量索引: 262143
```

## 6. 常见常量场景

```lua
-- 每个数字字面量 → 常量池条目
local x = 42         -- K(0): 42

-- 每个字符串字面量 → 常量池条目
print("hello")       -- K(1): "hello"

-- nil/true/false → 常量池条目
local x = nil        -- K(2): nil
local y = true       -- K(3): true
```

## 7. 常量池复用

```lua
-- 相同的常量只存一份
local a = 42
local b = 42    -- 复用 K(0): 42
```
