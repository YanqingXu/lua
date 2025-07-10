-- Test string library functions
print("=== Testing String Library Functions ===")

-- Test string table exists
print("type(string):", type(string))

-- Test string.len
print("Testing string.len:")
print("string.len('hello'):", string.len("hello"))
print("string.len(''):", string.len(""))

-- Test string.upper
print("Testing string.upper:")
print("string.upper('hello'):", string.upper("hello"))

-- Test string.lower
print("Testing string.lower:")
print("string.lower('WORLD'):", string.lower("WORLD"))

-- Test string.reverse
print("Testing string.reverse:")
print("string.reverse('hello'):", string.reverse("hello"))

-- Test string.rep
print("Testing string.rep:")
print("string.rep('ab', 3):", string.rep("ab", 3))

-- Test string.sub
print("Testing string.sub:")
print("string.sub('hello', 2, 4):", string.sub("hello", 2, 4))
print("string.sub('hello', 2):", string.sub("hello", 2))

print("String library tests completed!")
