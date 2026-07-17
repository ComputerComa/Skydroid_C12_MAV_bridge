# C12 MAVLink Bridge

Native Linux C++ service that exposes a Skydroid C12 payload as MAVLink camera
and gimbal components to ArduPilot. Its first deliverable is a Raspberry Pi
onboard-computer component connected directly to the flight controller.

MAVLink is the command, control, status, and metadata plane only. Video never
travels inside MAVLink messages. The C12 visible and thermal streams remain
RTSP traffic on Ethernet, are decoded separately on the Raspberry Pi, and are
routed to the video transmitter over HDMI as shown in the
[proposed routing diagram](docs/proposed_path.png) (source:
[proposed_pathing.mmd](docs/proposed_pathing.mmd)).

## Planned interfaces

- Pixhawk TELEM UART: MAVLink 2
- C12 Ethernet: UDP control and telemetry
- C12 media: independent visible and thermal RTSP streams; no video over MAVLink
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
- MAVLink: official full source repository with locally generated C headers

See [Debian development packages](docs/debian-packages.md) for installation,
source preparation, build, and test commands.

## Run the UDP bridge

The bridge sends to and receives from one MAVLink UDP endpoint. The destination
defaults to localhost and the endpoint port defaults to `14551`:

```bash
./build/debug/c12-bridge [DESTINATION_IP] [UDP_PORT]
```

For example, to send to a MAVLink router at `192.168.1.20`:

```bash
./build/debug/c12-bridge 192.168.1.20 14551
```

The bridge sends its onboard-computer heartbeat and measured Linux
`ONBOARD_COMPUTER_STATUS` once per second even when it has not received
flight-controller traffic. Both use MAVLink system ID `1` and
`MAV_COMP_ID_ONBOARD_COMPUTER` (component ID 191). Incoming autopilot heartbeats
are reported without gating transmission.

The UDP manager first tries to bind the configured target port locally. If that
port is already occupied, it logs the collision and asks Linux for a free
ephemeral port. Both cases use a non-blocking socket.
