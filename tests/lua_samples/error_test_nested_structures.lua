-- Test case: Nested structures with missing end
-- Expected Lua 5.1 output: "stdin:6: 'end' expected (to close 'function' at line 1)"

function outer()
  if true then
    for i = 1, 10 do
      print(i)
    end
  end
-- Missing 'end' for function
