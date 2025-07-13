-- Debug RK encoding and string constant handling
print("=== Debug RK Encoding Issue ===")

-- Create a simple table
local t = {
    __call = function() return "test" end,
    other = "value"
}

print("\n--- Direct string access ---")
print("t['__call']:", t["__call"])
print("t['__call'] type:", type(t["__call"]))
print("t['__call'] ~= nil:", t["__call"] ~= nil)

print("\n--- Dot notation access ---")
print("t.__call:", t.__call)
print("t.__call type:", type(t.__call))
print("t.__call ~= nil:", t.__call ~= nil)

print("\n--- Other field comparison ---")
print("t['other']:", t["other"])
print("t.other:", t.other)
print("t['other'] == t.other:", t["other"] == t.other)

print("\n--- Variable key access ---")
local key = "__call"
print("key:", key)
print("t[key]:", t[key])
print("t[key] type:", type(t[key]))
print("t[key] ~= nil:", t[key] ~= nil)

print("\n=== Debug completed ===")
