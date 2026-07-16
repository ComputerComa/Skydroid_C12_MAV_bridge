#include "c12bridge/mavlink/message.hpp"

#include <array>

namespace c12bridge::mavlink {

std::optional<mavlink_message_t>
StreamParser::parse_byte(const std::uint8_t byte) {
  mavlink_message_t parsed_message{};
  mavlink_status_t parsed_status{};

  if (mavlink_frame_char_buffer(&receive_buffer_, &parser_status_, byte,
                                &parsed_message,
                                &parsed_status) == MAVLINK_FRAMING_OK) {
    return parsed_message;
  }

  return std::nullopt;
}

std::vector<std::uint8_t> encode_heartbeat(const std::uint8_t system_id,
                                           const std::uint8_t component_id,
                                           const MAV_TYPE component_type) {
  mavlink_message_t message{};

  mavlink_msg_heartbeat_pack(system_id, component_id, &message, component_type,
                             MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);

  std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> buffer{};
  const std::uint16_t byte_count =
      mavlink_msg_to_send_buffer(buffer.data(), &message);

  return {buffer.begin(), buffer.begin() + byte_count};
}

std::vector<std::uint8_t> encode_onboard_computer_status(
    const std::uint8_t system_id, const std::uint8_t component_id,
    const mavlink_onboard_computer_status_t &status) {
  mavlink_message_t message{};
  mavlink_msg_onboard_computer_status_encode(system_id, component_id, &message,
                                             &status);

  std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> buffer{};
  const std::uint16_t byte_count =
      mavlink_msg_to_send_buffer(buffer.data(), &message);
  return {buffer.begin(), buffer.begin() + byte_count};
}

std::optional<mavlink_message_t>
parse_message(const std::span<const std::uint8_t> bytes) {
  StreamParser parser;

  for (const std::uint8_t byte : bytes) {
    if (const auto message = parser.parse_byte(byte)) {
      return message;
    }
  }

  return std::nullopt;
}

} // namespace c12bridge::mavlink
