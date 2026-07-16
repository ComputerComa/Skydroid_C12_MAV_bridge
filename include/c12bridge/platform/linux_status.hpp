#pragma once

#include <array>
#include <cstdint>

#include <ardupilotmega/mavlink.h>

namespace c12bridge::platform {

class LinuxStatusCollector {
public:
  struct CpuTimes {
    std::uint64_t total{};
    std::uint64_t idle{};
  };

  mavlink_onboard_computer_status_t collect();

private:
  std::array<CpuTimes, 9> previous_cpu_times_{};
  bool have_previous_cpu_times_{};
};

} // namespace c12bridge::platform
