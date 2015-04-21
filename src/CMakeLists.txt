cmake_minimum_required(VERSION 2.8.12)
set(PROJECT_NAME_STR flowsolver)
project(${PROJECT_NAME_STR} C CXX)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release Custom)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
                                "Reset the configurations to what we need"
                                 FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")

set(GNU_LIKE_FLAGS "-Wall -Wextra -pedantic")
set(GNU_LIKE_CXX_FLAGS "-std=c++11")
set(GNU_LIKE_FLAGS_RELEASE "-O3 -DNDEBUG")
set(GNU_LIKE_FLAGS_DEBUG "-g")
if(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNU_LIKE_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNU_LIKE_FLAGS} ${GNU_LIKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE ${GNU_LIKE_FLAGS_RELEASE})
  set(CMAKE_CXX_FLAGS_RELEASE ${GNU_LIKE_FLAGS_RELEASE})
  set(CMAKE_C_FLAGS_DEBUG ${GNU_LIKE_FLAGS_DEBUG})
  set(CMAKE_CXX_FLAGS_DEBUG ${GNU_LIKE_FLAGS_DEBUG})
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
