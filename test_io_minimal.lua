-- Minimal I/O test - even simpler
print("Calling io.open...")
local f = io.open("test.txt", "w")
print("Returned from io.open")
print("Type of f:", type(f))
print("Done")

