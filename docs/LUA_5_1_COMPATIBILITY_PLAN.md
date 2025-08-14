# Lua 5.1 完整兼容性开发计划

## 📊 项目概述

基于对官方Lua 5.1源码的系统性对比分析，本文档制定了详细的开发计划，旨在将我们的Lua解释器实现提升到完整的Lua 5.1兼容性水平。

**当前兼容性状态：65%**
- ✅ 基础栈操作：70%完整度
- ✅ 类型系统：80%完整度  
- ⚠️ 调用机制：60%完整度
- ❌ 全局状态：50%完整度
- ❌ GC集成：40%完整度
- ❌ 错误处理：30%完整度

**目标：实现95%+ Lua 5.1兼容性**

## 🎯 Phase 1: 高优先级核心API实现 (预计2-3周)

### 1.1 栈操作API补全

**目标文件：** `src/vm/lua_state.hpp/cpp`

#### 缺失的关键栈操作函数：

```cpp
// 需要在LuaState类中添加的方法
class LuaState {
public:
    // 栈操作 (对应lua_* API)
    void pushValue(i32 idx);        // lua_pushvalue - 复制栈中值到栈顶
    void remove(i32 idx);           // lua_remove - 移除指定索引的值
    void insert(i32 idx);           // lua_insert - 在指定位置插入栈顶值
    void replace(i32 idx);          // lua_replace - 用栈顶值替换指定位置
    
    // Push函数系列
    void pushNil();                 // lua_pushnil
    void pushNumber(f64 n);         // lua_pushnumber  
    void pushInteger(i64 n);        // lua_pushinteger
    void pushString(const char* s); // lua_pushstring
    void pushLString(const char* s, usize len); // lua_pushlstring
    void pushBoolean(bool b);       // lua_pushboolean
    void pushCFunction(lua_CFunction fn); // lua_pushcfunction
    void pushLightUserdata(void* p); // lua_pushlightuserdata
    
    // To函数系列  
    f64 toNumber(i32 idx);          // lua_tonumber
    i64 toInteger(i32 idx);         // lua_tointeger
    const char* toString(i32 idx);  // lua_tostring
    const char* toLString(i32 idx, usize* len); // lua_tolstring
    bool toBoolean(i32 idx);        // lua_toboolean
    lua_CFunction toCFunction(i32 idx); // lua_tocfunction
    void* toUserdata(i32 idx);      // lua_touserdata
    
    // 类型检查增强
    bool isCFunction(i32 idx);      // lua_iscfunction
    bool isUserdata(i32 idx);       // lua_isuserdata
    i32 type(i32 idx);              // lua_type
    const char* typeName(i32 tp);   // lua_typename
};
```

**实现优先级：**
1. 🔴 **立即实现**：pushValue, remove, insert, replace
2. 🔴 **立即实现**：pushNil, pushNumber, pushString, pushBoolean
3. 🔴 **立即实现**：toNumber, toString, toBoolean
4. 🟡 **第二批**：其余Push/To函数

### 1.2 LuaState类字段补全

**目标文件：** `src/vm/lua_state.hpp/cpp`

#### 需要添加的关键字段：

```cpp
class LuaState : public GCObject {
private:
    // 现有字段...
    
    // 新增缺失字段
    Value l_gt_;                    // 全局表 (对应官方TValue l_gt)
    Value env_;                     // 环境表临时存储 (对应官方TValue env)
    GCObject* gclist_;              // GC链表节点 (对应官方GCObject *gclist)
    u8 allowhook_;                  // 允许钩子标志 (对应官方lu_byte allowhook)
    
    // 修复类型不匹配
    lua_Hook hook_;                 // 修复：从void*改为lua_Hook类型
    
    // 错误处理增强
    struct lua_longjmp* errorJmp_;  // 错误恢复点 (对应官方struct lua_longjmp *errorJmp)
};
```

**实现计划：**
1. **Week 1**: 添加l_gt_和env_字段，实现全局表访问
2. **Week 2**: 集成gclist_字段到GC系统
3. **Week 3**: 实现钩子系统基础结构

### 1.3 表操作API实现

**目标文件：** `src/vm/lua_state.hpp/cpp`

```cpp
// 表操作核心API
void getTable(i32 idx);             // lua_gettable
void setTable(i32 idx);             // lua_settable  
void getField(i32 idx, const char* k); // lua_getfield
void setField(i32 idx, const char* k); // lua_setfield
void rawGet(i32 idx);               // lua_rawget
void rawSet(i32 idx);               // lua_rawset
void rawGetI(i32 idx, i32 n);       // lua_rawgeti
void rawSetI(i32 idx, i32 n);       // lua_rawseti
void createTable(i32 narr, i32 nrec); // lua_createtable
```

## 🎯 Phase 2: 中优先级系统完善 (预计3-4周)

### 2.1 GlobalState类GC字段补全

**目标文件：** `src/vm/global_state.hpp/cpp`

#### 需要添加的GC相关字段：

```cpp
class GlobalState {
private:
    // 现有字段...
    
    // GC状态管理 (对应官方global_State)
    u8 currentwhite_;               // 当前白色标记
    u8 gcstate_;                    // GC状态机状态
    i32 sweepstrgc_;                // 字符串GC扫描位置
    
    // GC对象链表管理
    GCObject* rootgc_;              // 所有可回收对象的根链表
    GCObject** sweepgc_;            // GC扫描位置指针
    GCObject* gray_;                // 灰色对象链表
    GCObject* grayagain_;           // 需要重新遍历的灰色对象
    GCObject* weak_;                // 弱表链表
    GCObject* tmudata_;             // 待GC的userdata链表
    
    // 内存管理增强
    void* ud_;                      // 分配器用户数据
    usize estimate_;                // 内存使用估计
    usize gcdept_;                  // GC债务
    i32 gcpause_;                   // GC暂停参数
    i32 gcstepmul_;                 // GC步长倍数
    
    // 字符串连接缓冲区
    struct Mbuffer {
        char* buffer;
        usize size;
        usize capacity;
    } buff_;
    
    // 恐慌函数
    lua_CFunction panic_;           // 未保护错误时调用的函数
    
    // upvalue管理
    UpVal uvhead_;                  // upvalue双向链表头
    
    // 元方法名称
    GCString* tmname_[TM_N];        // 元方法名称数组
};
```

### 2.2 函数调用机制完善

**目标文件：** `src/vm/lua_state.hpp/cpp`

```cpp
// 函数调用API
void call(i32 nargs, i32 nresults);    // lua_call
i32 pcall(i32 nargs, i32 nresults, i32 errfunc); // lua_pcall
i32 cpcall(lua_CFunction func, void* ud); // lua_cpcall

// 协程API
i32 yield(i32 nresults);               // lua_yield
i32 resume(i32 narg);                  // lua_resume
i32 status();                          // lua_status

// 元表操作
i32 getMetatable(i32 objindex);        // lua_getmetatable
i32 setMetatable(i32 objindex);        // lua_setmetatable
void getFenv(i32 idx);                 // lua_getfenv
i32 setFenv(i32 idx);                  // lua_setfenv
```

## 🎯 Phase 3: 低优先级高级功能 (预计2-3周)

### 3.1 错误处理机制完善

**目标文件：** `src/vm/error_handling.hpp/cpp` (新建)

```cpp
// 错误处理系统
class ErrorHandler {
public:
    // setjmp/longjmp替代方案 (使用C++异常)
    struct LuaLongJmp {
        std::exception_ptr exception;
        i32 status;
    };
    
    // 错误恢复机制
    void setErrorJmp(LuaLongJmp* jmp);
    void clearErrorJmp();
    void throwError(i32 status, const char* msg);
};
```

### 3.2 调试钩子系统

**目标文件：** `src/vm/debug_hooks.hpp/cpp` (新建)

```cpp
// 调试钩子系统
class DebugHooks {
public:
    // 钩子类型
    enum HookMask {
        LUA_MASKCALL = 1,
        LUA_MASKRET = 2, 
        LUA_MASKLINE = 4,
        LUA_MASKCOUNT = 8
    };
    
    // 钩子管理
    void setHook(lua_Hook hook, i32 mask, i32 count);
    lua_Hook getHook();
    i32 getHookMask();
    i32 getHookCount();
};
```

## 📅 实施时间线

### 第1-3周：Phase 1 高优先级实现
- **Week 1**: 栈操作API (pushValue, remove, insert, replace)
- **Week 2**: Push/To函数系列实现
- **Week 3**: LuaState字段补全和表操作API

### 第4-7周：Phase 2 中优先级实现  
- **Week 4-5**: GlobalState GC字段补全
- **Week 6**: 函数调用机制完善
- **Week 7**: 协程和元表操作

### 第8-10周：Phase 3 低优先级实现
- **Week 8**: 错误处理机制
- **Week 9**: 调试钩子系统
- **Week 10**: 性能优化和测试

## 🧪 兼容性测试策略

### 测试框架设计

**目标文件：** `src/tests/lua51_compatibility/` (新建目录)

```
src/tests/lua51_compatibility/
├── api_tests/              # API兼容性测试
│   ├── stack_ops_test.cpp  # 栈操作测试
│   ├── type_conv_test.cpp  # 类型转换测试
│   ├── table_ops_test.cpp  # 表操作测试
│   └── call_test.cpp       # 函数调用测试
├── official_tests/         # 官方测试用例移植
│   ├── basic.lua          # 基础功能测试
│   ├── coroutine.lua      # 协程测试
│   └── gc.lua             # 垃圾回收测试
└── regression_tests/       # 回归测试
    └── compatibility_suite.cpp
```

### 测试验证标准

1. **API行为一致性**：每个实现的API必须与官方Lua 5.1行为完全一致
2. **内存管理正确性**：GC行为必须符合Lua 5.1规范
3. **错误处理兼容性**：错误消息和处理流程必须匹配
4. **性能基准**：关键操作性能不得低于官方实现的80%

## 📈 成功指标

### 量化目标
- **API覆盖率**：95%+ Lua 5.1 C API实现
- **测试通过率**：100%官方测试用例通过
- **兼容性评分**：从当前65%提升到95%+
- **性能指标**：核心操作性能达到官方实现的80%+

### 里程碑检查点
- **Phase 1完成**：栈操作和基础API 100%实现
- **Phase 2完成**：GC和调用机制完全兼容
- **Phase 3完成**：高级功能和调试支持完整

## 🔄 持续集成策略

### 自动化测试流程
1. **每日构建**：确保所有新实现不破坏现有功能
2. **兼容性回归**：运行完整的Lua 5.1兼容性测试套件
3. **性能基准**：监控关键操作的性能变化
4. **内存泄漏检测**：确保GC实现的正确性

---

**文档版本**: v1.0  
**创建日期**: 2025-08-14  
**预计完成**: 2025-10-14 (10周开发周期)  
**负责团队**: VM核心开发组
