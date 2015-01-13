add_executable(netgen netgen.c index.c random.c)

set_target_properties(netgen PROPERTIES COMPILE_DEFINITIONS "DIMACS")

target_link_libraries(netgen)
