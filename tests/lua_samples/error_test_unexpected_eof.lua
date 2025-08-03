-- Test case: Unexpected end of file
-- Expected Lua 5.1 output: "stdin:2: 'end' expected (to close 'function' at line 1)"

function test()
  print("hello")
-- Missing 'end' for function
