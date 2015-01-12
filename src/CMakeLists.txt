cmake_minimum_required(VERSION 2.8.8)
set(PROJECT_NAME_STR flowsolver)
project(${PROJECT_NAME_STR} C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")

if(CMAKE_COMPILER_IS_GNUCXX)
  # TODO: -O2 removed. Want to make optimisation and debug flags configurable
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -ansi -pedantic -std=c++11 -g")
endif()

# external dependencies
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)
# treated as system include folder
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${BOOST_INCLUDE_DIRS})
set(LIBS ${LIBS} ${BOOST_LIBRARIES})

find_package(Glog REQUIRED)
include_directories(${GLOG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLOG_LIBRARY})

# Recursive application
add_subdirectory(${EXT_PROJECTS_DIR}/gtest)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/cs2)
enable_testing()
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)

