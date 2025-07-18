add_executable(clox ${CLOX_SOURCES})

target_include_directories(
  clox
  PUBLIC ${CLOX_PUBLIC_HEADERS}
  PRIVATE ${CLOX_PRIVATE_HEADERS})
target_link_libraries(clox ${CLOX_LINKS})

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type_lower)
if(build_type_lower STREQUAL debug)
  target_compile_definitions(clox PRIVATE DEBUG_PRINT_CODE)
  target_compile_definitions(clox PRIVATE DEBUG_TRACE_EXECUTION)
endif()
