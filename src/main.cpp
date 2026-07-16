#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "c12bridge/mavlink/message.hpp"
#include "c12bridge/platform/linux_status.hpp"

namespace {

constexpr std::uint16_t default_udp_port = 14551;

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

} // namespace

int main(const int argc, const char *const argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [UDP_PORT]\n";
    return 1;
  }

  const std::uint16_t port = argc == 2 ? parse_port(argv[1]) : default_udp_port;
  if (port == 0) {
    std::cerr << "UDP port must be a number from 1 to 65535.\n";
    return 1;
  }

  const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd == -1) {
    std::cerr << "Could not create UDP socket: " << std::strerror(errno)
              << '\n';
    return 1;
  }

  sockaddr_in local_address{};
  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = htonl(INADDR_ANY);
  local_address.sin_port = htons(port);

  if (bind(socket_fd, reinterpret_cast<const sockaddr *>(&local_address),
           sizeof(local_address)) == -1) {
    std::cerr << "Could not bind UDP port " << port << ": "
              << std::strerror(errno) << '\n';
    close(socket_fd);
    return 1;
  }

  std::cout << "Listening for MAVLink UDP traffic on 0.0.0.0:" << port
            << "...\n";

  c12bridge::mavlink::StreamParser parser;
  c12bridge::platform::LinuxStatusCollector status_collector;
  std::array<std::uint8_t, 2048> datagram{};
  auto last_heartbeat_sent = std::chrono::steady_clock::time_point{};

  while (true) {
    sockaddr_in sender_address{};
    socklen_t sender_address_size = sizeof(sender_address);
    const ssize_t byte_count = recvfrom(
        socket_fd, datagram.data(), datagram.size(), 0,
        reinterpret_cast<sockaddr *>(&sender_address), &sender_address_size);
    if (byte_count == -1) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "UDP receive failed: " << std::strerror(errno) << '\n';
      close(socket_fd);
      return 1;
    }

    for (ssize_t index = 0; index < byte_count; ++index) {
      const auto message =
          parser.parse_byte(datagram[static_cast<std::size_t>(index)]);
      if (!message || message->msgid != MAVLINK_MSG_ID_HEARTBEAT) {
        continue;
      }

      mavlink_heartbeat_t heartbeat{};
      mavlink_msg_heartbeat_decode(&*message, &heartbeat);

      std::cout << "Heartbeat: system="
                << static_cast<unsigned int>(message->sysid)
                << " component=" << static_cast<unsigned int>(message->compid)
                << " vehicle_type=" << static_cast<unsigned int>(heartbeat.type)
                << " autopilot="
                << static_cast<unsigned int>(heartbeat.autopilot) << '\n';

      // A real autopilot heartbeat identifies the aircraft system. Heartbeats
      // from companion components use MAV_AUTOPILOT_INVALID and are ignored.
      if (heartbeat.autopilot == MAV_AUTOPILOT_INVALID) {
        continue;
      }

      const auto now = std::chrono::steady_clock::now();
      if (now - last_heartbeat_sent < std::chrono::seconds{1}) {
        continue;
      }

      const auto response = c12bridge::mavlink::encode_heartbeat(
          message->sysid, MAV_COMP_ID_ONBOARD_COMPUTER,
          MAV_TYPE_ONBOARD_CONTROLLER);
      const ssize_t sent_byte_count = sendto(
          socket_fd, response.data(), response.size(), 0,
          reinterpret_cast<const sockaddr *>(&sender_address),
          sender_address_size);

      if (sent_byte_count != static_cast<ssize_t>(response.size())) {
        std::cerr << "Could not send onboard-computer heartbeat: "
                  << std::strerror(errno) << '\n';
        continue;
      }

      last_heartbeat_sent = now;
      std::cout << "Sent onboard-computer heartbeat: system="
                << static_cast<unsigned int>(message->sysid)
                << " component=" << MAV_COMP_ID_ONBOARD_COMPUTER << '\n';

      const auto status = status_collector.collect();
      const auto status_response =
          c12bridge::mavlink::encode_onboard_computer_status(
              message->sysid, MAV_COMP_ID_ONBOARD_COMPUTER, status);
      const ssize_t status_byte_count = sendto(
          socket_fd, status_response.data(), status_response.size(), 0,
          reinterpret_cast<const sockaddr *>(&sender_address),
          sender_address_size);

      if (status_byte_count != static_cast<ssize_t>(status_response.size())) {
        std::cerr << "Could not send onboard-computer status: "
                  << std::strerror(errno) << '\n';
        continue;
      }

      std::cout << "Sent onboard-computer status: RAM=" << status.ram_usage
                << '/' << status.ram_total << " MiB, storage="
                << status.storage_usage[0] << '/' << status.storage_total[0]
                << " MiB, frame=" << status_response.size()
                << " bytes (MAVLink 2)\n";
    }
  }
}
