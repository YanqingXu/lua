# Error Handling Tests Documentation

This document describes the comprehensive error handling test suite that has been added to the Lua interpreter project.

## Overview

The error handling test suite provides comprehensive testing of error detection, handling, and recovery mechanisms across all modules of the Lua interpreter. It ensures that the system gracefully handles various error conditions and maintains stability under adverse circumstances.

## Test Structure

### Module-Specific Error Tests

#### 1. Lexer Error Tests (`lexer/test_lexer_error.hpp/cpp`)
- **Invalid Characters**: Tests handling of invalid UTF-8 sequences and control characters
- **String Errors**: Tests unterminated strings, invalid escape sequences
- **Number Errors**: Tests malformed numeric literals
- **Edge Cases**: Tests boundary conditions and unusual input patterns

#### 2. Parser Error Recovery Tests (`parser/error_recovery_test.hpp/cpp`)
- **Basic Synchronization**: Tests parser's ability to recover from syntax errors
- **Balanced Delimiters**: Tests skipping of unmatched parentheses, brackets, braces
- **Expression Recovery**: Tests error recovery in expression parsing
- **Statement Recovery**: Tests error recovery in statement parsing
- **Recovery Reporting**: Tests quality and accuracy of error messages

#### 3. Compiler Error Tests (`compiler/compiler_error_test.hpp/cpp`)
- **Semantic Errors**: Tests undefined variables, functions, redefinitions
- **Type Errors**: Tests type mismatches, invalid operations
- **Scope Errors**: Tests variable scope violations
- **Symbol Table Errors**: Tests symbol table overflow and corruption
- **Code Generation Errors**: Tests bytecode generation failures

#### 4. VM Error Tests (`vm/vm_error_test.hpp/cpp`)
- **Runtime Errors**: Tests division by zero, nil access, type errors
- **Stack Errors**: Tests stack overflow/underflow, corruption
- **Memory Errors**: Tests out-of-memory conditions, memory leaks
- **Bytecode Errors**: Tests invalid/corrupted bytecode handling
- **Error Propagation**: Tests error handling in nested calls, coroutines

#### 5. GC Error Tests (`gc/gc_error_test.hpp/cpp`)
- **Memory Allocation Errors**: Tests allocation failures and recovery
- **Circular References**: Tests detection and cleanup of circular references
- **Collection Errors**: Tests errors during garbage collection
- **Memory Pressure**: Tests behavior under low memory conditions
- **Finalizer Errors**: Tests error handling in object finalizers

### Integration Tests

#### Error Handling Suite (`error_handling_suite.hpp/cpp`)
- **Error Propagation**: Tests error propagation between modules
- **System Recovery**: Tests system-wide error recovery mechanisms
- **Error Reporting**: Tests consistency and quality of error messages
- **Integration Scenarios**: Tests complex error scenarios across modules

## Usage

### Running All Error Handling Tests

```cpp
#include "error_handling_suite.hpp"

// Run comprehensive error handling tests
Lua::Tests::ErrorHandlingSuite::runAllTests();
```

### Running Module-Specific Tests

```cpp
// Run only lexer error tests
Lua::Tests::LexerErrorTestSuite::runAllTests();

// Run only parser error recovery tests
Lua::Tests::ParserErrorRecoveryTest::runAllTests();

// Run only compiler error tests
Lua::Tests::CompilerErrorTest::runAllTests();

// Run only VM error tests
Lua::Tests::VMErrorTest::runAllTests();

// Run only GC error tests
Lua::Tests::GCErrorTest::runAllTests();
```

### Integration with Main Test Suite

The error handling tests are integrated into the main test suite and can be run as part of the complete test execution:

```cpp
#include "test_main.hpp"

// This will include error handling tests
Lua::Tests::runAllTests();
```

## Test Framework Integration

All error handling tests use the unified test framework defined in `test_utils.hpp`:

- **RUN_TEST_MODULE**: For module-level test coordination
- **RUN_TEST_SUITE**: For test suite execution
- **RUN_TEST_GROUP**: For test group organization
- **RUN_TEST**: For individual test execution
- **SAFE_RUN_TEST**: For safe test execution with exception handling

## Error Test Patterns

### 1. Exception-Based Testing
```cpp
void testErrorCondition() {
    try {
        // Code that should cause an error
        performInvalidOperation();
        printTestResult("Error detection", false); // Should not reach here
    } catch (const std::exception& e) {
        printTestResult("Error detection", true); // Expected behavior
    }
}
```

### 2. Return Value Testing
```cpp
void testErrorCondition() {
    bool hasError = performOperationAndCheckError(invalidInput);
    printTestResult("Error detection", hasError);
}
```

### 3. State Validation Testing
```cpp
void testErrorRecovery() {
    // Cause error
    performInvalidOperation();
    
    // Test recovery
    bool recovered = performValidOperation();
    printTestResult("Error recovery", recovered);
}
```

## Expected Behavior

### Error Detection
- All invalid inputs should be detected and reported
- Error messages should be informative and consistent
- Errors should be detected as early as possible in the pipeline

### Error Recovery
- System should recover gracefully from errors
- No memory leaks or resource leaks after errors
- System state should remain consistent after error recovery

### Error Propagation
- Errors should propagate correctly between modules
- Error context should be preserved during propagation
- System should handle cascading errors appropriately

## Adding New Error Tests

### 1. Module-Specific Tests
To add new error tests to an existing module:

1. Add test method declaration to the appropriate header file
2. Implement the test method in the corresponding .cpp file
3. Add the test to the appropriate test group using `SAFE_RUN_TEST`

### 2. New Error Test Categories
To add a new category of error tests:

1. Create new test group method
2. Add individual test methods for the category
3. Call the test group from `runAllTests()`

### 3. Integration Tests
To add new integration tests:

1. Add test method to `ErrorHandlingSuite`
2. Implement cross-module error scenarios
3. Add to `runIntegrationErrorTests()`

## Best Practices

### Test Design
- Test both expected and unexpected error conditions
- Verify error messages are helpful and accurate
- Test error recovery and system stability
- Include edge cases and boundary conditions

### Error Simulation
- Use realistic error scenarios
- Test with various input sizes and patterns
- Simulate resource constraints (memory, stack)
- Test concurrent error conditions where applicable

### Validation
- Verify proper cleanup after errors
- Check for memory leaks and resource leaks
- Validate system state consistency
- Ensure error messages are user-friendly

## Maintenance

### Regular Updates
- Update tests when adding new error conditions
- Maintain test coverage as code evolves
- Review and update error messages for clarity
- Add tests for newly discovered error scenarios

### Performance Considerations
- Error tests should not significantly impact build time
- Use efficient error simulation techniques
- Avoid excessive resource consumption in tests
- Balance comprehensive testing with execution speed

## Troubleshooting

### Common Issues
1. **Tests failing unexpectedly**: Check if error conditions have changed
2. **Memory leaks in tests**: Ensure proper cleanup in test methods
3. **Inconsistent error messages**: Update expected error patterns
4. **Platform-specific failures**: Add platform-specific error handling

### Debugging Tips
- Use detailed error messages in test output
- Add logging to trace error propagation
- Use debugger to examine error conditions
- Check system state before and after errors

This comprehensive error handling test suite ensures the robustness and reliability of the Lua interpreter under various error conditions.
