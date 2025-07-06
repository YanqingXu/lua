#!/bin/bash
# Minimal Lua Interpreter Build Script
# Builds only the core components that are known to work

echo "Building minimal Lua interpreter..."

# Source files that are known to compile
CORE_SOURCES=(
    "src/main_simple.cpp"
    "src/vm/state.cpp"
    "src/vm/value.cpp"
    "src/vm/table.cpp"
    "src/vm/function.cpp"
    "src/lexer/lexer.cpp"
    "src/parser/parser.cpp"
    "src/compiler/symbol_table.cpp"
    "src/lib/base/base_lib.cpp"
    "src/lib/base/lib_base_utils.cpp"
    "src/lib/core/lib_define.cpp"
    "src/lib/core/lib_module.cpp"
    "src/lib/core/lib_func_registry.cpp"
    "src/lib/core/lib_context.cpp"
    "src/gc/gc.cpp"
    "src/common/defines.cpp"
)

# Compiler flags
CXX_FLAGS="-std=c++17 -Wall -Wextra -O2 -I src/"

# Output binary
OUTPUT="bin/lua_minimal"

# Create bin directory if it doesn't exist
mkdir -p bin

# Check which source files actually exist
EXISTING_SOURCES=()
for src in "${CORE_SOURCES[@]}"; do
    if [ -f "$src" ]; then
        EXISTING_SOURCES+=("$src")
    else
        echo "Warning: $src not found, skipping..."
    fi
done

# Compile
echo "Compiling with sources: ${EXISTING_SOURCES[@]}"
g++ $CXX_FLAGS -o $OUTPUT "${EXISTING_SOURCES[@]}"

if [ $? -eq 0 ]; then
    echo "Build successful! Output: $OUTPUT"
    echo "Usage: $OUTPUT script.lua"
else
    echo "Build failed!"
    exit 1
fi
