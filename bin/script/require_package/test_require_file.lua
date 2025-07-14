-- Test require with file module
print("Testing require with file module...")

-- Test require with test_module_example
print("Calling require('test_module_example')...")
local test_mod = require('test_module_example')

print("Module loaded:", type(test_mod))
if test_mod then
    print("Module name:", test_mod.name)
    print("Module version:", test_mod.version)
    
    -- Test module functions
    print("Testing greet function:")
    local greeting = test_mod.greet("Lua")
    print("Greeting result:", greeting)
    
    print("Testing increment function:")
    local count1 = test_mod.increment()
    print("Count after first increment:", count1)
    
    local count2 = test_mod.increment()
    print("Count after second increment:", count2)
    
    print("Testing add_message function:")
    test_mod.add_message("Hello World")
    test_mod.add_message("Test Message")
    
    local messages = test_mod.get_messages()
    print("Messages count:", #messages)
    if #messages > 0 then
        print("First message:", messages[1])
        print("Second message:", messages[2])
    end
end

-- Test caching - require the same module again
print("\nTesting module caching...")
local test_mod2 = require('test_module_example')
print("Second require result:", test_mod == test_mod2)
print("Counter value in second reference:", test_mod2.get_counter())

print("\nFile module test completed.")
