-- Direct require test
print("Testing require directly...")

-- Test require with string library
print("Calling require('string')...")
local str_lib = require('string')

print("String library loaded:", type(str_lib))
if str_lib then
    print("String library has len function:", type(str_lib.len))
    print("String library has sub function:", type(str_lib.sub))
end

print("Test completed.")
