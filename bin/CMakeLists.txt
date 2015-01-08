add_executable (find_min_cost find_min_cost.cpp)
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(find_min_cost PRIVATE ${Boost_LIBRARIES})
target_link_libraries(find_min_cost PUBLIC ${PROJECT_NAME_STR})

add_executable (dimacs_echo dimacs_echo.cpp)

target_link_libraries (dimacs_echo LINK_PUBLIC ${PROJECT_NAME_STR})
