function m()
    return 10, 20, 30
end

function pass(...) 
    return ... 
end

print("Testing m() directly:")
local a, b, c = m()
print("m() ->", a, b, c)

print("Testing pass(m()):")
local x, y = pass(m())
print("pass(m()) ->", x, y)
