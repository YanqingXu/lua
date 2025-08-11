-- 调试 test_multi_ret.lua 的问题

print('-- multi return from function')
function m()
  return 10, 20, 30
end
local a, b, c = m()
print('m ->', a, b, c)

print('-- multi return through call chain')
function pass(...) return ... end
local x,y = pass(m())
print('pass(m()) ->', x, y)

print('-- 测试完成')
