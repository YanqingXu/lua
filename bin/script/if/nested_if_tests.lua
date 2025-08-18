-- Nested if statement tests
-- Test complex nested conditional structures

print("=== Nested If Statement Tests ===")

-- Test 1: Simple nesting
print("Test 1: Simple nesting")
local a = 10
local b = 5
if a > 5 then
    print("✓ a is greater than 5")
    if b > 3 then
        print("✓ b is also greater than 3")
    else
        print("✗ b is not greater than 3")
    end
else
    print("✗ a is not greater than 5")
end

-- Test 2: Deep nesting
print("\nTest 2: Deep nesting")
local level1 = true
local level2 = true
local level3 = false
if level1 then
    print("✓ Entered level 1")
    if level2 then
        print("✓ Entered level 2")
        if level3 then
            print("✗ Entered level 3")
        else
            print("✓ Level 3 condition is false")
        end
    else
        print("✗ Level 2 condition is false")
    end
else
    print("✗ Level 1 condition is false")
end

-- Test 3: Complex conditional nesting
print("\nTest 3: Complex conditional nesting")
local age = 25
local hasLicense = true
local hasInsurance = true

if age >= 18 then
    print("✓ Age requirement met")
    if hasLicense then
        print("✓ Has a license")
        if hasInsurance then
            print("✓ Has insurance, can drive")
        else
            print("✗ No insurance, cannot drive")
        end
    else
        print("✗ No license, cannot drive")
    end
else
    print("✗ Age requirement not met")
end

-- Test 4: elseif in nesting
print("\nTest 4: elseif in nesting")
local weather = "sunny"
local temperature = 25

if weather == "sunny" then
    print("✓ The weather is sunny")
    if temperature > 30 then
        print("✗ It is too hot")
    elseif temperature > 20 then
        print("✓ The temperature is pleasant")
    else
        print("✗ It is a bit cold")
    end
elseif weather == "rainy" then
    print("✗ Rainy day")
else
    print("✗ Other weather")
end

-- Test 5: Multiple nested else branches
print("\nTest 5: Multiple nested else branches")
local x = 3
local y = 7

if x > 5 then
    print("✗ x is greater than 5")
    if y > 10 then
        print("✗ y is greater than 10")
    else
        print("✗ y is not greater than 10")
    end
else
    print("✓ x is not greater than 5")
    if y > 5 then
        print("✓ y is greater than 5")
    else
        print("✗ y is not greater than 5")
    end
end

print("\n=== Nested If Statement Tests Completed ===")
