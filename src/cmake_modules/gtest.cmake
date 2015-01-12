# Credit: https://gist.github.com/oneamtu/3734295/

########################### GTEST
# Add gtest
# http://stackoverflow.com/questions/9689183/cmake-googletest
ExternalProject_Add(
    googletest
    URL http://googletest.googlecode.com/files/gtest-1.7.0.zip
    # TIMEOUT 10
    # # Force separate output paths for debug and release builds to allow easy
    # # identification of correct lib in subsequent TARGET_LINK_LIBRARIES commands
    # CMAKE_ARGS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
    #            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
    #            -Dgtest_force_shared_crt=ON
    # Disable install step
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON)

# Specify include dir
ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIR ${source_dir}/include)

# Library
ExternalProject_Get_Property(googletest binary_dir)
find_package(Threads)

set(GTEST_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a)
set(GTEST_LIBRARY gtest)
add_library(${GTEST_LIBRARY} STATIC IMPORTED)
set_property(TARGET ${GTEST_LIBRARY} PROPERTY IMPORTED_LOCATION
                ${GTEST_LIBRARY_PATH} )
set_property(TARGET ${GTEST_LIBRARY} PROPERTY INTERFACE_INCLUDE_DIRECTORIES 
                ${GTEST_INCLUDE_DIR} )
set_property(TARGET ${GTEST_LIBRARY} PROPERTY INTERFACE_LINK_LIBRARIES
                ${CMAKE_THREAD_LIBS_INIT} )
add_dependencies(${GTEST_LIBRARY} googletest)

set(GTEST_MAIN_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a)
set(GTEST_MAIN_LIBRARY gtest_main)
add_library(${GTEST_MAIN_LIBRARY} STATIC IMPORTED)
set_property(TARGET ${GTEST_MAIN_LIBRARY} PROPERTY IMPORTED_LOCATION
            ${GTEST_MAIN_LIBRARY_PATH} )
set_property(TARGET ${GTEST_MAIN_LIBRARY} PROPERTY INTERFACE_INCLUDE_DIRECTORIES 
                ${GTEST_INCLUDE_DIR} )
set_property(TARGET ${GTEST_LIBRARY} PROPERTY INTERFACE_LINK_LIBRARIES
                ${CMAKE_THREAD_LIBS_INIT} )
add_dependencies(${GTEST_MAIN_LIBRARY} googletest ${GTEST_LIBRARY})
