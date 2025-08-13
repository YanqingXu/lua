-- Comprehensive Object-Oriented Programming Test for Lua 5.1 Interpreter
-- Tests: Classes, Inheritance, Method Calls, Metatables, Varargs, Table Constructors

print("=== Comprehensive OOP Test Suite ===")

-- Test 1: Basic Class Definition and Method Calls
print("\n--- Test 1: Basic Class Definition ---")

-- Define a simple Person class
Person = {}
Person.__index = Person

function Person:new(name, age)
    local obj = {}
    setmetatable(obj, Person)
    obj.name = name or "Unknown"
    obj.age = age or 0
    return obj
end

function Person:introduce()
    return "Hello, I'm " .. self.name .. " and I'm " .. self.age .. " years old."
end

function Person:greet(message)
    return "Hello " .. message .. "! I'm " .. self.name .. "."
end

function Person:celebrateBirthday()
    self.age = self.age + 1
    return "Happy birthday! Now I'm " .. self.age .. " years old."
end

-- Create and test Person objects
local person1 = Person:new("Alice", 25)
print("Person1 introduction:", person1:introduce())
print("Person1 greeting:", person1:greet("World"))
print("Person1 birthday:", person1:celebrateBirthday())

-- Test 2: Inheritance
print("\n--- Test 2: Inheritance ---")

-- Define Student class that inherits from Person
Student = {}
Student.__index = Student
setmetatable(Student, {__index = Person})

function Student:new(name, age, school)
    local obj = Person:new(name, age)
    setmetatable(obj, Student)
    obj.school = school or "Unknown School"
    return obj
end

function Student:introduce()
    return Person.introduce(self) .. " I study at " .. self.school .. "."
end

function Student:study(subject)
    return self.name .. " is studying " .. subject .. " at " .. self.school .. "."
end

-- Create and test Student object
local student1 = Student:new("Bob", 20, "MIT")
print("Student introduction:", student1:introduce())
print("Student studying:", student1:study("Computer Science"))
print("Student greeting:", student1:greet("Professor"))

-- Test 3: Varargs in Object-Oriented Context
print("\n--- Test 3: Varargs in OOP ---")

function Person:addSkills(...)
    if not self.skills then
        self.skills = {}
    end
    
    local args = {...}
    for i = 1, #args do
        table.insert(self.skills, args[i])
    end
    
    return "Added " .. #args .. " skills."
end

function Person:listSkills()
    if not self.skills or #self.skills == 0 then
        return self.name .. " has no skills listed."
    end
    
    local skillList = ""
    for i = 1, #self.skills do
        if i > 1 then
            skillList = skillList .. ", "
        end
        skillList = skillList .. self.skills[i]
    end
    
    return self.name .. "'s skills: " .. skillList
end

-- Test varargs with method calls
print("Adding skills:", person1:addSkills("Programming", "Design", "Communication"))
print("Skills list:", person1:listSkills())

-- Test 4: Advanced Table Constructors
print("\n--- Test 4: Advanced Table Constructors ---")

function Person:setAttributes(attrs)
    for key, value in pairs(attrs) do
        self[key] = value
    end
    return "Attributes updated."
end

-- Test with table constructor
local attributes = {
    hobby = "Reading",
    city = "New York",
    profession = "Engineer"
}

print("Setting attributes:", person1:setAttributes(attributes))
print("Person hobby:", person1.hobby)
print("Person city:", person1.city)

-- Test 5: Complex Method Chaining
print("\n--- Test 5: Method Chaining ---")

function Person:setName(name)
    self.name = name
    return self
end

function Person:setAge(age)
    self.age = age
    return self
end

function Person:setCity(city)
    self.city = city
    return self
end

-- Create a new person with method chaining
local person2 = Person:new()
person2:setName("Charlie"):setAge(30):setCity("San Francisco")
print("Chained person:", person2:introduce())

-- Test 6: Metamethods
print("\n--- Test 6: Metamethods ---")

-- Define a Vector class with metamethods
Vector = {}
Vector.__index = Vector

function Vector:new(x, y)
    local obj = {x = x or 0, y = y or 0}
    setmetatable(obj, Vector)
    return obj
end

function Vector.__add(a, b)
    return Vector:new(a.x + b.x, a.y + b.y)
end

function Vector.__tostring(v)
    return "Vector(" .. v.x .. ", " .. v.y .. ")"
end

function Vector:magnitude()
    return math.sqrt(self.x * self.x + self.y * self.y)
end

-- Test vector operations
local v1 = Vector:new(3, 4)
local v2 = Vector:new(1, 2)
local v3 = v1 + v2

print("Vector 1:", tostring(v1))
print("Vector 2:", tostring(v2))
print("Vector sum:", tostring(v3))
print("V1 magnitude:", v1:magnitude())

print("\n=== All OOP Tests Completed Successfully! ===")
