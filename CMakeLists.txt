cmake_minimum_required(VERSION 3.20)

project(lua_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LUA_CPP_BUILD_TOOLS "Build the interpreter and bytecode tools" ON)
option(LUA_CPP_BUILD_TESTS "Build the unit test runner" ON)

set(LUA_CORE_SOURCES
    src/compiler/ast.cpp
    src/compiler/codegen.cpp
    src/compiler/codegen_binding.cpp
    src/compiler/codegen_expr.cpp
    src/compiler/codegen_jump.cpp
    src/compiler/codegen_stmt.cpp
    src/compiler/lexer.cpp
    src/compiler/opcode.cpp
    src/compiler/parser.cpp
    src/compiler/parser_expr.cpp
    src/compiler/parser_func.cpp
    src/compiler/parser_primary.cpp
    src/compiler/parser_stmt.cpp
    src/compiler/parser_table.cpp
    src/core/function.cpp
    src/core/gc_object.cpp
    src/core/gc_string.cpp
    src/core/metatable.cpp
    src/core/string_pool.cpp
    src/core/table.cpp
    src/core/upvalue.cpp
    src/core/userdata.cpp
    src/core/value.cpp
    src/core/thread.cpp
    src/gc/garbage_collector.cpp
    src/gc/gc_mark.cpp
    src/gc/gc_sweep.cpp
    src/gc/gc_finalize.cpp
    src/gc/gc_weak.cpp
    src/io/dynamic_buffer.cpp
    src/io/file_loader.cpp
    src/io/input_stream.cpp
    src/lib/baselib.cpp
    src/lib/coroutinelib.cpp
    src/lib/debuglib.cpp
    src/lib/packagelib.cpp
    src/lib/iolib.cpp
    src/lib/lib_catalog.cpp
    src/lib/lib_manager.cpp
    src/lib/lib_registry.cpp
    src/lib/mathlib.cpp
    src/lib/oslib.cpp
    src/lib/stringlib.cpp
    src/lib/tablelib.cpp
    src/vm/global_state.cpp
    src/vm/lua_state.cpp
    src/vm/stack.cpp
    src/vm/vm_arith.cpp
    src/vm/vm_call.cpp
    src/vm/vm_dispatch_strategy.cpp
    src/vm/vm_handlers.cpp
    src/vm/vm_handlers/vm_handlers_arith.cpp
    src/vm/vm_handlers/vm_handlers_branch.cpp
    src/vm/vm_handlers/vm_handlers_call.cpp
    src/vm/vm_handlers/vm_handlers_closure.cpp
    src/vm/vm_handlers/vm_handlers_data.cpp
    src/vm/vm_handlers/vm_handlers_global_upvalue.cpp
    src/vm/vm_handlers/vm_handlers_loop.cpp
    src/vm/vm_handlers/vm_handlers_table.cpp
    src/vm/vm_handlers/vm_handlers_unary.cpp
    src/vm/vm_entry.cpp
    src/vm/vm_frame.cpp
    src/vm/vm_loop.cpp
    src/vm/vm_ops.cpp
    src/vm/vm_table.cpp
    src/vm/vm_trace.cpp
    src/vm/vm.cpp
    src/debug/value_serializer.cpp
    src/debug/json_trace_sink.cpp
)

set(LUA_TEST_SOURCES
    src/app/app_options.cpp
    src/bytecode/bytecode_printer.cpp
    src/repl.cpp
    tests/unit/framework/test_framework.cpp
    tests/unit/framework/test_runner.cpp
    tests/unit/app/test_app_options.cpp
    tests/unit/app/test_repl_commands.cpp
    tests/unit/bytecode/test_bytecode_printer.cpp
    tests/unit/compiler/test_ast_visitor.cpp
    tests/unit/compiler/test_indexed_access.cpp
    tests/unit/compiler/test_lexer_lookahead.cpp
    tests/unit/compiler/test_lexer_number.cpp
    tests/unit/compiler/test_method_call.cpp
    tests/unit/compiler/test_parser_boundaries.cpp
    tests/unit/compiler/test_parser_error_recovery.cpp
    tests/unit/compiler/test_parser_memory_pool.cpp
    tests/unit/compiler/test_parser_recursion.cpp
    tests/unit/compiler/test_storevar.cpp
    tests/unit/compiler/test_lvalue_pipeline.cpp
    tests/unit/compiler/test_value_pipeline.cpp
    tests/unit/compiler/test_syntax_sugar.cpp
    tests/unit/compiler/test_binary_unary_expr.cpp
    tests/unit/compiler/test_codegen_conditions.cpp
    tests/unit/compiler/test_codegen_multret.cpp
    tests/unit/compiler/test_call_pipeline.cpp
    tests/unit/compiler/test_symbol_binding.cpp
    tests/unit/compiler/test_codegen_result_types.cpp
    tests/unit/compiler/test_codegen_state.cpp
    tests/unit/compiler/test_bytecode_builder.cpp
    tests/unit/compiler/test_function_codegen.cpp
    tests/unit/compiler/test_lua_functions.cpp
    tests/unit/core/test_function.cpp
    tests/unit/core/test_gc_string.cpp
    tests/unit/core/test_table.cpp
    tests/unit/core/test_value.cpp
    tests/unit/gc/test_gc.cpp
    tests/unit/io/test_dynamic_buffer.cpp
    tests/unit/io/test_file_loader.cpp
    tests/unit/io/test_input_stream_file.cpp
    tests/unit/io/test_input_stream_stream.cpp
    tests/unit/io/test_input_stream_string.cpp
    tests/unit/metamethod/test_metamethod_arith.cpp
    tests/unit/metamethod/test_metamethod_complete.cpp
    tests/unit/stdlib/test_baselib.cpp
    tests/unit/stdlib/test_coroutinelib.cpp
    tests/unit/stdlib/test_debuglib.cpp
    tests/unit/stdlib/test_lib_catalog.cpp
    tests/unit/stdlib/test_mathlib.cpp
    tests/unit/stdlib/test_packagelib.cpp
    tests/unit/stdlib/test_oslib.cpp
    tests/unit/stdlib/test_stringlib.cpp
    tests/unit/stdlib/test_tablelib.cpp
    tests/unit/vm/test_function_call.cpp
    tests/unit/vm/test_lua_state_init.cpp
    tests/unit/vm/test_runtime_services.cpp
    tests/unit/vm/test_vm_trace_debug.cpp
    tests/unit/vm/test_vm_dispatch.cpp
    tests/unit/vm/test_vm_internal_boundaries.cpp
    tests/unit/vm/test_vm_core.cpp
)

add_library(lua_core STATIC ${LUA_CORE_SOURCES})
target_include_directories(lua_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(CMAKE_DL_LIBS)
    target_link_libraries(lua_core PUBLIC ${CMAKE_DL_LIBS})
endif()

if(MSVC)
    target_compile_options(lua_core PRIVATE /utf-8 /FS)
endif()

if(LUA_CPP_BUILD_TOOLS)
    add_executable(lua_app
        src/app/app_options.cpp
        src/bytecode/bytecode_printer.cpp
        src/main.cpp
        src/repl.cpp
    )
    target_link_libraries(lua_app PRIVATE lua_core)
    target_include_directories(lua_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    add_executable(lua_bytecode
        src/bytecode/bytecode_main.cpp
        src/bytecode/bytecode_printer.cpp
    )
    target_link_libraries(lua_bytecode PRIVATE lua_core)
    target_include_directories(lua_bytecode PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    if(MSVC)
        target_compile_options(lua_app PRIVATE /utf-8 /FS)
        target_compile_options(lua_bytecode PRIVATE /utf-8 /FS)
    endif()
endif()

include(CTest)

if(BUILD_TESTING AND LUA_CPP_BUILD_TESTS)
    add_executable(lua_test ${LUA_TEST_SOURCES})
    target_link_libraries(lua_test PRIVATE lua_core)
    target_include_directories(lua_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/lua_test/include
    )

    if(MSVC)
        target_compile_options(lua_test PRIVATE /utf-8 /FS)
    elseif(UNIX)
        target_link_options(lua_test PRIVATE -rdynamic)
    endif()

    add_test(NAME lua_test COMMAND $<TARGET_FILE:lua_test>)
    set_tests_properties(lua_test PROPERTIES
        LABELS unit
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(LUA_CPP_BUILD_TOOLS)
        add_test(NAME example_hello COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.lua)
        add_test(NAME example_control_flow COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/control_flow.lua)
        add_test(NAME example_tables_and_methods COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/tables_and_methods.lua)
        add_test(NAME example_metamethods COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/metamethods.lua)
        set_tests_properties(
            example_hello
            example_control_flow
            example_tables_and_methods
            example_metamethods
            PROPERTIES
                LABELS examples
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()
