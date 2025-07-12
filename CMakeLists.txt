cmake_minimum_required(VERSION 3.10)

project(
  clox
  VERSION 1.0
  LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# option(DEBUG_PRINT_CODE "" OFF) option(DEBUG_TRACE_EXECUTION "" OFF)

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type_lower)
if(build_type_lower STREQUAL debug)
  add_compile_definitions(DEBUG_PRINT_CODE)
  add_compile_definitions(DEBUG_TRACE_EXECUTION)
endif()

add_executable(
  clox
  src/array.c
  src/chunk.c
  src/clox.c
  src/compiler.c
  src/debug.c
  src/memory.c
  src/scanner.c
  src/value.c
  src/vm.c)
target_include_directories(
  clox
  PUBLIC include/
  PRIVATE private/)
