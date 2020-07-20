# cmake version
cmake_minimum_required( VERSION 3.1 )

# project name
project( notesy )

add_subdirectory(notesy_core)

# use PkgConfig to detect GTK+
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# use PkgConfig to detect protobuf-c
pkg_check_modules(PROTOBUF_C REQUIRED libprotobuf-c>=1.0.0)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# include and link dirs
include_directories( ./include ./notesy_core/include ./src ${GTK3_INCLUDE_DIRS} ${PRTOBUF_C_INCLUDE_DIRS} )
# link_directories(${GTK3_LIBRARY_DIRS})

# target
add_executable( notesy ./src/main.c )

# get core lib location
get_target_property(CORE_DIR notesy_core LOCATION)

target_link_libraries(notesy ${GTK3_LIBRARIES})
target_link_libraries(notesy ${PROTOBUF_C_LIBRARIES})
target_link_libraries(notesy ${CORE_DIR}/libnotesy_core.so)
target_link_libraries(notesy Threads::Threads)
target_link_libraries(notesy zmq)

add_dependencies(notesy notesy_core)

# external libs
# add_subdirectory( ./libs/Library )
# target_link_libraries( notesy library )

# add_library(ex STATIC IMPORTED)
# set_property(TARGET ex PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/Example/exlib.a)
# target_link_libraries( notesy ex )
