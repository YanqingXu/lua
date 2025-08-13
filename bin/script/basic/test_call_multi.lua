print('-- string.find returns 2 values:')
local s, e = string.find('hello world','world')
print('find', s, e)

print('-- select multi returns:')
print(select(1,'a','b','c'))

