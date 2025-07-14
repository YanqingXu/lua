# Package Library Test Suite

This directory contains comprehensive test scripts to verify the correctness of the require() function and package library implementation in the Lua interpreter.

## Test Structure

### 📁 Test Files Overview

```
├── run_all_tests.lua              # Main test runner (run this first)
├── manual_verification.lua        # Simple manual verification script
├── test_suite_main.lua            # Basic functionality tests
├── test_file_modules.lua          # File-based module tests
├── test_real_world_scenarios.lua  # Real-world usage scenarios
├── test_modules/                  # Test module files
│   ├── simple_module.lua          # Basic module example
│   ├── utility_module.lua         # Utility functions module
│   ├── object_module.lua          # Object-oriented module
│   ├── circular_a.lua             # Circular dependency test A
│   ├── circular_b.lua             # Circular dependency test B
│   └── subdir/
│       └── nested_module.lua      # Subdirectory module test
└── TEST_README.md                 # This file
```

## 🚀 Quick Start

### Option 1: Run All Tests (Recommended)
```bash
lua run_all_tests.lua
```
This runs all test suites and provides a comprehensive verification report.

### Option 2: Manual Verification (Simple)
```bash
lua manual_verification.lua
```
This provides a step-by-step manual verification that's easy to follow.

### Option 3: Individual Test Suites
```bash
# Basic functionality tests
lua test_suite_main.lua

# File-based module tests
lua test_file_modules.lua

# Real-world scenario tests
lua test_real_world_scenarios.lua
```

## 📋 Test Categories

### 1. Basic Functionality Tests (`test_suite_main.lua`)
- ✅ Package system structure verification
- ✅ Standard library integration
- ✅ package.searchpath functionality
- ✅ Basic require() with preload
- ✅ Error handling and edge cases
- ✅ Package.path modification
- ✅ Module caching behavior

### 2. File-based Module Tests (`test_file_modules.lua`)
- ✅ Simple module loading from files
- ✅ Utility module with standard library usage
- ✅ Object-oriented module patterns
- ✅ Module caching with file modules
- ✅ Subdirectory module loading
- ✅ Circular dependency detection
- ✅ loadfile() and dofile() functionality

### 3. Real-world Scenario Tests (`test_real_world_scenarios.lua`)
- ✅ Modular application development
- ✅ Library ecosystem simulation
- ✅ Configuration and initialization patterns
- ✅ Error handling and recovery
- ✅ Performance and memory patterns
- ✅ Standard library integration

### 4. Manual Verification (`manual_verification.lua`)
- ✅ Step-by-step interactive verification
- ✅ Simple pass/fail checks
- ✅ User-friendly output
- ✅ Basic functionality confirmation

## 🎯 What Each Test Validates

### Core Package System
- `package` table exists with all required fields
- `package.path`, `package.loaded`, `package.preload`, `package.loaders`
- `require()`, `loadfile()`, `dofile()` functions exist and work
- `package.searchpath()` function works correctly

### Module Loading
- Basic module loading with `require()`
- Module caching (same module returns same object)
- Module search using `package.path` patterns
- Preloaded modules via `package.preload`
- Subdirectory module loading

### Error Handling
- Non-existent module errors
- Circular dependency detection
- Invalid argument handling
- Graceful error recovery

### Integration
- Standard libraries properly registered in `package.loaded`
- Custom modules can use standard libraries
- No conflicts between modules
- VM context integration

### Performance
- Module loading performance
- Memory usage patterns
- Caching effectiveness

## 📊 Expected Results

### Successful Test Run
```
=== Comprehensive Package Library Test Runner ===
✅ Basic Functionality Tests completed successfully
✅ File-based Module Tests completed successfully  
✅ Real-world Scenario Tests completed successfully

🎉 ALL TESTS PASSED! Package library is working correctly.
```

### Test Statistics
- **Total test suites**: 3
- **Individual tests**: 50+
- **Test modules**: 6
- **Scenarios covered**: 15+

## 🔧 Troubleshooting

### Common Issues

1. **File Not Found Errors**
   - Ensure all test module files are in the correct directories
   - Check that `test_modules/` directory exists with all files

2. **Module Loading Failures**
   - Verify the package library is properly compiled and linked
   - Check that `require()` function is available

3. **Path Issues**
   - Run tests from the directory containing the test files
   - Ensure relative paths in `package.path` are correct

### Debug Mode
To enable verbose output in tests, modify the test configuration:
```lua
local TEST_CONFIG = {
    verbose = true,
    stop_on_error = false
}
```

## 📝 Test Module Examples

### Simple Module Pattern
```lua
-- test_modules/simple_module.lua
local M = {}
M.name = "simple_module"
M.version = "1.0.0"

function M.greet(name)
    return "Hello, " .. (name or "World") .. "!"
end

return M
```

### Utility Module Pattern
```lua
-- test_modules/utility_module.lua
local M = {}

M.string_utils = {}
function M.string_utils.trim(str)
    return string.match(str, "^%s*(.-)%s*$")
end

return M
```

### Object-Oriented Module Pattern
```lua
-- test_modules/object_module.lua
local M = {}

local Person = {}
Person.__index = Person

function Person.new(name, age)
    local self = setmetatable({}, Person)
    self.name = name
    self.age = age
    return self
end

M.Person = Person
return M
```

## 🎉 Success Criteria

The package library implementation is considered successful if:

1. ✅ All test suites pass without errors
2. ✅ Module loading works with file-based modules
3. ✅ Caching prevents duplicate loading
4. ✅ Error handling is appropriate and informative
5. ✅ Standard library integration is seamless
6. ✅ Real-world usage patterns work correctly
7. ✅ Performance is acceptable for production use

## 📞 Support

If tests fail or you encounter issues:

1. Check the error messages for specific failure details
2. Verify all test files are present and accessible
3. Ensure the package library is properly compiled
4. Review the implementation against Lua 5.1 specifications
5. Check VM context integration and calling mechanisms

## 🔄 Continuous Testing

For ongoing development, run the test suite regularly:
```bash
# Quick verification
lua manual_verification.lua

# Full test suite
lua run_all_tests.lua
```

The test suite is designed to be:
- **Fast**: Completes in under 10 seconds
- **Comprehensive**: Covers all major functionality
- **Reliable**: Consistent results across runs
- **Informative**: Clear pass/fail indicators and error messages
