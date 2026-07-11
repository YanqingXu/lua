# Value Debug Print — Value 调试输出

## 1. toString()

```cpp
Str Value::toString() const {
    switch (getType()) {
    case ValueType::Nil:     return "nil";
    case ValueType::Boolean: return asBoolean() ? "true" : "false";
    case ValueType::Number:  return std::to_string(asNumber());
    case ValueType::String:  return Str("\"") + asString()->c_str() + "\"";
    case ValueType::Table:   return "table: " + ptrStr(asTable());
    case ValueType::Function:return "function: " + ptrStr(asFunction());
    case ValueType::Userdata:return "userdata: " + ptrStr(asUserdata());
    case ValueType::Thread:  return "thread: " + ptrStr(asThread());
    case ValueType::LightUserdata: return "lightuserdata: " + ptrStr(asLightUserdata());
    }
}
```

## 2. Value Serializer (JSON 格式)

```cpp
// src/debug/value_serializer.cpp
// 用于 trace 输出中的值序列化

Str serializeValue(const Value& v) {
    // null → "null"
    // true → "true"
    // 42.0 → "42.0"
    // "hello" → "\"hello\""
    // table → "{...}" (递归序列化)
    // function → "\"function: 0x...\""
}
```

## 3. 调试断点中的 Value 查看

```cpp
// Visual Studio Watch 窗口:
// 直接查看 value.value_ 可以看到 variant 的当前索引和值

// 在代码中打印:
std::cerr << "R(0) = " << R(0).toString() << std::endl;
std::cerr << "R(1).type = " << static_cast<int>(R(1).getType()) << std::endl;
```

## 4. 常见调试场景

```
1. 栈状态打印:
   for (i32 i = ci.base; i < ci.top; i++)
     printf("R(%d) = %s\n", i-ci.base, stack[i].toString().c_str());

2. 常量表打印:
   for (usize i = 0; i < proto->getConstants().size(); i++)
     printf("K(%zu) = %s\n", i, proto->getConstants()[i].toString().c_str());

3. Upvalue 值打印:
   printf("uv[%d] = %s\n", i, uv->getValue().toString().c_str());
```
