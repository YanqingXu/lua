local function main()
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

end

local ok, err = pcall(main)
if not ok then
  print('ERR:', err)
end
os.exit(ok and 0 or 1)
