add_executable(generator_netgen netgen.c index.c random.c)
set_target_properties(generator_netgen PROPERTIES COMPILE_DEFINITIONS "DIMACS")
