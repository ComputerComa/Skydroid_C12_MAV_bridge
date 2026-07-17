#include "c12bridge/network/udp_mavlink_manager.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <utility>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace c12bridge::network {
namespace {

constexpr std::uint8_t system_id = 1;
constexpr std::uint8_t component_id = MAV_COMP_ID_ONBOARD_COMPUTER;
constexpr auto heartbeat_interval = std::chrono::milliseconds{1000};
constexpr useconds_t loop_sleep_microseconds = 10'000;

} // namespace

UdpMavlinkManager::UdpMavlinkManager(UdpMavlinkConfig config,
                                     MessageHandler message_handler)
    : config_(std::move(config)), message_handler_(std::move(message_handler)) {
}

UdpMavlinkManager::~UdpMavlinkManager() { close_socket(); }

bool UdpMavlinkManager::initialize() {
  if (socket_fd_ != -1) {
    std::cerr << "UDP manager is already initialized.\n";
    return false;
  }

  target_address_ = {};
  target_address_.sin_family = AF_INET;
  target_address_.sin_port = htons(config_.target_port);
  if (inet_pton(AF_INET, config_.target_ip.c_str(),
                &target_address_.sin_addr) != 1) {
    std::cerr << "Invalid MAVLink target IPv4 address: " << config_.target_ip
              << '\n';
    return false;
  }

  socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd_ == -1) {
    std::cerr << "Could not create MAVLink UDP socket: " << std::strerror(errno)
              << '\n';
    return false;
  }

  sockaddr_in local_address{};
  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = htonl(INADDR_ANY);
  local_address.sin_port = htons(config_.target_port);

  if (bind(socket_fd_, reinterpret_cast<const sockaddr *>(&local_address),
           sizeof(local_address)) == -1) {
    if (errno != EADDRINUSE) {
      std::cerr << "Could not bind MAVLink UDP port " << config_.target_port
                << ": " << std::strerror(errno) << '\n';
      close_socket();
      return false;
    }

    std::cerr << "MAVLink UDP port " << config_.target_port
              << " is already in use; falling back to an ephemeral port.\n";
    local_address.sin_port = htons(0);
    if (bind(socket_fd_, reinterpret_cast<const sockaddr *>(&local_address),
             sizeof(local_address)) == -1) {
      std::cerr << "Could not bind an ephemeral MAVLink UDP port: "
                << std::strerror(errno) << '\n';
      close_socket();
      return false;
    }
  }

  socklen_t local_address_size = sizeof(local_address);
  if (getsockname(socket_fd_, reinterpret_cast<sockaddr *>(&local_address),
                  &local_address_size) == -1) {
    std::cerr << "Could not determine the local MAVLink UDP port: "
              << std::strerror(errno) << '\n';
    close_socket();
    return false;
  }
  local_port_ = ntohs(local_address.sin_port);

  const int current_flags = fcntl(socket_fd_, F_GETFL, 0);
  if (current_flags == -1 ||
      fcntl(socket_fd_, F_SETFL, current_flags | O_NONBLOCK) == -1) {
    std::cerr << "Could not set the MAVLink UDP socket to non-blocking mode: "
              << std::strerror(errno) << '\n';
    close_socket();
    return false;
  }

  next_heartbeat_ = std::chrono::steady_clock::now();
  std::cout << "MAVLink UDP local port: " << local_port_ << '\n'
            << "MAVLink UDP target: " << config_.target_ip << ':'
            << config_.target_port << '\n';
  return true;
}

bool UdpMavlinkManager::send_frame(const std::span<const std::uint8_t> frame) {
  if (socket_fd_ == -1) {
    std::cerr << "Cannot send through an uninitialized UDP manager.\n";
    return false;
  }

  const ssize_t sent_size =
      sendto(socket_fd_, frame.data(), frame.size(), 0,
             reinterpret_cast<const sockaddr *>(&target_address_),
             sizeof(target_address_));
  if (sent_size == -1) {
    std::cerr << "MAVLink UDP send to " << config_.target_ip << ':'
              << config_.target_port << " failed: " << std::strerror(errno)
              << '\n';
    return false;
  }

  if (sent_size != static_cast<ssize_t>(frame.size())) {
    std::cerr << "Incomplete MAVLink UDP send: wrote " << sent_size << " of "
              << frame.size() << " bytes.\n";
    return false;
  }

  return true;
}

void UdpMavlinkManager::run(CycleHandler cycle_handler) {
  if (socket_fd_ == -1) {
    std::cerr << "Cannot run an uninitialized UDP manager.\n";
    return;
  }

  while (true) {
    run_once();
    if (cycle_handler) {
      cycle_handler(std::chrono::steady_clock::now());
    }
    usleep(loop_sleep_microseconds);
  }
}

void UdpMavlinkManager::run_once() {
  if (socket_fd_ == -1) {
    std::cerr << "Cannot run an uninitialized UDP manager.\n";
    return;
  }

  transmit_heartbeat_if_due(std::chrono::steady_clock::now());
  receive_available();
}

std::uint16_t UdpMavlinkManager::local_port() const noexcept {
  return local_port_;
}

void UdpMavlinkManager::transmit_heartbeat_if_due(
    const std::chrono::steady_clock::time_point now) {
  if (now < next_heartbeat_) {
    return;
  }
  next_heartbeat_ = now + heartbeat_interval;

  mavlink_message_t heartbeat_message{};
  mavlink_msg_heartbeat_pack(system_id, component_id, &heartbeat_message,
                             MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID,
                             0, 0, MAV_STATE_ACTIVE);

  std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> send_buffer{};
  const std::uint16_t frame_size =
      mavlink_msg_to_send_buffer(send_buffer.data(), &heartbeat_message);
  if (!send_frame(std::span{send_buffer}.first(frame_size))) {
    return;
  }

  std::cout << "Sent MAVLink heartbeat: system="
            << static_cast<unsigned int>(system_id)
            << " component=" << static_cast<unsigned int>(component_id)
            << " bytes=" << frame_size << '\n';
}

void UdpMavlinkManager::receive_available() {
  std::array<std::uint8_t, 2048> receive_buffer{};

  while (true) {
    sockaddr_in sender_address{};
    socklen_t sender_address_size = sizeof(sender_address);
    const ssize_t received_size = recvfrom(
        socket_fd_, receive_buffer.data(), receive_buffer.size(), 0,
        reinterpret_cast<sockaddr *>(&sender_address), &sender_address_size);

    if (received_size == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "MAVLink UDP receive failed: " << std::strerror(errno)
                << '\n';
      return;
    }

    mavlink_message_t parsed_message{};
    mavlink_status_t parser_status{};
    for (ssize_t index = 0; index < received_size; ++index) {
      const auto byte = receive_buffer[static_cast<std::size_t>(index)];
      if (mavlink_parse_char(MAVLINK_COMM_0, byte, &parsed_message,
                             &parser_status) != 0 &&
          message_handler_) {
        message_handler_(parsed_message);
      }
    }
  }
}

void UdpMavlinkManager::close_socket() noexcept {
  if (socket_fd_ != -1) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
  local_port_ = 0;
}

} // namespace c12bridge::network
