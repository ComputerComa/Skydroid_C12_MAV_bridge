# C12 MAVLink Bridge

Native Linux C++ service that exposes a Skydroid C12 payload as MAVLink camera
and gimbal components to ArduPilot.

## Planned interfaces

- Pixhawk TELEM UART: MAVLink 2
- C12 Ethernet: UDP control and telemetry
- C12 RTSP: visible and thermal video
- Raspberry Pi 4: Linux ARM64 deployment target

## Status

Early development. Bench-test C12 UDP commands before connecting the bridge to
a flight controller.

## Development

- Host development: Debian WSL2
- Target hardware: Raspberry Pi 4 / Raspberry Pi OS 64-bit
- Build system: CMake + Ninja
- MAVLink: official `c_library_v2` Git submodule
