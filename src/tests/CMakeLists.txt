add_definitions(-DCMAKE_SRC_DIR="${PROJECT_SOURCE_DIR}")

add_executable(test_task_assignment test_task_assignment.cpp)
target_link_libraries(test_task_assignment PRIVATE gtest gtest_main ${PROJECT_NAME_STR})

add_test(task_assignment test_task_assignment)
add_test(NAME flowsolver_tests 
         COMMAND python3 run_tests.py
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
