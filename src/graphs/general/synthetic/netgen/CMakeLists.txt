add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/generator)

add_custom_command(
  OUTPUT generated/
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate.sh
  DEPENDS generator_netgen
  WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR})
add_custom_target(netgen DEPENDS generated/)
