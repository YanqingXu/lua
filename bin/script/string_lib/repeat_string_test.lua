-- repeatString函数测试

print("=== RepeatString Test ===")

function repeatString(str, count)
    print("  Function called with str='" .. str .. "', count=" .. count)
    local result = ""
    print("  After result initialization, result='" .. result .. "'")
    
    for i = 1, count do
        print("    Loop i=" .. i)
        result = result .. str
        print("    After concat, result='" .. result .. "'")
    end
    
    print("  Function returning: '" .. result .. "'")
    return result
end

-- 测试调用
print("Before function call")
local output = repeatString("x", 3)
print("After function call, output='" .. output .. "'")

print("=== Test End ===")
