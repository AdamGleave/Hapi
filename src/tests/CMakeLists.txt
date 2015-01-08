# TODO: Do I need to add this to top-level CMakeLists?
include_directories(${gtest_SOURCE_DIR}/include)
add_definitions(-DCMAKE_SRC_DIR="${PROJECT_SOURCE_DIR}")

add_executable(test_task_assignment test_task_assignment.cpp)
target_link_libraries(test_task_assignment PRIVATE gtest gtest_main ${PROJECT_NAME_STR})

add_test(task_assignment test_task_assignment)