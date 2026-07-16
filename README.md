# C12 MAVLink Bridge

Native Linux C++ service that exposes a Skydroid C12 payload as MAVLink camera
and gimbal components to ArduPilot. Its first deliverable is a Raspberry Pi
onboard-computer component connected directly to the flight controller.

## Planned interfaces

- Pixhawk TELEM UART: MAVLink 2
- C12 Ethernet: UDP control and telemetry
- C12 RTSP: visible and thermal video
- Raspberry Pi 4: Linux ARM64 deployment target

## Status

Early development. Current work focuses on a bidirectional MAVLink connection
between the Raspberry Pi and flight controller, followed by onboard-computer
telemetry. C12 control and video integration will follow when hardware is
available.

See the [project roadmap](ROADMAP.md) for planned goals and release milestones.

## Development

- Host development: Debian WSL2
- Target hardware: Raspberry Pi 4 / Raspberry Pi OS 64-bit
- Build system: CMake + Ninja
- MAVLink: official `c_library_v2` Git submodule

See [Debian development packages](docs/debian-packages.md) for installation,
source preparation, build, and test commands.
