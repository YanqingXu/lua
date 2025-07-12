-- Test constant index limits
print("=== Testing Constant Index Limits ===")

-- Test 1: Basic constants (should work)
local t1 = {
    a = 1, b = 2, c = 3, d = 4, e = 5,
    f = 6, g = 7, h = 8, i = 9, j = 10
}
print("t1.a =", t1.a, "t1.j =", t1.j)

-- Test 2: Many string constants (may hit limit)
local t2 = {
    key001 = "value001", key002 = "value002", key003 = "value003",
    key004 = "value004", key005 = "value005", key006 = "value006",
    key007 = "value007", key008 = "value008", key009 = "value009",
    key010 = "value010", key011 = "value011", key012 = "value012",
    key013 = "value013", key014 = "value014", key015 = "value015",
    key016 = "value016", key017 = "value017", key018 = "value018",
    key019 = "value019", key020 = "value020", key021 = "value021",
    key022 = "value022", key023 = "value023", key024 = "value024",
    key025 = "value025", key026 = "value026", key027 = "value027",
    key028 = "value028", key029 = "value029", key030 = "value030"
}

print("t2.key001 =", t2.key001)
print("t2.key030 =", t2.key030)

-- Test 3: Check if we reach the limit
print("Testing access to multiple keys...")
print("t2.key015 =", t2.key015)
print("t2.key025 =", t2.key025)

print("=== Test completed ===")
