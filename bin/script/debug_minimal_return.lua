-- Minimal test case for conditional return issue
print("=== Minimal Conditional Return Debug ===")

-- Test 1: Simple return (works)
function simple_return()
    return 42
end

print("Simple return result:", simple_return())

-- Test 2: Conditional return (fails)
function conditional_return(flag)
    if flag then
        return 100
    end
    return 200
end

print("Conditional return (true):", conditional_return(true))
print("Conditional return (false):", conditional_return(false))

-- Test 3: Even simpler conditional
function minimal_conditional()
    if true then
        return 999
    end
end

print("Minimal conditional result:", minimal_conditional())

print("=== End Debug ===")
