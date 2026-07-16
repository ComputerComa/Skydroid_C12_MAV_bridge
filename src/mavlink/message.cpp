#include "c12bridge/mavlink/message.hpp"

#include <array>

namespace c12bridge::mavlink {

std::vector<std::uint8_t> encode_heartbeat(
    const std::uint8_t system_id,
    const std::uint8_t component_id,
    const MAV_TYPE component_type
)
{
    mavlink_message_t message{};

    mavlink_msg_heartbeat_pack(
        system_id,
        component_id,
        &message,
        component_type,
        MAV_AUTOPILOT_INVALID,
        0,
        0,
        MAV_STATE_ACTIVE
    );

    std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> buffer{};
    const std::uint16_t byte_count =
        mavlink_msg_to_send_buffer(buffer.data(), &message);

    return {buffer.begin(), buffer.begin() + byte_count};
}

std::optional<mavlink_message_t> parse_message(
    const std::span<const std::uint8_t> bytes
)
{
    mavlink_message_t receive_buffer{};
    mavlink_message_t parsed_message{};
    mavlink_status_t parser_status{};
    mavlink_status_t parsed_status{};

    for (const std::uint8_t byte : bytes) {
        if (mavlink_frame_char_buffer(
                &receive_buffer,
                &parser_status,
                byte,
                &parsed_message,
                &parsed_status) == MAVLINK_FRAMING_OK) {
            return parsed_message;
        }
    } 

    return std::nullopt;
}

} // namespace c12bridge::mavlink
