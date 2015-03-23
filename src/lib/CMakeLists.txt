file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library (${PROJECT_NAME_STR} ${SRC_FILES})
# Will add this directory to the include path when building against the resulting library
target_include_directories (${PROJECT_NAME_STR} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (${PROJECT_NAME_STR} PUBLIC ${GLOG_LIBRARY}) 