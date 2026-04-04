print("=== I/O Core Regression ===")

local path = "test_iolib_core.txt"
local f = io.open(path, "w")
assert(f, "io.open should return file handle")
assert(io.type(f) == "file", "open handle type should be file")

local str = tostring(f)
assert(type(str) == "string", "tostring(file) should return string")

f:write("alpha\n")
f:write("beta\n")
f:flush()
f:close()
assert(io.type(f) == "closed file", "closed handle type should be closed file")

local rf = io.open(path, "r")
assert(rf, "reopen for reading should succeed")
assert(rf:read("*l") == "alpha", "first line should match")
local pos = rf:seek("cur", 0)
assert(type(pos) == "number", "seek should return numeric position")
assert(rf:read("*l") == "beta", "second line should match")
rf:close()

local tf = io.tmpfile()
assert(tf, "tmpfile should succeed")
tf:write("temp-data")
tf:seek("set", 0)
assert(tf:read("*a") == "temp-data", "tmpfile readback should match")
tf:close()

local ok, err = pcall(function()
    for line in io.lines(path) do
        print(line)
    end
end)
assert(not ok, "io.lines should currently fail clearly")
assert(type(err) == "string", "io.lines failure should be string error")

print("I/O core regression passed")
