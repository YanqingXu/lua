# Lua Standard Library Framework - Modular Structure

## 📁 Directory Structure

The Lua standard library has been reorganized into a modular structure for better maintainability and clarity:

```
src/lib/
├── lua_lib.hpp                 # Main library header (include this for full framework)
│
├── core/                       # Core framework components
│   ├── core.hpp               # Core framework aggregation header
│   ├── lib_define.hpp         # Core definitions and macros
│   ├── lib_module.hpp         # Base module interface
│   ├── lib_context.hpp/.cpp   # Configuration and dependency injection
│   ├── lib_func_registry.hpp/.cpp  # Function registration system
│   └── lib_manager.hpp/.cpp   # Central library manager
│
├── base/                       # Base library (fundamental Lua functions)
│   ├── base_lib.hpp/.cpp     # Modern base library implementation
│   ├── base_lib.hpp/.cpp      # Legacy base library (to be removed)
│   └── lib_base_utils.hpp/.cpp # Base library utilities
│
├── string/                     # String manipulation library
│   ├── string.hpp             # String library aggregation header
│   └── string_lib.hpp/.cpp    # String library implementation
│
├── math/                       # Mathematical functions library
│   ├── math.hpp               # Math library aggregation header
│   └── math_lib.hpp/.cpp      # Math library implementation
│
├── table/                      # Table manipulation library
│   ├── table.hpp              # Table library aggregation header
│   └── table_lib.hpp/.cpp     # Table library implementation
│
└── utils/                      # Common utilities and helpers
    ├── utils.hpp              # Utilities aggregation header
    ├── lib_utils.hpp/.cpp     # General library utilities
    ├── error_handling.hpp/.cpp # Error handling utilities
    └── type_conversion.hpp    # Type conversion utilities
```

## 🎯 Design Principles

### 1. **Modular Organization**
- Each standard library (base, string, math, table) has its own directory
- Core framework components are separated from library implementations
- Utilities are centralized for reuse across modules

### 2. **Clear Dependencies**
- Core framework provides the foundation for all modules
- Libraries depend on core framework and utilities
- No circular dependencies between library modules

### 3. **Aggregation Headers**
- Each module provides a single header file for easy inclusion
- Main `lua_lib.hpp` provides access to the entire framework
- Selective inclusion possible for specific modules

## 🔧 Usage Examples

### Include Entire Framework
```cpp
#include "lib/lua_lib.hpp"

// Initialize complete standard library
Lua::Lib::initializeStandardLibrary(state);
```

### Include Specific Modules
```cpp
#include "lib/core/core.hpp"
#include "lib/base/base_lib.hpp"

// Initialize only base library
auto manager = std::make_unique<Lua::Lib::LibraryManager>();
auto baseLib = std::make_unique<Lua::Lib::Base::BaseLib>();
manager->registerLibrary(std::move(baseLib));
```

### Core Framework Only
```cpp
#include "lib/core/core.hpp"

// Use framework components directly
Lua::Lib::LibContext context;
Lua::Lib::LibFuncRegistry registry;
```

## 🚀 Migration Benefits

### 1. **Better Code Organization**
- Related files are grouped together
- Clear separation of concerns
- Easier navigation and maintenance

### 2. **Improved Build Times**
- Selective compilation possible
- Reduced include dependencies
- Better incremental builds

### 3. **Enhanced Modularity**
- Independent development of library modules
- Easy addition of new libraries
- Plugin-friendly architecture

### 4. **Cleaner Dependencies**
- Explicit dependency relationships
- Reduced coupling between modules
- Better testability

## 📝 Development Guidelines

### 1. **File Naming Convention**
- Module implementation: `module_lib.hpp/.cpp`
- Module aggregation: `module.hpp`
- Core components: `lib_component.hpp/.cpp`

### 2. **Include Path Updates**
- Use relative paths from module directory
- Core framework: `../core/`
- VM components: `../../vm/`
- Common types: `../../common/`

### 3. **Namespace Organization**
- Core framework: `Lua::Lib::Core::`
- Standard libraries: `Lua::Lib::ModuleName::`
- Utilities: `Lua::Lib::Utils::`

## 🔄 Next Steps

1. **Update Include Paths**: Complete the update of all include paths in moved files
2. **Test Compilation**: Verify that all modules compile correctly
3. **Update Build Scripts**: Modify CMake/build files to reflect new structure
4. **Documentation Update**: Update all documentation references
5. **Legacy Cleanup**: Remove old base_lib files after migration complete

## 📊 Migration Status

- ✅ **Directory Structure Created**: All module directories established
- ✅ **Files Moved**: All library files moved to appropriate directories
- 🔄 **Include Paths**: In progress - updating file references
- 🔄 **Build System**: Pending - CMake updates needed
- 🔄 **Testing**: Pending - compilation verification needed
- 🔄 **Documentation**: Pending - reference updates needed

This modular structure provides a solid foundation for the continued development and maintenance of the Lua standard library framework.
