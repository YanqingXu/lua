# Lua Standard Library Framework - Simplified Structure

## 📁 Directory Structure

The Lua standard library uses a clean, simplified framework for better maintainability and performance:

```
src/lib/
├── core/                       # Core framework components
│   ├── lib_module.hpp         # Base module interface (LibModule)
│   ├── lib_registry.hpp/.cpp  # Function registration helper (LibRegistry)
│   └── lib_manager.hpp/.cpp   # Standard library manager (StandardLibrary)
│
├── base/                       # Base library (fundamental Lua functions)
│   └── base_lib.hpp/.cpp      # BaseLib implementation
│
├── string/                     # String manipulation library
│   └── string_lib.hpp/.cpp    # StringLib implementation
│
└── math/                       # Mathematical functions library
    └── math_lib.hpp/.cpp      # MathLib implementation
```

## 🎯 Design Principles

### 1. **Simplified Architecture**
- Clean base class interface (`LibModule`) for all libraries
- Direct function registration through `LibRegistry` helper
- Unified initialization via `StandardLibrary` manager
- No complex dependency injection or context management

### 2. **Performance First**
- Direct registration to Lua State (no intermediate layers)
- Minimal overhead for function calls
- Static function implementations for better optimization
- Efficient memory usage with smart pointers

### 3. **Easy to Use**
- Convenient macros for function registration
- Simple inheritance model for new libraries
- Clear separation of concerns
- Comprehensive error handling

## 🔧 Usage Examples

### Initialize All Libraries
```cpp
#include "lib/core/lib_manager.hpp"

// Initialize complete standard library
StandardLibrary::initializeAll(state);
```

### Initialize Specific Libraries
```cpp
#include "lib/base/base_lib.hpp"
#include "lib/string/string_lib.hpp"

// Initialize only specific libraries
initializeBaseLib(state);
initializeStringLib(state);
```

### Create Custom Library
```cpp
#include "lib/core/lib_module.hpp"
#include "lib/core/lib_registry.hpp"

class MyLib : public LibModule {
public:
    const char* getName() const override { return "mylib"; }

    void registerFunctions(State* state) override {
        REGISTER_GLOBAL_FUNCTION(state, myfunction, myfunction);
    }

    void initialize(State* state) override {
        // Optional initialization
    }

    static Value myfunction(State* state, i32 nargs) {
        // Function implementation
        return Value();
    }
};
```

## 🚀 Framework Benefits

### 1. **Simplified Architecture**
- Clean inheritance model with single base class
- Direct function registration without complex layers
- Minimal boilerplate code for new libraries
- Easy to understand and maintain

### 2. **High Performance**
- Direct registration to Lua State (no overhead)
- Static function implementations for optimization
- Efficient memory usage with smart pointers
- Fast library initialization

### 3. **Developer Friendly**
- Convenient macros for common operations
- Comprehensive error handling and validation
- Clear documentation and examples
- Consistent coding patterns

### 4. **Extensible Design**
- Easy to add new standard libraries
- Plugin-friendly architecture
- Modular compilation support
- Future-proof design

## 📝 Development Guidelines

### 1. **File Naming Convention**
- Library implementation: `module_lib.hpp/.cpp`
- Core components: `lib_component.hpp/.cpp`
- Use consistent naming across all modules

### 2. **Include Paths**
- Core framework: `../core/`
- VM components: `../../vm/`
- Common types: `../../common/`
- GC components: `../../gc/core/`

### 3. **Coding Standards**
- Use project type system (`i32`, `f64`, `Str`, etc.)
- English-only comments and documentation
- Proper error handling with exceptions
- Doxygen-style documentation

## 📊 Current Status

- ✅ **Core Framework**: LibModule, LibRegistry, StandardLibrary
- ✅ **Base Library**: print, type, tostring, tonumber, error
- ✅ **String Library**: len, sub, upper, lower, reverse, rep
- ✅ **Math Library**: abs, floor, ceil, sqrt, sin, cos, tan, min, max
- ✅ **Build System**: Complete project builds successfully
- ✅ **Documentation**: Comprehensive API documentation
- ✅ **Testing**: All libraries initialize and function correctly

This simplified framework provides an excellent foundation for Lua standard library development with optimal performance and maintainability.
