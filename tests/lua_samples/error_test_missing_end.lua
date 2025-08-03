-- Test case: Missing 'end' keyword
-- Expected Lua 5.1 output: "stdin:3: 'end' expected (to close 'if' at line 1)"

if true then
  print("hello world")
-- Missing 'end' here
