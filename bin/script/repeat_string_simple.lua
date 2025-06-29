-- 测试repeatString函数

print("Testing repeatString...")

function repeatString(str, count)
    local result = ""
    for i = 1, count do
        result = result .. str
    end
    return result
end

local output = repeatString("=", 5)
print("output =", output)

print("Testing the exact line:")
print("=" .. repeatString("=", 40) .. "=")

print("Done.")
