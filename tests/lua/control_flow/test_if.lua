-- ============================================================================
-- test_if.lua
-- Lua 解释器 if 语句全面测试脚本
--
-- 测试内容：
--   1. 基础 if 语句（if-then）
--   2. if-else 语句
--   3. if-elseif-else 语句（多个 elseif 分支）
--   4. 嵌套 if 语句（多层嵌套）
--   5. 带有逻辑运算符的 if 语句（and、or、not）
--   6. 带有比较运算符的 if 语句（==、~=、<、>、<=、>=）
--   7. 边界情况测试（nil、false、0、空字符串等）
-- ============================================================================

-- 测试计数器
local pass_count = 0
local fail_count = 0
local total_count = 0

-- 辅助函数：断言检查并输出结果
local function check(name, condition)
    total_count = total_count + 1
    if condition then
        pass_count = pass_count + 1
        print("  [✓] " .. name)
    else
        fail_count = fail_count + 1
        print("  [✗] " .. name)
    end
end

print("========================================")
print("  Lua if 语句全面测试")
print("========================================")

-- ============================================================================
-- 1. 基础 if 语句（if-then）
-- ============================================================================
print("\n--- 1. 基础 if 语句 ---")

-- 测试1.1：条件为 true 时执行 if 块
local r1 = false
if true then
    r1 = true
end
check("if true 执行 if 块", r1 == true)

-- 测试1.2：条件为 false 时不执行 if 块
local r2 = "unchanged"
if false then
    r2 = "changed"
end
check("if false 不执行 if 块", r2 == "unchanged")

-- 测试1.3：数值条件（非零数值为真）
local r3 = false
if 1 then
    r3 = true
end
check("if 1（非零数值为真）", r3 == true)

-- 测试1.4：数值 0 也为真（Lua 中 0 是真值）
local r4 = false
if 0 then
    r4 = true
end
check("if 0（Lua 中 0 也是真值）", r4 == true)

-- 测试1.5：字符串条件为真
local r5 = false
if "hello" then
    r5 = true
end
check("if \"hello\"（字符串为真）", r5 == true)

-- ============================================================================
-- 2. if-else 语句
-- ============================================================================
print("\n--- 2. if-else 语句 ---")

-- 测试2.1：条件为 true，执行 if 块
local r6 = ""
if true then
    r6 = "if"
else
    r6 = "else"
end
check("true 时执行 if 块", r6 == "if")

-- 测试2.2：条件为 false，执行 else 块
local r7 = ""
if false then
    r7 = "if"
else
    r7 = "else"
end
check("false 时执行 else 块", r7 == "else")

-- 测试2.3：nil 条件执行 else 块
local r8 = ""
if nil then
    r8 = "if"
else
    r8 = "else"
end
check("nil 时执行 else 块", r8 == "else")

-- 测试2.4：非空字符串执行 if 块
local r9 = ""
if "test" then
    r9 = "if"
else
    r9 = "else"
end
check("非空字符串执行 if 块", r9 == "if")

-- ============================================================================
-- 3. if-elseif-else 语句（多个 elseif 分支）
-- ============================================================================
print("\n--- 3. if-elseif-else 语句 ---")

-- 测试3.1：匹配第一个 if 条件
local r10 = ""
local x = 1
if x == 1 then
    r10 = "one"
elseif x == 2 then
    r10 = "two"
elseif x == 3 then
    r10 = "three"
else
    r10 = "other"
end
check("匹配第一个 if (x==1)", r10 == "one")

-- 测试3.2：匹配第一个 elseif 条件
local r11 = ""
x = 2
if x == 1 then
    r11 = "one"
elseif x == 2 then
    r11 = "two"
elseif x == 3 then
    r11 = "three"
else
    r11 = "other"
end
check("匹配第一个 elseif (x==2)", r11 == "two")

-- 测试3.3：匹配第二个 elseif 条件
local r12 = ""
x = 3
if x == 1 then
    r12 = "one"
elseif x == 2 then
    r12 = "two"
elseif x == 3 then
    r12 = "three"
else
    r12 = "other"
end
check("匹配第二个 elseif (x==3)", r12 == "three")

-- 测试3.4：所有条件不满足，执行 else
local r13 = ""
x = 99
if x == 1 then
    r13 = "one"
elseif x == 2 then
    r13 = "two"
elseif x == 3 then
    r13 = "three"
else
    r13 = "other"
end
check("所有条件不满足执行 else (x==99)", r13 == "other")

-- 测试3.5：多个 elseif 无 else
local r14 = "default"
x = 5
if x == 1 then
    r14 = "one"
elseif x == 2 then
    r14 = "two"
elseif x == 3 then
    r14 = "three"
end
check("多个 elseif 无 else 且不匹配", r14 == "default")

-- 测试3.6：大量 elseif 分支
local r15 = ""
x = 5
if x == 1 then r15 = "a"
elseif x == 2 then r15 = "b"
elseif x == 3 then r15 = "c"
elseif x == 4 then r15 = "d"
elseif x == 5 then r15 = "e"
elseif x == 6 then r15 = "f"
else r15 = "z"
end
check("大量 elseif 分支匹配第5个", r15 == "e")

-- ============================================================================
-- 4. 嵌套 if 语句（多层嵌套）
-- ============================================================================
print("\n--- 4. 嵌套 if 语句 ---")

-- 测试4.1：两层嵌套 - 外层和内层都为 true
local r16 = ""
local a = true
local b = true
if a then
    if b then
        r16 = "both_true"
    else
        r16 = "a_only"
    end
else
    r16 = "a_false"
end
check("两层嵌套：外 true 内 true", r16 == "both_true")

-- 测试4.2：两层嵌套 - 外层 true，内层 false
local r17 = ""
a = true
b = false
if a then
    if b then
        r17 = "both_true"
    else
        r17 = "a_only"
    end
else
    r17 = "a_false"
end
check("两层嵌套：外 true 内 false", r17 == "a_only")

-- 测试4.3：两层嵌套 - 外层 false
local r18 = ""
a = false
b = true
if a then
    if b then
        r18 = "both_true"
    else
        r18 = "a_only"
    end
else
    r18 = "a_false"
end
check("两层嵌套：外 false", r18 == "a_false")

-- 测试4.4：三层嵌套
local r19 = ""
local c = true
if true then
    if true then
        if c then
            r19 = "level3"
        end
    end
end
check("三层嵌套全部为 true", r19 == "level3")

-- 测试4.5：嵌套 if 与 elseif 组合
local r20 = ""
local score = 85
if score >= 60 then
    if score >= 90 then
        r20 = "excellent"
    elseif score >= 80 then
        r20 = "good"
    elseif score >= 70 then
        r20 = "average"
    else
        r20 = "pass"
    end
else
    r20 = "fail"
end
check("嵌套 if + elseif (score=85)", r20 == "good")

-- ============================================================================
-- 5. 带有逻辑运算符的 if 语句（and、or、not）
-- ============================================================================
print("\n--- 5. 逻辑运算符 ---")

-- 测试5.1：and - 两个都为 true
local r21 = false
if true and true then
    r21 = true
end
check("true and true", r21 == true)

-- 测试5.2：and - 第一个为 false（短路求值）
local r22 = false
if false and true then
    r22 = true
end
check("false and true（短路为 false）", r22 == false)

-- 测试5.3：and - 第二个为 false
local r23 = false
if true and false then
    r23 = true
end
check("true and false", r23 == false)

-- 测试5.4：or - 两个都为 false
local r24 = false
if false or false then
    r24 = true
end
check("false or false", r24 == false)

-- 测试5.5：or - 第一个为 true（短路求值）
local r25 = false
if true or false then
    r25 = true
end
check("true or false（短路为 true）", r25 == true)

-- 测试5.6：or - 第二个为 true
local r26 = false
if false or true then
    r26 = true
end
check("false or true", r26 == true)

-- 测试5.7：not true
local r27 = false
if not false then
    r27 = true
end
check("not false 为 true", r27 == true)

-- 测试5.8：not false
local r28 = false
if not true then
    r28 = true
end
check("not true 为 false", r28 == false)

-- 测试5.9：复合逻辑表达式
local r29 = false
local p = true
local q = false
if (p or q) and (not q) then
    r29 = true
end
check("(true or false) and (not false)", r29 == true)

-- 测试5.10：and 与 or 的优先级（and 优先于 or）
local r30 = false
if false and true or true then
    r30 = true
end
check("false and true or true（and 优先于 or）", r30 == true)

-- 测试5.11：not 的最高优先级
local r31 = false
if not false and true then
    r31 = true
end
check("not false and true（not 最高优先级）", r31 == true)

-- ============================================================================
-- 6. 带有比较运算符的 if 语句
-- ============================================================================
print("\n--- 6. 比较运算符 ---")

-- 测试6.1：等于 ==
local r32 = false
if 10 == 10 then
    r32 = true
end
check("10 == 10", r32 == true)

-- 测试6.2：等于 == （不等时）
local r33 = false
if 10 == 20 then
    r33 = true
end
check("10 == 20 为 false", r33 == false)

-- 测试6.3：不等于 ~=
local r34 = false
if 10 ~= 20 then
    r34 = true
end
check("10 ~= 20", r34 == true)

-- 测试6.4：不等于 ~= （相等时）
local r35 = false
if 10 ~= 10 then
    r35 = true
end
check("10 ~= 10 为 false", r35 == false)

-- 测试6.5：小于 <
local r36 = false
if 5 < 10 then
    r36 = true
end
check("5 < 10", r36 == true)

-- 测试6.6：小于 < （不满足时）
local r37 = false
if 10 < 5 then
    r37 = true
end
check("10 < 5 为 false", r37 == false)

-- 测试6.7：大于 >
local r38 = false
if 10 > 5 then
    r38 = true
end
check("10 > 5", r38 == true)

-- 测试6.8：大于 > （不满足时）
local r39 = false
if 5 > 10 then
    r39 = true
end
check("5 > 10 为 false", r39 == false)

-- 测试6.9：小于等于 <=
local r40 = false
if 5 <= 10 then
    r40 = true
end
check("5 <= 10", r40 == true)

-- 测试6.10：小于等于 <=（等于时）
local r41 = false
if 10 <= 10 then
    r41 = true
end
check("10 <= 10（等于也满足）", r41 == true)

-- 测试6.11：大于等于 >=
local r42 = false
if 10 >= 5 then
    r42 = true
end
check("10 >= 5", r42 == true)

-- 测试6.12：大于等于 >=（等于时）
local r43 = false
if 10 >= 10 then
    r43 = true
end
check("10 >= 10（等于也满足）", r43 == true)

-- 测试6.13：字符串比较 ==
local r44 = false
if "abc" == "abc" then
    r44 = true
end
check("\"abc\" == \"abc\"", r44 == true)

-- 测试6.14：字符串比较 ~=
local r45 = false
if "abc" ~= "def" then
    r45 = true
end
check("\"abc\" ~= \"def\"", r45 == true)

-- 测试6.15：数值与运算结果比较
local r46 = false
if 2 + 3 == 5 then
    r46 = true
end
check("2 + 3 == 5（表达式比较）", r46 == true)

-- ============================================================================
-- 7. 边界情况测试
-- ============================================================================
print("\n--- 7. 边界情况测试 ---")

-- 测试7.1：nil 为假值
local r47 = "if"
if nil then
    r47 = "if"
else
    r47 = "else"
end
check("nil 为假值", r47 == "else")

-- 测试7.2：false 为假值
local r48 = "if"
if false then
    r48 = "if"
else
    r48 = "else"
end
check("false 为假值", r48 == "else")

-- 测试7.3：0 是真值（Lua 特性：只有 nil 和 false 为假）
local r49 = false
if 0 then
    r49 = true
end
check("0 是真值（Lua 特性）", r49 == true)

-- 测试7.4：空字符串是真值
local r50 = false
if "" then
    r50 = true
end
check("空字符串 \"\" 是真值", r50 == true)

-- 测试7.5：未定义变量为 nil（假值）
local r51 = false
if undefined_variable then
    r51 = true
end
check("未定义变量为 nil（假值）", r51 == false)

-- 测试7.6：负数是真值
local r52 = false
if -1 then
    r52 = true
end
check("负数 -1 是真值", r52 == true)

-- 测试7.7：空表是真值
local r53 = false
if {} then
    r53 = true
end
check("空表 {} 是真值", r53 == true)

-- 测试7.8：nil 和 false 是唯二的假值
local r54 = true
local false_count = 0
if not nil then false_count = false_count + 1 end
if not false then false_count = false_count + 1 end
if not 0 then false_count = false_count + 1 end        -- 0 是真值，not 0 为 false
if not "" then false_count = false_count + 1 end       -- "" 是真值，not "" 为 false
check("nil 和 false 是唯二假值（计数=2）", false_count == 2)

-- 测试7.9：if 块中赋值的局部变量作用域
local r55 = "outer"
if true then
    local r55_inner = "inner"
    r55 = r55_inner
end
check("if 块内局部变量赋值到外部", r55 == "inner")

-- 测试7.10：连续 if 语句（非 elseif，独立判断）
local r56 = 0
local val = 15
if val > 10 then
    r56 = r56 + 1
end
if val > 5 then
    r56 = r56 + 1
end
if val > 20 then
    r56 = r56 + 1
end
check("连续独立 if 语句（val=15，2个满足）", r56 == 2)

-- ============================================================================
-- 测试总结
-- ============================================================================
print("\n========================================")
print("  测试总结")
print("========================================")
print("  总计: " .. total_count)
print("  通过: " .. pass_count)
print("  失败: " .. fail_count)
print("========================================")
if fail_count == 0 then
    print("  [✓] 所有 if 语句测试全部通过！")
else
    print("  [✗] 有 " .. fail_count .. " 个测试失败！")
end
print("========================================")
