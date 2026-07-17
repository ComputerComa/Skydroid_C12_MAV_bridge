#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <span>
#include <string>

#include <netinet/in.h>

#include <ardupilotmega/mavlink.h>

namespace c12bridge::network {

struct UdpMavlinkConfig {
  std::string target_ip{"127.0.0.1"};
  std::uint16_t target_port{14551};
};

class UdpMavlinkManager {
public:
  using MessageHandler = std::function<void(const mavlink_message_t &)>;
  using CycleHandler =
      std::function<void(std::chrono::steady_clock::time_point)>;

  explicit UdpMavlinkManager(UdpMavlinkConfig config,
                             MessageHandler message_handler = {});
  ~UdpMavlinkManager();

  UdpMavlinkManager(const UdpMavlinkManager &) = delete;
  UdpMavlinkManager &operator=(const UdpMavlinkManager &) = delete;
  UdpMavlinkManager(UdpMavlinkManager &&) = delete;
  UdpMavlinkManager &operator=(UdpMavlinkManager &&) = delete;

  bool initialize();
  bool send_frame(std::span<const std::uint8_t> frame);
  void run_once();
  void run(CycleHandler cycle_handler = {});

  [[nodiscard]] std::uint16_t local_port() const noexcept;

private:
  void transmit_heartbeat_if_due(std::chrono::steady_clock::time_point now);
  void receive_available();
  void close_socket() noexcept;

  UdpMavlinkConfig config_;
  MessageHandler message_handler_;
  int socket_fd_{-1};
  sockaddr_in target_address_{};
  std::uint16_t local_port_{0};
  std::chrono::steady_clock::time_point next_heartbeat_{};
};

} // namespace c12bridge::network
