#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "c12bridge/mavlink/message.hpp"
#include "c12bridge/network/udp_mavlink_manager.hpp"
#include "c12bridge/platform/linux_status.hpp"

namespace {

std::uint16_t parse_port(const std::string_view text) {
  unsigned int port = 0;
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), port);

  if (error != std::errc{} || end != text.data() + text.size() || port == 0 ||
      port > 65535) {
    return 0;
  }

  return static_cast<std::uint16_t>(port);
}

void handle_message(const mavlink_message_t &message) {
  if (message.msgid != MAVLINK_MSG_ID_HEARTBEAT) {
    return;
  }

  mavlink_heartbeat_t heartbeat{};
  mavlink_msg_heartbeat_decode(&message, &heartbeat);
  if (heartbeat.autopilot == MAV_AUTOPILOT_INVALID) {
    return;
  }

  std::cout << "Received flight-controller heartbeat: system="
            << static_cast<unsigned int>(message.sysid)
            << " component=" << static_cast<unsigned int>(message.compid)
            << " vehicle_type=" << static_cast<unsigned int>(heartbeat.type)
            << " autopilot=" << static_cast<unsigned int>(heartbeat.autopilot)
            << '\n';
}

} // namespace

int main(const int argc, const char *const argv[]) {
  if (argc > 3) {
    std::cerr << "Usage: " << argv[0] << " [TARGET_IP] [TARGET_PORT]\n";
    return 1;
  }

  c12bridge::network::UdpMavlinkConfig config;
  if (argc >= 2) {
    config.target_ip = argv[1];
  }
  if (argc >= 3) {
    config.target_port = parse_port(argv[2]);
    if (config.target_port == 0) {
      std::cerr << "Target port must be a number from 1 to 65535.\n";
      return 1;
    }
  }

  c12bridge::network::UdpMavlinkManager manager{config, handle_message};
  if (!manager.initialize()) {
    return 1;
  }

  c12bridge::platform::LinuxStatusCollector status_collector;
  auto next_status_send = std::chrono::steady_clock::now();

  manager.run([&manager, &status_collector, &next_status_send](const auto now) {
    if (now < next_status_send) {
      return;
    }
    next_status_send = now + std::chrono::seconds{1};

    const auto status = status_collector.collect();
    const auto status_frame =
        c12bridge::mavlink::encode_onboard_computer_status(
            1, MAV_COMP_ID_ONBOARD_COMPUTER, status);
    if (manager.send_frame(status_frame)) {
      std::cout << "Sent onboard-computer status: RAM=" << status.ram_usage
                << '/' << status.ram_total
                << " MiB, storage=" << status.storage_usage[0] << '/'
                << status.storage_total[0]
                << " MiB, bytes=" << status_frame.size() << '\n';
    }
  });
  return 0;
}
