# Dummy OUTPUT -- doesn't matter
add_custom_command(
  OUTPUT vision_flows
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/fetch.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(vision DEPENDS vision_flows)
