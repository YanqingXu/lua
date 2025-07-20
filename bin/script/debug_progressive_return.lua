-- Progressive test to find the boundary of the return issue
print("=== Progressive Return Debug ===")

-- Test 1: Simple conditional with constant (works)
function test1()
    if true then
        return 1
    end
    return 2
end
print("Test 1 (simple constant):", test1())

-- Test 2: Conditional with parameter comparison
function test2(x)
    if x > 5 then
        return 10
    end
    return 20
end
print("Test 2 (parameter comparison):", test2(8))

-- Test 3: Conditional with parameter return
function test3(x)
    if x > 5 then
        return x
    end
    return 0
end
print("Test 3 (parameter return):", test3(8))

-- Test 4: Conditional with arithmetic
function test4(x)
    if x > 5 then
        return x * 2
    end
    return x + 10
end
print("Test 4 (arithmetic):", test4(8))

-- Test 5: If-else structure
function test5(x)
    if x > 5 then
        return x
    else
        return 0
    end
end
print("Test 5 (if-else):", test5(8))

print("=== End Debug ===")
