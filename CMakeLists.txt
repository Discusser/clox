cmake_minimum_required(VERSION 3.10)

project(
  clox
  VERSION 1.0
  LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
  clox
  src/array.c
  src/chunk.c
  src/clox.c
  src/debug.c
  src/memory.c
  src/value.c
  src/vm.c)
target_include_directories(
  clox
  PUBLIC include/
  PRIVATE private/)
