-- Object-oriented module demonstrating class-like patterns
-- This module shows how to create object-oriented modules using metatables

local M = {}

-- Class definition for Person
local Person = {}
Person.__index = Person

function Person.new(name, age)
    local self = setmetatable({}, Person)
    self.name = name or "Unknown"
    self.age = age or 0
    self.friends = {}
    return self
end

function Person:get_name()
    return self.name
end

function Person:set_name(name)
    if type(name) == "string" and name ~= "" then
        self.name = name
        return true
    end
    return false
end

function Person:get_age()
    return self.age
end

function Person:set_age(age)
    if type(age) == "number" and age >= 0 then
        self.age = age
        return true
    end
    return false
end

function Person:add_friend(friend)
    if type(friend) == "table" and friend.name then
        table.insert(self.friends, friend)
        return true
    end
    return false
end

function Person:get_friends()
    return self.friends
end

function Person:greet()
    return "Hello, I'm " .. self.name .. " and I'm " .. self.age .. " years old."
end

function Person:introduce_to(other)
    if type(other) == "table" and other.name then
        return self.name .. " meets " .. other.name
    end
    return "Invalid introduction"
end

function Person:__tostring()
    return "Person(" .. self.name .. ", " .. self.age .. ")"
end

-- Class definition for Student (inherits from Person)
local Student = setmetatable({}, {__index = Person})
Student.__index = Student

function Student.new(name, age, school)
    local self = Person.new(name, age)
    setmetatable(self, Student)
    self.school = school or "Unknown School"
    self.grades = {}
    return self
end

function Student:get_school()
    return self.school
end

function Student:set_school(school)
    if type(school) == "string" and school ~= "" then
        self.school = school
        return true
    end
    return false
end

function Student:add_grade(subject, grade)
    if type(subject) == "string" and type(grade) == "number" then
        self.grades[subject] = grade
        return true
    end
    return false
end

function Student:get_grade(subject)
    return self.grades[subject]
end

function Student:get_all_grades()
    return self.grades
end

function Student:calculate_average()
    local total = 0
    local count = 0
    for _, grade in pairs(self.grades) do
        total = total + grade
        count = count + 1
    end
    if count > 0 then
        return total / count
    end
    return 0
end

function Student:greet()
    return "Hello, I'm " .. self.name .. ", I'm " .. self.age .. " years old and I study at " .. self.school .. "."
end

function Student:__tostring()
    return "Student(" .. self.name .. ", " .. self.age .. ", " .. self.school .. ")"
end

-- Class definition for Teacher (inherits from Person)
local Teacher = setmetatable({}, {__index = Person})
Teacher.__index = Teacher

function Teacher.new(name, age, subject)
    local self = Person.new(name, age)
    setmetatable(self, Teacher)
    self.subject = subject or "Unknown Subject"
    self.students = {}
    return self
end

function Teacher:get_subject()
    return self.subject
end

function Teacher:set_subject(subject)
    if type(subject) == "string" and subject ~= "" then
        self.subject = subject
        return true
    end
    return false
end

function Teacher:add_student(student)
    if type(student) == "table" and student.name then
        table.insert(self.students, student)
        return true
    end
    return false
end

function Teacher:get_students()
    return self.students
end

function Teacher:get_student_count()
    return #self.students
end

function Teacher:greet()
    return "Hello, I'm " .. self.name .. ", I'm " .. self.age .. " years old and I teach " .. self.subject .. "."
end

function Teacher:teach(lesson)
    lesson = lesson or "a lesson"
    return self.name .. " is teaching " .. lesson .. " to " .. #self.students .. " students."
end

function Teacher:__tostring()
    return "Teacher(" .. self.name .. ", " .. self.age .. ", " .. self.subject .. ")"
end

-- Utility functions for the module
local function create_classroom(teacher, students)
    if type(teacher) ~= "table" or type(students) ~= "table" then
        return nil
    end
    
    local classroom = {
        teacher = teacher,
        students = {},
        name = teacher.subject .. " Class"
    }
    
    for _, student in ipairs(students) do
        if type(student) == "table" and student.name then
            table.insert(classroom.students, student)
            teacher:add_student(student)
        end
    end
    
    return classroom
end

local function simulate_class(classroom)
    if type(classroom) ~= "table" or not classroom.teacher then
        return "Invalid classroom"
    end
    
    local result = {}
    table.insert(result, classroom.teacher:greet())
    table.insert(result, classroom.teacher:teach())
    
    for _, student in ipairs(classroom.students) do
        table.insert(result, student:greet())
    end
    
    return result
end

-- Export classes and functions
M.Person = Person
M.Student = Student
M.Teacher = Teacher
M.create_classroom = create_classroom
M.simulate_class = simulate_class

-- Module metadata
M._NAME = "object_module"
M._VERSION = "1.0.0"
M._DESCRIPTION = "Object-oriented programming examples for package testing"

-- Factory functions
function M.create_person(name, age)
    return Person.new(name, age)
end

function M.create_student(name, age, school)
    return Student.new(name, age, school)
end

function M.create_teacher(name, age, subject)
    return Teacher.new(name, age, subject)
end

-- Demonstration function
function M.demo()
    print("=== Object Module Demo ===")
    
    local teacher = M.create_teacher("Ms. Smith", 35, "Mathematics")
    local student1 = M.create_student("Alice", 16, "High School")
    local student2 = M.create_student("Bob", 17, "High School")
    
    student1:add_grade("Math", 95)
    student1:add_grade("Science", 88)
    student2:add_grade("Math", 92)
    student2:add_grade("Science", 85)
    
    local classroom = M.create_classroom(teacher, {student1, student2})
    local simulation = M.simulate_class(classroom)
    
    for _, line in ipairs(simulation) do
        print(line)
    end
    
    print("Alice's average grade: " .. student1:calculate_average())
    print("Bob's average grade: " .. student2:calculate_average())
    print("Teacher has " .. teacher:get_student_count() .. " students")
end

return M
