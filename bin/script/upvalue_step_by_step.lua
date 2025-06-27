-- Step by step upvalue debug
print("=== Step by Step Upvalue Debug ===")

-- Step 1: Define a local variable
print("Step 1: Defining local variable")
local x = 42
print("x =", x)

-- Step 2: Define a function that uses the variable
print("Step 2: Defining function")
function test()
    print("Inside function, trying to access x")
    print("x =", x)
    return x
end

-- Step 3: Call the function
print("Step 3: Calling function")
local result = test()
print("Function returned:", result)

print("=== Debug completed ===")
