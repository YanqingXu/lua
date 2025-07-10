-- Final Standard Library Validation Test
-- Comprehensive validation of all Lua standard library functions

print("=== FINAL STANDARD LIBRARY VALIDATION ===")
print("Testing Lua Interpreter Standard Library Implementation")
print("Date: " .. os.date("%Y-%m-%d %H:%M:%S"))
print("")

local test_results = {}
local function add_result(library, test_name, status, details)
    if not test_results[library] then
        test_results[library] = {}
    end
    table.insert(test_results[library], {
        name = test_name,
        status = status,
        details = details or ""
    })
end

-- Test Base Library
print("1. BASE LIBRARY VALIDATION")
local base_tests = {
    {"type() function", function() return type(42) == "number" and type("hello") == "string" end},
    {"tostring() function", function() return tostring(42) == "42" and tostring(true) == "true" end},
    {"tonumber() function", function() return tonumber("123") == 123 and tonumber("abc") == nil end},
    {"print() function", function() print("Print test"); return true end},
    {"assert() function", function() return assert(true) == true end},
    {"error() function", function() return type(error) == "function" end}
}

for _, test in ipairs(base_tests) do
    local name, func = test[1], test[2]
    local success, result = pcall(func)
    add_result("Base", name, success and result, "")
    print("  " .. name .. ": " .. (success and result and "PASS" or "FAIL"))
end

-- Test String Library
print("\n2. STRING LIBRARY VALIDATION")
local string_tests = {
    {"string.len()", function() return string.len("hello") == 5 and string.len("") == 0 end},
    {"string.upper()", function() return string.upper("hello") == "HELLO" end},
    {"string.lower()", function() return string.lower("WORLD") == "world" end},
    {"string.sub()", function() return string.sub("hello", 2, 4) == "ell" end},
    {"string.reverse()", function() return string.reverse("abc") == "cba" end},
    {"string.rep()", function() return string.rep("a", 3) == "aaa" end}
}

for _, test in ipairs(string_tests) do
    local name, func = test[1], test[2]
    local success, result = pcall(func)
    add_result("String", name, success and result, "")
    print("  " .. name .. ": " .. (success and result and "PASS" or "FAIL"))
end

-- Test Math Library
print("\n3. MATH LIBRARY VALIDATION")
local math_tests = {
    {"math.abs()", function() return math.abs(-5) == 5 end},
    {"math.sqrt()", function() return math.sqrt(16) == 4 end},
    {"math.sin()", function() return math.sin(0) == 0 end},
    {"math.cos()", function() return math.cos(0) == 1 end},
    {"math.floor()", function() return math.floor(3.7) == 3 end},
    {"math.ceil()", function() return math.ceil(3.2) == 4 end},
    {"math.min()", function() return math.min(1, 2, 3) == 1 end},
    {"math.max()", function() return math.max(1, 2, 3) == 3 end},
    {"math.pi constant", function() return math.pi > 3.14 and math.pi < 3.15 end}
}

for _, test in ipairs(math_tests) do
    local name, func = test[1], test[2]
    local success, result = pcall(func)
    add_result("Math", name, success and result, "")
    print("  " .. name .. ": " .. (success and result and "PASS" or "FAIL"))
end

-- Test Table Library
print("\n4. TABLE LIBRARY VALIDATION")
local table_tests = {
    {"table.insert()", function()
        local t = {1,2}
        table.insert(t, 3)
        return t[3] == 3
    end},
    {"table.remove()", function()
        local t = {1,2,3}
        local removed = table.remove(t)
        return removed == 3 and #t == 2
    end},
    {"table.concat()", function()
        local t = {1,2,3}
        return table.concat(t, ",") == "1,2,3"
    end},
    {"table.sort()", function()
        local t = {3,1,2}
        table.sort(t)
        return t[1] == 1 and t[2] == 2 and t[3] == 3
    end}
}

for _, test in ipairs(table_tests) do
    local name, func = test[1], test[2]
    local success, result = pcall(func)
    add_result("Table", name, success and result, "")
    print("  " .. name .. ": " .. (success and result and "PASS" or "FAIL"))
end

-- Test IO Library
print("\n5. IO LIBRARY VALIDATION")
local io_tests = {
    {"io.write()", function() io.write(""); return true end},
    {"io.type()", function() return io.type(io.stdout) == "file" end},
    {"io.stdout", function() return type(io.stdout) ~= "nil" end}
}

for _, test in ipairs(io_tests) do
    local name, func = test[1], test[2]
    local success, result = pcall(func)
    add_result("IO", name, success and result, "")
    print("  " .. name .. ": " .. (success and result and "PASS" or "FAIL"))
end

-- Test OS Library
print("\n6. OS LIBRARY VALIDATION")
local os_tests = {
    {"os.time()", function() return type(os.time()) == "number" end},
    {"os.date()", function() return type(os.date()) == "string" end},
    {"os.clock()", function() return type(os.clock()) == "number" end},
    {"os.getenv()", function() return type(os.getenv("PATH")) == "string" end}
}

for _, test in ipairs(os_tests) do
    local name, func = test[1], test[2]
    local success, result = pcall(func)
    add_result("OS", name, success and result, "")
    print("  " .. name .. ": " .. (success and result and "PASS" or "FAIL"))
end

-- Generate Summary Report
print("\n" .. string.rep("=", 60))
print("VALIDATION SUMMARY REPORT")
print(string.rep("=", 60))

local total_tests = 0
local passed_tests = 0

for library, tests in pairs(test_results) do
    local lib_passed = 0
    for _, test in ipairs(tests) do
        total_tests = total_tests + 1
        if test.status then
            passed_tests = passed_tests + 1
            lib_passed = lib_passed + 1
        end
    end
    
    local percentage = math.floor((lib_passed / #tests) * 100)
    print(string.format("%-15s: %2d/%2d tests passed (%3d%%)",
          library .. " Library", lib_passed, #tests, percentage))
end

print(string.rep("-", 60))
local overall_percentage = math.floor((passed_tests / total_tests) * 100)
print(string.format("OVERALL RESULT : %2d/%2d tests passed (%3d%%)",
      passed_tests, total_tests, overall_percentage))

if overall_percentage >= 95 then
    print("\n🎉 EXCELLENT: Standard library implementation is production-ready!")
elseif overall_percentage >= 85 then
    print("\n✅ GOOD: Standard library implementation is functional with minor issues.")
elseif overall_percentage >= 70 then
    print("\n⚠️ FAIR: Standard library needs improvement before production use.")
else
    print("\n❌ POOR: Standard library requires significant work.")
end

print("\nValidation completed at: " .. os.date("%Y-%m-%d %H:%M:%S"))
print("=== END OF VALIDATION ====")
