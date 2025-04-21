# Modern C++ Lua Interpreter

This project is a reimplementation of the Lua interpreter using modern C++ techniques and features. The goal is to provide a clean, efficient, and maintainable codebase while preserving the functionality and behavior of the original Lua interpreter.

## Features

- Fully compatible with Lua syntax and semantics.
- Written in modern C++ (C++17 or later).
- Modular and extensible design.
- Improved performance and memory management.
- Comprehensive test suite.

## Getting Started

### Prerequisites

To build and run the project, you will need:

- A C++ compiler that supports C++17 or later (e.g., GCC, Clang, or MSVC).
- CMake (version 3.15 or later).
- A build system (e.g., Make, Ninja, or Visual Studio).

### Building the Project

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd lua
   ```

2. Create a build directory and navigate to it:
   ```bash
   mkdir build
   cd build
   ```

3. Configure the project with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   cmake --build .
   ```

### Running the Interpreter

After building, you can run the Lua interpreter:

```bash
./lua
```

This will start the interactive Lua shell, where you can execute Lua scripts and commands.

### Running Tests

To ensure everything is working correctly, you can run the test suite:

```bash
ctest
```

## Contributing

Contributions are welcome! If you'd like to contribute, please fork the repository, create a feature branch, and submit a pull request. Make sure to follow the project's coding style and include tests for your changes.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgments

This project is inspired by the original Lua interpreter. Special thanks to the Lua team for their work on the Lua programming language.
