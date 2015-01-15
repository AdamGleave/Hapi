cmake_minimum_required(VERSION 2.8.12)
set(PROJECT_NAME_STR flowsolver)
project(${PROJECT_NAME_STR} C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")

if(CMAKE_COMPILER_IS_GNUCXX)
  # TODO: -O2 removed. Want to make optimisation and debug flags configurable
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -ansi -pedantic -std=c++11 -g")
endif()

# treated as system include folder
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)

find_package(Boost COMPONENTS REQUIRED program_options timer system)

find_package(Glog REQUIRED)
include_directories(${GLOG_INCLUDE_DIRS})

INCLUDE(ExternalProject)
SET_DIRECTORY_PROPERTIES(PROPERTIES EP_BASE ${CMAKE_BINARY_DIR}/ext)
include(gtest)

# Recursive application
add_subdirectory(${PROJECT_SOURCE_DIR}/lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/cs2)
add_subdirectory(${PROJECT_SOURCE_DIR}/graphs/general)
enable_testing()
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
