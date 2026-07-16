cmake_minimum_required(VERSION 3.25)

project(c12_mavlink_bridge
    VERSION 0.1.0
    DESCRIPTION "Skydroid C12 to MAVLink camera and gimbal bridge"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(c12-bridge
    src/main.cpp
)

target_include_directories(c12-bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(c12-bridge SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/mavlink"
)

target_compile_options(c12-bridge PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)
