add_executable(generator_goto goto.c)
target_link_libraries(generator_goto m)

add_custom_command(
  OUTPUT generated/
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate.sh
  DEPENDS generator_goto
  WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR})
add_custom_target(goto DEPENDS generated/)
