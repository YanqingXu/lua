# Closure — 闭包

## 1. Closure 结构

```cpp
class Closure {
    Proto* proto_;               // Lua Closure: 指向 Proto
    lua_CFunction cFunc_;        // C Closure: 函数指针
    Vec<Upvalue*> upvalues_;     // 捕获的上值列表
    Table* env_;                 // 函数环境
    
    enum ClosureType {
        LuaClosure,
        CClosure
    };
};
```

## 2. Lua Closure 创建

```cpp
// CLOSURE 指令:
Closure* cl = new Closure();
cl->proto_ = subProto;
cl->env_ = currentEnv;

// 绑定 Upvalue
for (auto& desc : proto_->upvalueDescs_) {
    Upvalue* uv = findOrCreateUpvalue(desc);
    cl->upvalues_.push_back(uv);
}
```

## 3. C Closure 创建

```cpp
// C API: lua_pushcclosure(L, func, nupvalues)
Closure* cl = new Closure();
cl->cFunc_ = func;

// Upvalue 从栈上获取
for (i32 i = 0; i < nupvalues; i++) {
    cl->upvalues_.push_back(stack.popUpvalue());
}
```

## 4. 函数环境 (env)

```lua
-- 每个函数有独立的环境表 (Lua 5.1)
function f()
    print(x)  -- 在 f 的环境中查找 x
end

-- 可以改变函数的环境
setfenv(f, { x = 100, print = print })
f()  -- 打印 100 (而不是全局 x)
```

## 5. Function 包装

```cpp
class Function : public GCObject {
    Closure* closure_;     // 指向 Closure (Lua 或 C)
    Table* env_;           // 函数环境 (getfenv/setfenv)
    
    bool isLua() const;
    bool isC() const;
    Proto* getProto() const;
    lua_CFunction getCFunction() const;
    const Vec<Upvalue*>& getUpvalues() const;
};
```
