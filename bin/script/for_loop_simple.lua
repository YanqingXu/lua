-- 简单for循环测试

print("Testing for loop...")

function repeatString(str, count)
    local result = ""
    for i = 1, count do
        result = result .. str
    end
    return result
end

local output = repeatString("x", 3)
print("output =", output)

print("Done.")
