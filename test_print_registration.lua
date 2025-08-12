-- Test if print function is registered
local p = print
if p then
    p("Print function is registered")
else
    -- This won't work if print isn't registered
end
