if(CMAKE_COMPILER_IS_GNUCXX)
  # Disable warnings. 
  # This is ugly external code which I cannot change.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-all")
endif()

configure_file(cs2 cs2 COPYONLY)

add_executable (cs2.exe cs2.c)
set_property(
  TARGET cs2.exe
  PROPERTY COMPILE_DEFINITIONS PRINT_ANS COST_RESTART
  )
target_link_libraries(cs2.exe m)
