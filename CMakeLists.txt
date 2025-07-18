cmake_minimum_required(VERSION 3.14)

project(
  clox
  VERSION 1.0
  LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CLOX_SOURCES_RELATIVE
    src/native/native.c
    src/array.c
    src/chunk.c
    src/clox.c
    src/compiler.c
    src/debug.c
    src/memory.c
    src/object.c
    src/scanner.c
    src/table.c
    src/value.c
    src/vm.c)
set(CLOX_SOURCES)

foreach(file IN LISTS CLOX_SOURCES_RELATIVE)
  list(APPEND CLOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${file})
endforeach()

set(CLOX_LINKS m)
set(CLOX_PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/)
set(CLOX_PRIVATE_HEADERS ${PROJECT_SOURCE_DIR}/private/)

add_subdirectory(src)
add_subdirectory(tests)
