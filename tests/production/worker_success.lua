local values = {}
for index = 1, 100 do
    values[index] = index * index
end

assert(values[10] == 100)
