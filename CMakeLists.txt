cmake_minimum_required(VERSION 3.16)
project(ddbot)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SRC
        src/*.cc
        src/*.h
        )

add_library(dpp SHARED IMPORTED)
set_target_properties(dpp PROPERTIES
        IMPORTED_LOCATION "/usr/local/lib/libdpp.so"
        INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include/dpp/"
        )

add_executable(ddbot ${SRC})
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG> -Wall -Wextra -Wundef")
target_link_libraries(ddbot dpp)