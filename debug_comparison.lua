-- Debug comparison operations
function isEven(n)
    if n % 2 == 0 then
        return true
    else
        return false
    end
end

print("=== Testing comparison ===")
print("1 % 2 = " .. (1 % 2))
print("2 % 2 = " .. (2 % 2))
print("1 % 2 == 0: " .. tostring(1 % 2 == 0))
print("2 % 2 == 0: " .. tostring(2 % 2 == 0))

print("")
print("=== Testing isEven function ===")
print("isEven(1): " .. tostring(isEven(1)))
print("isEven(2): " .. tostring(isEven(2)))
print("isEven(3): " .. tostring(isEven(3)))
print("isEven(4): " .. tostring(isEven(4)))
