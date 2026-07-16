#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <ardupilotmega/mavlink.h>

namespace c12bridge::mavlink {

std::vector<std::uint8_t> encode_heartbeat(
    std::uint8_t system_id,
    std::uint8_t component_id,
    MAV_TYPE component_type
);

std::optional<mavlink_message_t> parse_message(
    std::span<const std::uint8_t> bytes
);

} // namespace c12bridge::mavlink
