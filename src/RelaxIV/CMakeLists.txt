if(CMAKE_COMPILER_IS_GNUCXX)
  # Disable warnings. 
  # This is ugly external code (translated from Fortran!) which I cannot change.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-all")
endif()
add_executable (RelaxIV RelaxIV.cc main.cc) 
