-- Test simple module
print("Testing simple module...")

local simple_mod = require('simple_test')

print("Simple module loaded:", type(simple_mod))
if simple_mod then
    print("Module name:", simple_mod.name)
    print("Module version:", simple_mod.version)
    print("Hello function:", simple_mod.hello())
end

print("Simple module test completed.")
