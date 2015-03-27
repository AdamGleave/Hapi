# Disable warnings for the external code. It was translated from Fortran, and
# I cannot change it.
set_source_files_properties(RelaxIV_incremental.cc PROPERTIES COMPILE_FLAGS "-Wno-all")
set_source_files_properties(RelaxIV.cc PROPERTIES COMPILE_FLAGS "-Wno-all")
set_source_files_properties(main.cc PROPERTIES COMPILE_FLAGS "-Wno-all")

add_executable (RelaxIV RelaxIV.cc main.cc)
target_link_libraries (RelaxIV PUBLIC ${GLOG_LIBRARY}) 

add_executable (RelaxIV_incremental incremental.cc RelaxIV_incremental.cc dimacs.cc)
target_link_libraries (RelaxIV_incremental PUBLIC ${GLOG_LIBRARY}) 