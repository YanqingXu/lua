-- Check which standard library functions are missing
print("=== Standard Library Check ===")

-- Check basic functions
print("Checking basic functions:")
print("type:", type(type))
print("tostring:", type(tostring))
print("tonumber:", type(tonumber))
print("print:", type(print))
print("error:", type(error))
print("pcall:", type(pcall))

-- Check missing functions
print("\nChecking potentially missing functions:")
print("loadfile:", type(loadfile))
print("loadstring:", type(loadstring))
print("dofile:", type(dofile))
print("require:", type(require))

-- Check math library
print("\nChecking math library:")
print("math:", type(math))
if type(math) == "table" then
    print("math.abs:", type(math.abs))
    print("math.max:", type(math.max))
    print("math.min:", type(math.min))
end

-- Check string library
print("\nChecking string library:")
print("string:", type(string))
if type(string) == "table" then
    print("string.len:", type(string.len))
    print("string.sub:", type(string.sub))
end

print("\n=== Standard Library Check Complete ===")
