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

add_library(c12bridge_mavlink
    src/mavlink/message.cpp
)

add_library(c12bridge_platform
    src/platform/linux_status.cpp
)

target_include_directories(c12bridge_platform PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(c12bridge_platform SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/mavlink"
)

target_compile_options(c12bridge_platform PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

target_include_directories(c12bridge_mavlink PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(c12bridge_mavlink SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/mavlink"
)

target_compile_options(c12bridge_mavlink PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

add_executable(c12-bridge
    src/main.cpp
)

target_link_libraries(c12-bridge PRIVATE c12bridge_mavlink c12bridge_platform)

include(CTest)

if(BUILD_TESTING)
    add_executable(mavlink-message-test
        tests/mavlink/message_test.cpp
    )

    target_link_libraries(mavlink-message-test PRIVATE c12bridge_mavlink)

    add_test(
        NAME mavlink-message-round-trip
        COMMAND mavlink-message-test
    )

    add_executable(linux-status-test
        tests/platform/linux_status_test.cpp
    )

    target_link_libraries(linux-status-test PRIVATE c12bridge_platform)

    add_test(
        NAME linux-status-collection
        COMMAND linux-status-test
    )
endif()
