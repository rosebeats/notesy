# cmake version
cmake_minimum_required( VERSION 3.0 )

# project name
project( notesy )

# use PkgConfig to detect GTK+
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# include and link dirs
include_directories( ./include ./src ${GTK3_INCLUDE_DIRS} )
link_directories(${GTK3_LIBRARY_DIRS})

# target
add_executable( notesy ./src/main.c )

target_link_libraries(notesy ${GTK3_LIBRARIES})

# external libs
# add_subdirectory( ./libs/Library )
# target_link_libraries( notesy library )

# add_library(ex STATIC IMPORTED)
# set_property(TARGET ex PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/Example/exlib.a)
# target_link_libraries( notesy ex )
