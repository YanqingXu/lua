-- 闭包和上值测试
-- 测试闭包、上值捕获等高级函数功能

print("=== 闭包和上值测试 ===")

-- 测试1: 简单闭包
print("测试1: 简单闭包")
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

-- 测试2: 多个函数共享上值
print("\n测试2: 多个函数共享上值")
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
            return nil, "余额不足"
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
print("  初始余额:", account.getBalance())
print("  存入50后:", account.deposit(50))
print("  取出30后:", account.withdraw(30))
print("  当前余额:", account.getBalance())

-- 测试3: 嵌套闭包
print("\n测试3: 嵌套闭包")
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

-- 测试4: 闭包捕获循环变量
print("\n测试4: 闭包捕获循环变量")
local functions = {}
for i = 1, 3 do
    functions[i] = function()
        return i
    end
end

print("  functions[1]() =", functions[1]())
print("  functions[2]() =", functions[2]())
print("  functions[3]() =", functions[3]())

-- 测试5: 正确的循环变量捕获
print("\n测试5: 正确的循环变量捕获")
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

-- 测试6: 闭包修改外部变量
print("\n测试6: 闭包修改外部变量")
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

-- 测试7: 多层嵌套的上值
print("\n测试7: 多层嵌套的上值")
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

-- 测试8: 闭包作为对象方法
print("\n测试8: 闭包作为对象方法")
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
            return "我是" .. privateName .. "，今年" .. privateAge .. "岁"
        end
    }
end

local person = createPerson("张三", 25)
print("  person.getName() =", person.getName())
print("  person.getAge() =", person.getAge())
print("  person.introduce() =", person.introduce())
person.setAge(26)
print("  设置年龄后: person.introduce() =", person.introduce())

print("\n=== 闭包和上值测试完成 ===")
