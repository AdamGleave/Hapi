if(CMAKE_COMPILER_IS_GNUCXX)
  # Disable warnings. 
  # This is ugly external code which I cannot change.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-all")
endif()

add_executable (cs2 cs2.c)
set_property(
  TARGET cs2 
  PROPERTY COMPILE_DEFINITIONS PRINT_ANS COST_RESTART
  )
target_link_libraries(cs2 m)
