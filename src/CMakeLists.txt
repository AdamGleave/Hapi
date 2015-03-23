cmake_minimum_required(VERSION 2.8.12)
set(PROJECT_NAME_STR flowsolver)
project(${PROJECT_NAME_STR} C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")

set(GNU_LIKE_FLAGS "-Wall -Wextra -pedantic -std=c++11 -g")
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS ${GNU_LIKE_FLAGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS ${GNU_LIKE_FLAGS})
endif()

# treated as system include folder
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)

find_package(Boost COMPONENTS REQUIRED program_options timer system)

find_package(Glog REQUIRED)
include_directories(${GLOG_INCLUDE_DIRS})

find_package(LEMON REQUIRED)

INCLUDE(ExternalProject)
SET_DIRECTORY_PROPERTIES(PROPERTIES EP_BASE ${CMAKE_BINARY_DIR}/ext)
include(gtest)

# Recursive application
add_subdirectory(${PROJECT_SOURCE_DIR}/lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/cs2)
add_subdirectory(${PROJECT_SOURCE_DIR}/RelaxIV)
add_subdirectory(${PROJECT_SOURCE_DIR}/graphs/general)
enable_testing()
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
