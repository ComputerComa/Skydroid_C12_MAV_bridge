#include "c12bridge/platform/linux_status.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <sys/statvfs.h>

namespace c12bridge::platform {
namespace {

constexpr std::uint32_t unused_u32 = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint8_t unused_u8 = std::numeric_limits<std::uint8_t>::max();
constexpr std::int8_t unused_i8 = std::numeric_limits<std::int8_t>::max();
constexpr std::int16_t unused_i16 = std::numeric_limits<std::int16_t>::max();
constexpr std::uint64_t bytes_per_mib = 1024ULL * 1024ULL;

std::uint32_t to_u32(const std::uint64_t value) {
  return static_cast<std::uint32_t>(std::min<std::uint64_t>(
      value, std::numeric_limits<std::uint32_t>::max()));
}

mavlink_onboard_computer_status_t make_unknown_status() {
  mavlink_onboard_computer_status_t status{};
  status.type = 0; // Primary mission computer.
  status.ram_usage = unused_u32;
  status.ram_total = unused_u32;
  std::fill_n(status.storage_type, 4, unused_u32);
  std::fill_n(status.storage_usage, 4, unused_u32);
  std::fill_n(status.storage_total, 4, unused_u32);
  std::fill_n(status.link_type, 6, unused_u32);
  std::fill_n(status.link_tx_rate, 6, unused_u32);
  std::fill_n(status.link_rx_rate, 6, unused_u32);
  std::fill_n(status.link_tx_max, 6, unused_u32);
  std::fill_n(status.link_rx_max, 6, unused_u32);
  std::fill_n(status.fan_speed, 4, unused_i16);
  std::fill_n(status.cpu_cores, 8, unused_u8);
  std::fill_n(status.cpu_combined, 10, unused_u8);
  std::fill_n(status.gpu_cores, 4, unused_u8);
  std::fill_n(status.gpu_combined, 10, unused_u8);
  status.temperature_board = unused_i8;
  std::fill_n(status.temperature_core, 8, unused_i8);
  return status;
}

std::vector<LinuxStatusCollector::CpuTimes> read_cpu_times() {
  std::ifstream input{"/proc/stat"};
  std::vector<LinuxStatusCollector::CpuTimes> result;
  std::string line;

  while (result.size() < 9 && std::getline(input, line)) {
    std::istringstream fields{line};
    std::string name;
    fields >> name;
    if (name != "cpu" && name.rfind("cpu", 0) != 0) {
      break;
    }

    std::vector<std::uint64_t> values;
    std::uint64_t value = 0;
    while (fields >> value) {
      values.push_back(value);
    }
    if (values.size() < 4) {
      continue;
    }

    std::uint64_t total = 0;
    for (const std::uint64_t field : values) {
      total += field;
    }
    const std::uint64_t idle = values[3] + (values.size() > 4 ? values[4] : 0);
    result.push_back({total, idle});
  }

  return result;
}

void collect_memory(mavlink_onboard_computer_status_t &status) {
  std::ifstream input{"/proc/meminfo"};
  std::string key;
  std::uint64_t value_kib = 0;
  std::string unit;
  std::uint64_t total_kib = 0;
  std::uint64_t available_kib = 0;

  while (input >> key >> value_kib >> unit) {
    if (key == "MemTotal:") {
      total_kib = value_kib;
    } else if (key == "MemAvailable:") {
      available_kib = value_kib;
    }
  }

  if (total_kib > 0 && available_kib <= total_kib) {
    status.ram_total = to_u32(total_kib / 1024);
    status.ram_usage = to_u32((total_kib - available_kib) / 1024);
  }
}

void collect_storage(mavlink_onboard_computer_status_t &status) {
  struct statvfs filesystem {};
  if (statvfs("/", &filesystem) != 0 || filesystem.f_blocks == 0) {
    return;
  }

  const std::uint64_t total_bytes =
      static_cast<std::uint64_t>(filesystem.f_blocks) * filesystem.f_frsize;
  const std::uint64_t free_bytes =
      static_cast<std::uint64_t>(filesystem.f_bfree) * filesystem.f_frsize;
  const std::uint64_t used_bytes = total_bytes - free_bytes;

  // WSL exposes capacity but not the physical medium behind its virtual disk.
  status.storage_usage[0] = to_u32(used_bytes / bytes_per_mib);
  status.storage_total[0] = to_u32(total_bytes / bytes_per_mib);
  if (used_bytes * 100 >= total_bytes * 95) {
    status.status_flags |= COMPUTER_STATUS_FLAGS_DISK_FULL;
  }
}

} // namespace

mavlink_onboard_computer_status_t LinuxStatusCollector::collect() {
  auto status = make_unknown_status();

  const auto now = std::chrono::system_clock::now().time_since_epoch();
  status.time_usec = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(now).count());

  std::ifstream uptime_input{"/proc/uptime"};
  double uptime_seconds = 0.0;
  if (uptime_input >> uptime_seconds) {
    status.uptime = to_u32(static_cast<std::uint64_t>(uptime_seconds * 1000.0));
  }

  const auto cpu_times = read_cpu_times();
  if (have_previous_cpu_times_) {
    const std::size_t count = std::min(cpu_times.size(), previous_cpu_times_.size());
    for (std::size_t index = 1; index < count; ++index) {
      const auto total_delta =
          cpu_times[index].total - previous_cpu_times_[index].total;
      const auto idle_delta =
          cpu_times[index].idle - previous_cpu_times_[index].idle;
      if (total_delta > 0 && idle_delta <= total_delta) {
        const auto usage = ((total_delta - idle_delta) * 100 + total_delta / 2) /
                           total_delta;
        status.cpu_cores[index - 1] = static_cast<std::uint8_t>(usage);
      }
    }
  }
  std::copy_n(cpu_times.begin(),
              std::min(cpu_times.size(), previous_cpu_times_.size()),
              previous_cpu_times_.begin());
  have_previous_cpu_times_ = !cpu_times.empty();

  collect_memory(status);
  collect_storage(status);
  return status;
}

} // namespace c12bridge::platform
