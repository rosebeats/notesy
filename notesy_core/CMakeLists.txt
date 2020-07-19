if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build)
    set(TARGET_DIR "debug")
else()
    set(CARGO_CMD cargo build --release)
    set(TARGET_DIR "release")
endif()

set(LIBFILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libnotesy_core.so")

add_custom_target(notesy_core ALL
    COMMENT "Compiling notesy_core"
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD}
    COMMAND cp ${LIBFILE} ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(notesy_core PROPERTIES LOCATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME notesy_core_test
    COMMAND cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
