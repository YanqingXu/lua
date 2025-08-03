-- Test case: Multiple syntax errors (should report first one)
-- Expected Lua 5.1 output: "stdin:1: unexpected symbol near '@'"

local x = 1 @
local y = "unfinished string
if true then
-- Multiple errors, but Lua 5.1 typically reports the first one
