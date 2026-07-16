#include <cstdint>
#include <iostream>
#include <span>

#include "c12bridge/mavlink/message.hpp"

int main() {
  constexpr std::uint8_t system_id = 42;
  constexpr std::uint8_t component_id = MAV_COMP_ID_ONBOARD_COMPUTER;

  const auto encoded_message = c12bridge::mavlink::encode_heartbeat(
      system_id, component_id, MAV_TYPE_ONBOARD_CONTROLLER);

  if (encoded_message.empty()) {
    std::cerr << "Heartbeat encoding produced no bytes.\n";
    return 1;
  }

  const auto truncated_message =
      std::span{encoded_message}.first(encoded_message.size() - 1);

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

  if (decoded_message->sysid != system_id ||
      decoded_message->compid != component_id ||
      heartbeat.type != MAV_TYPE_ONBOARD_CONTROLLER) {
    std::cerr << "Parsed heartbeat fields do not match the input.\n";
    return 1;
  }

  c12bridge::mavlink::StreamParser stream_parser;
  std::optional<mavlink_message_t> streamed_message;
  for (const std::uint8_t byte : encoded_message) {
    streamed_message = stream_parser.parse_byte(byte);
  }

  if (!streamed_message ||
      streamed_message->msgid != MAVLINK_MSG_ID_HEARTBEAT) {
    std::cerr << "Streaming parser did not preserve frame state.\n";
    return 1;
  }

  mavlink_onboard_computer_status_t input_status{};
  input_status.time_usec = 123456789;
  input_status.uptime = 54321;
  input_status.type = 0;
  input_status.cpu_cores[0] = 37;
  input_status.ram_usage = 1024;
  input_status.ram_total = 4096;

  const auto encoded_status =
      c12bridge::mavlink::encode_onboard_computer_status(
          system_id, component_id, input_status);
  if (encoded_status.empty() || encoded_status.front() != MAVLINK_STX) {
    std::cerr << "Onboard-computer status was not encoded as MAVLink 2.\n";
    return 1;
  }
  const auto decoded_status_message =
      c12bridge::mavlink::parse_message(encoded_status);
  if (!decoded_status_message ||
      decoded_status_message->msgid != MAVLINK_MSG_ID_ONBOARD_COMPUTER_STATUS) {
    std::cerr << "Onboard-computer status could not be parsed.\n";
    return 1;
  }

  mavlink_onboard_computer_status_t decoded_status{};
  mavlink_msg_onboard_computer_status_decode(&*decoded_status_message,
                                             &decoded_status);
  if (decoded_status.time_usec != input_status.time_usec ||
      decoded_status.cpu_cores[0] != input_status.cpu_cores[0] ||
      decoded_status.ram_usage != input_status.ram_usage) {
    std::cerr << "Onboard-computer status fields changed in transit.\n";
    return 1;
  }

  return 0;
}
