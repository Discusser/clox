add_executable(clox-test ${CLOX_SOURCES})

target_include_directories(
  clox-test
  PUBLIC ${PROJECT_SOURCE_DIR}/include/
  PRIVATE ${PROJECT_SOURCE_DIR}/private/)
target_link_libraries(clox-test ${CLOX_LINKS})

file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/main.py
     ${CMAKE_CURRENT_BINARY_DIR}/main.py SYMBOLIC)
add_custom_target(test COMMAND python main.py ${CMAKE_BUILD_TYPE})
