# Garbage Collector Module

This directory contains the garbage collection implementation for the Lua interpreter.

## Directory Structure

### `core/`
Core garbage collection implementation:
- `gc.hpp` - Main garbage collector class definitions
- `gc.cpp` - Core garbage collection algorithms and logic

### `algorithms/`
Specific GC algorithm implementations:
- Tri-color marking algorithm
- Incremental collection strategies
- Generational collection (future)

### `memory/`
Memory management utilities:
- Memory allocation tracking
- Heap management
- Memory statistics and profiling

### `features/`
Advanced GC features:
- Weak references implementation
- Finalizers and destructors
- Custom collection policies

### `utils/`
Utility types and helper functions:
- `gc_types.hpp` - Core type definitions and enums
- Helper macros and constants
- Debug and profiling utilities

### `integration/`
Integration with VM components:
- VM state integration
- Object lifecycle management
- Cross-module interfaces

## Design Principles

1. **Modularity**: Each subdirectory handles a specific aspect of garbage collection
2. **Performance**: Optimized for minimal pause times and high throughput
3. **Configurability**: Tunable parameters for different use cases
4. **Extensibility**: Easy to add new algorithms and features
5. **Maintainability**: Clear separation of concerns and well-documented interfaces

## Dependencies

- `../vm/` - Virtual machine core types (Value, Table, Function, State)
- `../types.hpp` - Common type definitions
- Standard C++ libraries for containers and algorithms

## Usage

The garbage collector is integrated into the VM through the State class and automatically manages memory for all Lua objects.