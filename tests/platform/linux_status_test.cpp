#include <cstdint>
#include <iostream>
#include <limits>

#include "c12bridge/platform/linux_status.hpp"

int main() {
  c12bridge::platform::LinuxStatusCollector collector;
  const auto status = collector.collect();

  if (status.time_usec == 0 || status.uptime == 0) {
    std::cerr << "Linux time or uptime was not collected.\n";
    return 1;
  }
  if (status.ram_total == std::numeric_limits<std::uint32_t>::max() ||
      status.ram_usage > status.ram_total) {
    std::cerr << "Linux memory values are invalid.\n";
    return 1;
  }
  if (status.storage_total[0] == std::numeric_limits<std::uint32_t>::max() ||
      status.storage_usage[0] > status.storage_total[0]) {
    std::cerr << "Linux root-filesystem values are invalid.\n";
    return 1;
  }
  if (status.gpu_cores[0] != std::numeric_limits<std::uint8_t>::max()) {
    std::cerr << "Unavailable WSL GPU telemetry was not marked unused.\n";
    return 1;
  }

  return 0;
}
