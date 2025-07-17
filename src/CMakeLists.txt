add_executable(
  clox
  array.c
  chunk.c
  clox.c
  compiler.c
  debug.c
  memory.c
  object.c
  scanner.c
  table.c
  value.c
  vm.c)

target_include_directories(
  clox
  PUBLIC ${PROJECT_SOURCE_DIR}/include/
  PRIVATE ${PROJECT_SOURCE_DIR}/private/)
target_link_libraries(clox m)
