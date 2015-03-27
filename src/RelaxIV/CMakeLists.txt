add_definitions(-DCMAKE_SRC_DIR="${PROJECT_SOURCE_DIR}")

# Disable warnings for the external code. It was translated from Fortran, and
# I cannot change it.
set_source_files_properties(RelaxIV_incremental.cc PROPERTIES COMPILE_FLAGS "-Wno-all")
set_source_files_properties(RelaxIV.cc PROPERTIES COMPILE_FLAGS "-Wno-all")
set_source_files_properties(main.cc PROPERTIES COMPILE_FLAGS "-Wno-all")

add_executable (RelaxIV RelaxIV.cc main.cc)
target_link_libraries (RelaxIV PUBLIC ${GLOG_LIBRARY}) 

add_executable (RelaxIV_incremental incremental.cc RelaxIV_incremental.cc dimacs.cc)
target_link_libraries (RelaxIV_incremental PUBLIC ${GLOG_LIBRARY})

add_executable (RelaxIV_snapshots snapshots.cc RelaxIV_incremental.cc dimacs.cc)
target_link_libraries (RelaxIV_snapshots PUBLIC ${GLOG_LIBRARY})

add_executable(test_dynamic_memory test_dynamic_memory.cc RelaxIV_incremental.cc dimacs.cc)
target_link_libraries(test_dynamic_memory PRIVATE ${GLOG_LIBRARY} gtest gtest_main)

add_test(relaxiv_dynamic_memory test_dynamic_memory) 