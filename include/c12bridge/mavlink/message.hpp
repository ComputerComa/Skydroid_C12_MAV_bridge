#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <ardupilotmega/mavlink.h>

namespace c12bridge::mavlink {

class StreamParser {
public:
  std::optional<mavlink_message_t> parse_byte(std::uint8_t byte);

private:
  mavlink_message_t receive_buffer_{};
  mavlink_status_t parser_status_{};
};

std::vector<std::uint8_t> encode_heartbeat(std::uint8_t system_id,
                                           std::uint8_t component_id,
                                           MAV_TYPE component_type);

std::vector<std::uint8_t> encode_onboard_computer_status(
    std::uint8_t system_id, std::uint8_t component_id,
    const mavlink_onboard_computer_status_t &status);

std::optional<mavlink_message_t>
parse_message(std::span<const std::uint8_t> bytes);

} // namespace c12bridge::mavlink
