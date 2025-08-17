-- Advanced metamethod tests
-- Test more complex metamethod functionality and combinations

print("=== Advanced Metamethod Tests ===")

-- Test 1: __lt and __le metamethods (comparison operations)
print("Test 1: __lt and __le metamethods")
local Number = {}
Number.__index = Number

function Number.new(value)
    return setmetatable({value = value}, Number)
end

Number.__lt = function(a, b)
    return a.value < b.value
end

Number.__le = function(a, b)
    return a.value <= b.value
end

Number.__tostring = function(n)
    return tostring(n.value)
end

local n1 = Number.new(5)
local n2 = Number.new(10)
local n3 = Number.new(5)

print("  n1 =", tostring(n1))
print("  n2 =", tostring(n2))
print("  n3 =", tostring(n3))
print("  n1 < n2:", n1 < n2)
print("  n2 < n1:", n2 < n1)
print("  n1 <= n3:", n1 <= n3)

-- Test 2: __concat metamethod (string concatenation)
print("\nTest 2: __concat metamethod")
local StringWrapper = {}
StringWrapper.__index = StringWrapper

function StringWrapper.new(str)
    return setmetatable({str = str}, StringWrapper)
end

StringWrapper.__concat = function(a, b)
    local aStr = type(a) == "table" and a.str or tostring(a)
    local bStr = type(b) == "table" and b.str or tostring(b)
    return StringWrapper.new(aStr .. bStr)
end

StringWrapper.__tostring = function(s)
    return s.str
end

local s1 = StringWrapper.new("Hello")
local s2 = StringWrapper.new(" World")
local s3 = s1 .. s2
print("  s1 =", tostring(s1))
print("  s2 =", tostring(s2))
print("  s1 .. s2 =", tostring(s3))

-- Test 3: __unm metamethod (unary minus)
print("\nTest 3: __unm metamethod")
local SignedNumber = {}
SignedNumber.__index = SignedNumber

function SignedNumber.new(value)
    return setmetatable({value = value}, SignedNumber)
end

SignedNumber.__unm = function(n)
    return SignedNumber.new(-n.value)
end

SignedNumber.__tostring = function(n)
    return tostring(n.value)
end

local sn1 = SignedNumber.new(42)
local sn2 = -sn1
print("  sn1 =", tostring(sn1))
print("  -sn1 =", tostring(sn2))

-- Test 4: __div and __mod metamethods
print("\nTest 4: __div and __mod metamethods")
SignedNumber.__div = function(a, b)
    return SignedNumber.new(a.value / b.value)
end

SignedNumber.__mod = function(a, b)
    return SignedNumber.new(a.value % b.value)
end

local sn3 = SignedNumber.new(10)
local sn4 = SignedNumber.new(3)
print("  sn3 / sn4 =", tostring(sn3 / sn4))
print("  sn3 % sn4 =", tostring(sn3 % sn4))

-- Test 5: __pow metamethod (power)
print("\nTest 5: __pow metamethod")
SignedNumber.__pow = function(a, b)
    return SignedNumber.new(a.value ^ b.value)
end

local sn5 = SignedNumber.new(2)
local sn6 = SignedNumber.new(3)
print("  sn5 ^ sn6 =", tostring(sn5 ^ sn6))

-- Test 6: Complex __index chain
print("\nTest 6: Complex __index chain")
local base = {
    baseMethod = function() return "from base" end
}

local middle = {}
setmetatable(middle, {__index = base})
middle.middleMethod = function() return "from middle" end

local derived = {}
setmetatable(derived, {__index = middle})
derived.derivedMethod = function() return "from derived" end

print("  derived.derivedMethod():", derived.derivedMethod())
print("  derived.middleMethod():", derived.middleMethod())
print("  derived.baseMethod():", derived.baseMethod())

-- Test 7: Combination of __index and __newindex
print("\nTest 7: Combination of __index and __newindex")
local ReadOnlyTable = {}

function ReadOnlyTable.new(data)
    local instance = {}
    setmetatable(instance, {
        __index = function(t, k)
            return data[k]
        end,
        __newindex = function(t, k, v)
            error("Attempt to modify a read-only table key: " .. tostring(k))
        end
    })
    return instance
end

local readOnly = ReadOnlyTable.new({a = 1, b = 2, c = 3})
print("  readOnly.a =", readOnly.a)
print("  readOnly.b =", readOnly.b)

-- Try to modify (should error)
print("  Trying to modify read-only table...")
local success, err = pcall(function()
    readOnly.a = 10
end)
if not success then
    print("  ✓ Modification correctly blocked:", err)
else
    print("  ✗ Modification not blocked")
end

-- Test 8: Recursive metamethod calls
print("\nTest 8: Recursive metamethod calls")
local RecursiveTable = {}
setmetatable(RecursiveTable, {
    __index = function(t, k)
        if k == "recursive" then
            return function(depth)
                if depth <= 0 then
                    return "Recursion end"
                else
                    return "Recursion depth " .. depth .. " -> " .. t.recursive(depth - 1)
                end
            end
        end
        return nil
    end
})

print("  RecursiveTable.recursive(3):", RecursiveTable.recursive(3))

print("\n=== Advanced Metamethod Tests Complete ===")
