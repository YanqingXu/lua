-- Minimal loop test
print("Testing minimal while loop:")
local i = 1
while i <= 2 do
    print("i = " .. i)
    i = i + 1
end
print("Done")

print("")
print("Testing minimal repeat-until loop:")
local j = 1
repeat
    print("j = " .. j)
    j = j + 1
until j > 2
print("Done")
