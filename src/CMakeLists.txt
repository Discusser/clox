add_executable(clox ${CLOX_SOURCES})

target_include_directories(
  clox
  PUBLIC ${CLOX_PUBLIC_HEADERS}
  PRIVATE ${CLOX_PRIVATE_HEADERS})
target_link_libraries(clox ${CLOX_LINKS})
