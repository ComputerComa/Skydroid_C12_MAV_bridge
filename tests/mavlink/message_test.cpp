#include <cstdint>
#include <iostream>
#include <span>

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

    if (encoded_message.empty()) {
        std::cerr << "Heartbeat encoding produced no bytes.\n";
        return 1;
    }

    const auto truncated_message = std::span{encoded_message}.first(
        encoded_message.size() - 1
    );

    if (c12bridge::mavlink::parse_message(truncated_message)) {
        std::cerr << "A truncated heartbeat was accepted.\n";
        return 1;
    }

    const auto decoded_message =
        c12bridge::mavlink::parse_message(encoded_message);

    if (!decoded_message) {
        std::cerr << "Encoded heartbeat could not be parsed.\n";
        return 1;
    }

    if (decoded_message->msgid != MAVLINK_MSG_ID_HEARTBEAT) {
        std::cerr << "Parsed message is not a heartbeat.\n";
        return 1;
    }

    mavlink_heartbeat_t heartbeat{};
    mavlink_msg_heartbeat_decode(&*decoded_message, &heartbeat);

    if (decoded_message->sysid != system_id
        || decoded_message->compid != component_id
        || heartbeat.type != MAV_TYPE_ONBOARD_CONTROLLER) {
        std::cerr << "Parsed heartbeat fields do not match the input.\n";
        return 1;
    }

    return 0;
}
