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

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(MAVLINK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/mavlink")
set(MAVLINK_DIALECT
    "${MAVLINK_SOURCE_DIR}/message_definitions/v1.0/ardupilotmega.xml"
)
set(MAVLINK_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/mavlink")
set(MAVLINK_GENERATED_HEADER
    "${MAVLINK_GENERATED_DIR}/ardupilotmega/mavlink.h"
)

file(GLOB_RECURSE MAVLINK_DEFINITION_FILES CONFIGURE_DEPENDS
    "${MAVLINK_SOURCE_DIR}/message_definitions/v1.0/*.xml"
)
file(GLOB_RECURSE MAVLINK_GENERATOR_FILES CONFIGURE_DEPENDS
    "${MAVLINK_SOURCE_DIR}/pymavlink/generator/*.py"
    "${MAVLINK_SOURCE_DIR}/pymavlink/tools/mavgen.py"
)

add_custom_command(
    OUTPUT "${MAVLINK_GENERATED_HEADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${MAVLINK_GENERATED_DIR}"
    COMMAND "${Python3_EXECUTABLE}"
        "${MAVLINK_SOURCE_DIR}/pymavlink/tools/mavgen.py"
        --lang C
        --wire-protocol 2.0
        --output "${MAVLINK_GENERATED_DIR}"
        "${MAVLINK_DIALECT}"
    DEPENDS
        ${MAVLINK_DEFINITION_FILES}
        ${MAVLINK_GENERATOR_FILES}
    COMMENT "Generating MAVLink 2 C headers for ardupilotmega"
    VERBATIM
)

add_custom_target(mavlink-headers DEPENDS "${MAVLINK_GENERATED_HEADER}")

set(MAVLINK_WIRESHARK_DISSECTOR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/wireshark/mavlink.lua"
)
add_custom_command(
    OUTPUT "${MAVLINK_WIRESHARK_DISSECTOR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated/wireshark"
    COMMAND "${Python3_EXECUTABLE}"
        "${MAVLINK_SOURCE_DIR}/pymavlink/tools/mavgen.py"
        --lang WLua
        --wire-protocol 2.0
        --output "${MAVLINK_WIRESHARK_DISSECTOR}"
        "${MAVLINK_DIALECT}"
    DEPENDS
        ${MAVLINK_DEFINITION_FILES}
        ${MAVLINK_GENERATOR_FILES}
    COMMENT "Generating MAVLink 2 Wireshark Lua dissector"
    VERBATIM
)

add_custom_target(mavlink-wireshark DEPENDS "${MAVLINK_WIRESHARK_DISSECTOR}")

add_library(c12bridge_mavlink
    src/mavlink/message.cpp
)

add_library(c12bridge_platform
    src/platform/linux_status.cpp
)

add_library(c12bridge_network
    src/network/udp_mavlink_manager.cpp
)

add_dependencies(c12bridge_mavlink mavlink-headers)
add_dependencies(c12bridge_platform mavlink-headers)
add_dependencies(c12bridge_network mavlink-headers)

target_include_directories(c12bridge_network PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(c12bridge_network SYSTEM PUBLIC
    "${MAVLINK_GENERATED_DIR}"
)

target_compile_options(c12bridge_network PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

target_include_directories(c12bridge_platform PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(c12bridge_platform SYSTEM PUBLIC
    "${MAVLINK_GENERATED_DIR}"
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
    "${MAVLINK_GENERATED_DIR}"
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

target_link_libraries(c12-bridge PRIVATE
    c12bridge_mavlink
    c12bridge_network
    c12bridge_platform
)

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

    add_executable(udp-mavlink-manager-test
        tests/network/udp_mavlink_manager_test.cpp
    )

    target_link_libraries(udp-mavlink-manager-test PRIVATE
        c12bridge_mavlink
        c12bridge_network
    )

    add_test(
        NAME udp-mavlink-manager-fallback
        COMMAND udp-mavlink-manager-test
    )
endif()
