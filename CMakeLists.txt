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

# find pthread package
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# directory containing protobuf c files
set(PROTOBUF_DIR ${CMAKE_CURRENT_BINARY_DIR}/protobuf)
file(MAKE_DIRECTORY ${PROTOBUF_DIR})

# find .proto files
file(GLOB_RECURSE PROTOBUF_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/notesy_core ${CMAKE_CURRENT_SOURCE_DIR}/notesy_core/protobuf/**.proto)
#execute_process(COMMAND bash "-c" "echo '${_PROTOBUF_SRCS}' | sed -r 's/\.proto$/.pb-c.c/' | sed -r 's/\.proto;/.pb-c.c;/' | tr -d '\n'"
#    OUTPUT_VARIABLE _PROTOBUF_SRCS)
foreach(SRC IN LISTS PROTOBUF_SRCS)
    string(REGEX REPLACE \.proto$ .pb-c.c SRC ${SRC})
    get_filename_component(SRC ${SRC} REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/protobuf")
    list(APPEND PROTOBUF_OUTPUT ${SRC})
endforeach(SRC)
file(GLOB_RECURSE PROTOBUF_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/notesy_core/protobuf/**.proto)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/protobuf/protobuf)

# include and link dirs
include_directories( ./include ./notesy_core/include ./src ${PROTOBUF_DIR} ${GTK3_INCLUDE_DIRS} ${PROTOBUF_C_INCLUDE_DIRS} )
# link_directories(${GTK3_LIBRARY_DIRS})

# target
add_executable( notesy ./src/main.c ./src/thread_comm.c ${PROTOBUF_OUTPUT} )

# get core lib location
get_target_property(CORE_DIR notesy_core LOCATION)

# link various libraries
target_link_libraries(notesy ${GTK3_LIBRARIES})
target_link_libraries(notesy ${PROTOBUF_C_LIBRARIES})
target_link_libraries(notesy ${CORE_DIR}/libnotesy_core.so)
target_link_libraries(notesy Threads::Threads)
target_link_libraries(notesy zmq)

# add protobuf source generation target
add_custom_command(
    OUTPUT ${PROTOBUF_OUTPUT}
    DEPENDS ${PROTOBUF_SRCS}
    COMMAND rm -rf ${PROTOBUF_DIR}/protobuf
    COMMAND protoc --c_out=${PROTOBUF_DIR} protobuf/*
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/notesy_core/)

# add notesy_core
add_dependencies(notesy notesy_core)

# external libs
# add_subdirectory( ./libs/Library )
# target_link_libraries( notesy library )

# add_library(ex STATIC IMPORTED)
# set_property(TARGET ex PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/Example/exlib.a)
# target_link_libraries( notesy ex )
