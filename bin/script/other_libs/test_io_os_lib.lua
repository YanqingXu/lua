-- IO and OS Library Tests
-- Testing basic IO and OS functions

print("=== IO Library Tests ===")

-- Test io.write (should be available)
if io.write then
    print("Testing io.write:")
    io.write("Hello from io.write!\n")
    io.write("Multiple ", "arguments ", "test\n")
else
    print("io.write is nil")
end

-- Test io.output/input redirection (basic test)
if io.output then
    print("io.output is available")
else
    print("io.output is nil")
end

if io.input then
    print("io.input is available")
else
    print("io.input is nil")
end

-- Test io.type (checks if something is a file)
if io.type then
    print("io.type(io.stdout):", io.type(io.stdout))
else
    print("io.type is nil")
end

print("\n=== OS Library Tests ===")

-- Test os.time
if os.time then
    local current_time = os.time()
    print("os.time():", current_time)
else
    print("os.time is nil")
end

-- Test os.date
if os.date then
    local date_str = os.date()
    print("os.date():", date_str)
else
    print("os.date is nil")
end

-- Test os.clock
if os.clock then
    local clock_time = os.clock()
    print("os.clock():", clock_time)
else
    print("os.clock is nil")
end

-- Test os.getenv (environment variables)
if os.getenv then
    local path = os.getenv("PATH")
    if path then
        print("os.getenv('PATH') exists, length:", #path)
    else
        print("os.getenv('PATH') returned nil")
    end
else
    print("os.getenv is nil")
end

print("=== IO and OS Tests Complete ===")
