-- Comprehensive Standard Library Test
-- Tests all major Lua standard libraries

print("=== Comprehensive Standard Library Test ===")

-- 1. Base Library Test
print("\n1. Base Library:")
print("  type(42):", type(42))
print("  tostring(3.14):", tostring(3.14))
print("  tonumber('123'):", tonumber('123'))

-- 2. String Library Test
print("\n2. String Library:")
if string and string.len then
    print("  string.len('test'):", string.len('test'))
    print("  string.upper('hello'):", string.upper('hello'))
    print("  string.sub('hello', 2, 4):", string.sub('hello', 2, 4))
else
    print("  String library functions not available")
end

-- 3. Math Library Test
print("\n3. Math Library:")
if math and math.abs then
    print("  math.abs(-5):", math.abs(-5))
    print("  math.sqrt(16):", math.sqrt(16))
    print("  math.sin(0):", math.sin(0))
    print("  math.pi:", math.pi)
else
    print("  Math library functions not available")
end

-- 4. Table Library Test
print("\n4. Table Library:")
if table and table.insert then
    local t = {1, 2, 3}
    table.insert(t, 4)
    print("  After table.insert, length:", #t)
    if table.concat then
        print("  table.concat result:", table.concat(t, ", "))
    end
else
    print("  Table library functions not available")
end

-- 5. IO Library Test
print("\n5. IO Library:")
if io and io.write then
    print("  io.write test:")
    io.write("    IO library is working!\n")
    print("  io.type(io.stdout):", io.type and io.type(io.stdout) or "io.type not available")
else
    print("  IO library functions not available")
end

-- 6. OS Library Test
print("\n6. OS Library:")
if os and os.time then
    print("  os.time():", os.time())
    print("  os.clock():", os.clock())
    local date = os.date and os.date("%Y-%m-%d") or "date not available"
    print("  os.date():", date)
else
    print("  OS library functions not available")
end

-- 7. Debug Library Test
print("\n7. Debug Library:")
if debug then
    print("  debug table exists:", type(debug))
    if debug.getinfo then
        local info = debug.getinfo(1, "n")
        print("  debug.getinfo available:", info ~= nil)
    else
        print("  debug.getinfo is nil")
    end
else
    print("  Debug library not available")
end

-- Summary
print("\n=== Library Availability Summary ===")
local libraries = {
    {"Base (type, tostring, etc.)", type(type) == "function"},
    {"String", string and string.len ~= nil},
    {"Math", math and math.abs ~= nil},
    {"Table", table and table.insert ~= nil},
    {"IO", io and io.write ~= nil},
    {"OS", os and os.time ~= nil},
    {"Debug", debug ~= nil}
}

for _, lib in ipairs(libraries) do
    local name, available = lib[1], lib[2]
    local status = available and "✓ Available" or "✗ Not Available"
    print(string.format("  %-25s %s", name, status))
end

print("\n=== Comprehensive Test Complete ===")
