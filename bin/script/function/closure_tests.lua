-- Closure and upvalue tests
-- Test closures and upvalue capturing for advanced function features

print("=== Closure and Upvalue Tests ===")

-- Test 1: Simple closure
print("Test 1: Simple closure")
function createCounter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end

local counter1 = createCounter()
local counter2 = createCounter()

print("  counter1() =", counter1())  -- 1
print("  counter1() =", counter1())  -- 2
print("  counter2() =", counter2())  -- 1
print("  counter1() =", counter1())  -- 3

-- Test 2: Multiple functions sharing upvalues
print("\nTest 2: Multiple functions sharing upvalues")
function createAccount(initialBalance)
    local balance = initialBalance
    
    local function deposit(amount)
        balance = balance + amount
        return balance
    end
    
    local function withdraw(amount)
        if balance >= amount then
            balance = balance - amount
            return balance
        else
            return nil, "Insufficient balance"
        end
    end
    
    local function getBalance()
        return balance
    end
    
    return {
        deposit = deposit,
        withdraw = withdraw,
        getBalance = getBalance
    }
end

local account = createAccount(100)
print("  Initial balance:", account.getBalance())
print("  After depositing 50:", account.deposit(50))
print("  After withdrawing 30:", account.withdraw(30))
print("  Current balance:", account.getBalance())

-- Test 3: Nested closures
print("\nTest 3: Nested closures")
function createMultiLevelCounter()
    local outerCount = 0
    
    return function()
        outerCount = outerCount + 1
        local innerCount = 0
        
        return function()
            innerCount = innerCount + 1
            return outerCount, innerCount
        end
    end
end

local outerCounter = createMultiLevelCounter()
local innerCounter1 = outerCounter()
local innerCounter2 = outerCounter()

print("  innerCounter1():", innerCounter1())  -- 1, 1
print("  innerCounter1():", innerCounter1())  -- 1, 2
print("  innerCounter2():", innerCounter2())  -- 2, 1
print("  innerCounter1():", innerCounter1())  -- 1, 3

-- Test 4: Closure capturing loop variables
print("\nTest 4: Closure capturing loop variables")
local functions = {}
for i = 1, 3 do
    functions[i] = function()
        return i
    end
end

print("  functions[1]() =", functions[1]())
print("  functions[2]() =", functions[2]())
print("  functions[3]() =", functions[3]())

-- Test 5: Correct loop variable capture
print("\nTest 5: Correct loop variable capture")
local correctFunctions = {}
for i = 1, 3 do
    correctFunctions[i] = (function(x)
        return function()
            return x
        end
    end)(i)
end

print("  correctFunctions[1]() =", correctFunctions[1]())
print("  correctFunctions[2]() =", correctFunctions[2]())
print("  correctFunctions[3]() =", correctFunctions[3]())

-- Test 6: Closure modifying outer variable
print("\nTest 6: Closure modifying outer variable")
function createToggle(initialState)
    local state = initialState
    
    return function()
        state = not state
        return state
    end
end

local toggle = createToggle(false)
print("  toggle() =", toggle())  -- true
print("  toggle() =", toggle())  -- false
print("  toggle() =", toggle())  -- true

-- Test 7: Multi-level upvalues
print("\nTest 7: Multi-level upvalues")
function level1()
    local var1 = "level1"
    
    function level2()
        local var2 = "level2"
        
        function level3()
            local var3 = "level3"
            
            return function()
                return var1 .. " -> " .. var2 .. " -> " .. var3
            end
        end
        
        return level3()
    end
    
    return level2()
end

local deepClosure = level1()
print("  deepClosure() =", deepClosure())

-- Test 8: Closures as object methods
print("\nTest 8: Closures as object methods")
function createPerson(name, age)
    local privateName = name
    local privateAge = age
    
    return {
        getName = function()
            return privateName
        end,
        
        getAge = function()
            return privateAge
        end,
        
        setAge = function(newAge)
            if newAge >= 0 then
                privateAge = newAge
                return true
            else
                return false
            end
        end,
        
        introduce = function()
            return "I am " .. privateName .. ", " .. privateAge .. " years old"
        end
    }
end

local person = createPerson("Zhang San", 25)
print("  person.getName() =", person.getName())
print("  person.getAge() =", person.getAge())
print("  person.introduce() =", person.introduce())
person.setAge(26)
print("  After setting age: person.introduce() =", person.introduce())

print("\n=== Closure and Upvalue Tests Completed ===")
