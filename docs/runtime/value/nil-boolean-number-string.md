# Basic Types — nil / boolean / number / string

## 1. nil

```cpp
// nil = std::monostate (默认构造)
Value nilVal;  // 默认为 nil

// 检查
value.isNil();

// 只有 nil 和 false 是假值
value.isFalse();  // nil → true
```

## 2. boolean

```cpp
Value trueVal(true);
Value falseVal(false);

// 检查
value.isBoolean();
bool b = value.asBoolean();

// Lua 真值: 只有 nil/false 为假，0 和 "" 都是真
Value(0.0).isTrue();   // true
```

## 3. number

```cpp
// Lua number = double (IEEE 754)
using LuaNumber = double;

Value numVal(42.0);
Value piVal(3.14159);

// 整数也存为 double
Value intVal(42);  // 隐式转为 42.0

// 访问
f64 n = value.asNumber();
i64 i = value.asInteger();  // 截断转换
```

number 支持所有 double 运算，NaN 和 ±Inf 行为与 Lua 5.1 一致。

## 4. string

```cpp
// String 通过 StringPool 驻留
GCString* str = StringPool::getInstance().intern("hello");
Value strVal(str);

// 访问
const char* s = value.asString()->c_str();
usize len = value.asString()->getLength();

// 指针比较 = 内容比较 (因为驻留)
str1->c_str() == str2->c_str()  // 同一字符串 = 同一指针
```

## 5. lightuserdata

```cpp
// 轻量级用户数据 = void* 指针，不受 GC 管理
int data = 42;
Value ludVal(static_cast<void*>(&data));

void* ptr = value.asLightUserdata();
```

## 6. 类型转换

```lua
-- Lua 隐式转换
"42" + 1      -- 43 (string→number)
10 .. "20"    -- "1020" (number→string)
tostring(42)  -- "42"
tonumber("42") -- 42
```
