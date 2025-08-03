-- Test case: Invalid escape sequence
-- Expected Lua 5.1 output: "stdin:1: invalid escape sequence near '\"\\z\"'"

local s = "\z"
