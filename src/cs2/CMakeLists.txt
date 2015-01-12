add_executable (cs2 cs2.c)
set_property(
  TARGET cs2 
  PROPERTY COMPILE_DEFINITIONS PRINT_ANS COMP_DUALS COST_RESTART
  )
target_link_libraries(cs2 m)
