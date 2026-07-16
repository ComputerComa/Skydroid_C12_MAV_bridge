#include <array>
#include <cstdint>
#include <iostream>

#include <ardupilotmega/mavlink.h>

int main()
{
    constexpr std::uint8_t system_id = 42;
    constexpr std::uint8_t component_id = MAV_COMP_ID_ONBOARD_COMPUTER;

    mavlink_message_t outgoing_message{};

    mavlink_msg_heartbeat_pack(
        system_id,
        component_id,
        &outgoing_message,
        MAV_TYPE_ONBOARD_CONTROLLER,
        MAV_AUTOPILOT_INVALID,
        0,
        0,
        MAV_STATE_ACTIVE
    );

    std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> buffer{};

    const std::uint16_t byte_count =
        mavlink_msg_to_send_buffer(buffer.data(), &outgoing_message);

    std::cout << "Encoded MAVLink heartbeat: "
              << byte_count
              << " bytes\n";

    mavlink_message_t incoming_message{};
    mavlink_status_t parser_status{};

    bool parsed_message = false;

    for (std::uint16_t index = 0; index < byte_count; ++index) {
        if (mavlink_parse_char(
                MAVLINK_COMM_0,
                buffer[index],
                &incoming_message,
                &parser_status)) {
            parsed_message = true;
        }
    }

    if (!parsed_message) {
        std::cerr << "Failed to parse the generated MAVLink message.\n";
        return 1;
    }

    std::cout << "Decoded message ID: "
              << incoming_message.msgid
              << '\n';

    if (incoming_message.msgid != MAVLINK_MSG_ID_HEARTBEAT) {
        std::cerr << "Decoded the wrong MAVLink message type.\n";
        return 1;
    }

    std::cout << "MAVLink C bindings are working correctly.\n";
    return 0;
}
