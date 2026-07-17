#include <array>
#include <cstdint>
#include <iostream>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ardupilotmega/mavlink.h>

#include "c12bridge/mavlink/message.hpp"
#include "c12bridge/network/udp_mavlink_manager.hpp"

int main() {
  const int receiver_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (receiver_fd == -1) {
    std::cerr << "Could not create test receiver socket.\n";
    return 1;
  }

  sockaddr_in receiver_address{};
  receiver_address.sin_family = AF_INET;
  receiver_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  receiver_address.sin_port = htons(0);
  if (bind(receiver_fd, reinterpret_cast<const sockaddr *>(&receiver_address),
           sizeof(receiver_address)) == -1) {
    std::cerr << "Could not bind test receiver socket.\n";
    close(receiver_fd);
    return 1;
  }

  socklen_t receiver_address_size = sizeof(receiver_address);
  if (getsockname(receiver_fd, reinterpret_cast<sockaddr *>(&receiver_address),
                  &receiver_address_size) == -1) {
    std::cerr << "Could not read test receiver address.\n";
    close(receiver_fd);
    return 1;
  }
  const std::uint16_t occupied_port = ntohs(receiver_address.sin_port);

  c12bridge::network::UdpMavlinkConfig config{
      .target_ip = "127.0.0.1",
      .target_port = occupied_port,
  };
  c12bridge::network::UdpMavlinkManager manager{config};
  if (!manager.initialize()) {
    std::cerr << "UDP manager initialization failed.\n";
    close(receiver_fd);
    return 1;
  }

  if (manager.local_port() == 0 || manager.local_port() == occupied_port) {
    std::cerr << "UDP manager did not fall back to an ephemeral port.\n";
    close(receiver_fd);
    return 1;
  }

  manager.run_once();

  pollfd receiver_event{receiver_fd, POLLIN, 0};
  if (poll(&receiver_event, 1, 1000) != 1) {
    std::cerr << "Timed out waiting for the unconditional heartbeat.\n";
    close(receiver_fd);
    return 1;
  }

  std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> receive_buffer{};
  const ssize_t received_size =
      recv(receiver_fd, receive_buffer.data(), receive_buffer.size(), 0);
  if (received_size <= 0) {
    std::cerr << "Could not receive the unconditional heartbeat.\n";
    return 1;
  }

  mavlink_message_t message{};
  mavlink_status_t parser_status{};
  bool parsed = false;
  for (ssize_t index = 0; index < received_size; ++index) {
    if (mavlink_parse_char(MAVLINK_COMM_1,
                           receive_buffer[static_cast<std::size_t>(index)],
                           &message, &parser_status) != 0) {
      parsed = true;
    }
  }

  if (!parsed || message.msgid != MAVLINK_MSG_ID_HEARTBEAT ||
      message.sysid != 1 || message.compid != MAV_COMP_ID_ONBOARD_COMPUTER) {
    std::cerr << "Received MAVLink frame has the wrong identity.\n";
    return 1;
  }

  mavlink_heartbeat_t heartbeat{};
  mavlink_msg_heartbeat_decode(&message, &heartbeat);
  if (heartbeat.type != MAV_TYPE_ONBOARD_CONTROLLER) {
    std::cerr << "Received heartbeat has the wrong component type.\n";
    return 1;
  }

  mavlink_onboard_computer_status_t status{};
  status.time_usec = 123456789;
  status.uptime = 42;
  const auto status_frame = c12bridge::mavlink::encode_onboard_computer_status(
      1, MAV_COMP_ID_ONBOARD_COMPUTER, status);
  if (!manager.send_frame(status_frame)) {
    std::cerr << "Generic MAVLink frame send failed.\n";
    return 1;
  }

  receiver_event.revents = 0;
  if (poll(&receiver_event, 1, 1000) != 1) {
    std::cerr << "Timed out waiting for onboard-computer status.\n";
    return 1;
  }

  const ssize_t status_size =
      recv(receiver_fd, receive_buffer.data(), receive_buffer.size(), 0);
  if (status_size <= 0) {
    std::cerr << "Could not receive onboard-computer status.\n";
    return 1;
  }

  const auto status_message = c12bridge::mavlink::parse_message(
      std::span{receive_buffer}.first(static_cast<std::size_t>(status_size)));
  if (!status_message ||
      status_message->msgid != MAVLINK_MSG_ID_ONBOARD_COMPUTER_STATUS) {
    std::cerr << "Generic send delivered the wrong MAVLink message.\n";
    return 1;
  }

  close(receiver_fd);
  return 0;
}
