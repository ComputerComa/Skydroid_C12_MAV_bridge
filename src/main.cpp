#include <cstdint>
#include <iostream>

#include "c12bridge/mavlink/message.hpp"

int main()
{
    constexpr std::uint8_t system_id = 42;
    constexpr std::uint8_t component_id = MAV_COMP_ID_ONBOARD_COMPUTER;

    const auto encoded_message = c12bridge::mavlink::encode_heartbeat(
        system_id,
        component_id,
        MAV_TYPE_ONBOARD_CONTROLLER
    );

    std::cout << "Encoded MAVLink heartbeat: "
              << encoded_message.size()
              << " bytes\n";

    const auto decoded_message =
        c12bridge::mavlink::parse_message(encoded_message);

    if (!decoded_message) {
        std::cerr << "Failed to parse the generated MAVLink message.\n";
        return 1;
    }

    std::cout << "Decoded message ID: "
              << decoded_message->msgid
              << '\n';

    if (decoded_message->msgid != MAVLINK_MSG_ID_HEARTBEAT) {
        std::cerr << "Decoded the wrong MAVLink message type.\n";
        return 1;
    }

    std::cout << "MAVLink C bindings are working correctly.\n";
    return 0;
}
