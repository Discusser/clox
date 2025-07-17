cmake_minimum_required(VERSION 3.10)

project(
  clox
  VERSION 1.0
  LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type_lower)
if(build_type_lower STREQUAL debug)
  add_compile_definitions(DEBUG_PRINT_CODE)
  add_compile_definitions(DEBUG_TRACE_EXECUTION)
endif()

add_subdirectory(src)
