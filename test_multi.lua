print("-- math.modf")
local i, f = math.modf(3.7)
print(i, f)

print("-- string.find")
local s, e = string.find("hello world", "world")
print(s, e)

print("-- pairs over table")
local t = {a=1, b=2}
for k,v in pairs(t) do
  print(k, v)
end

