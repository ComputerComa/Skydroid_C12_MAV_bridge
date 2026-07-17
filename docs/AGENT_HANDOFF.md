# Skydroid C12 MAVLink Bridge — Agent Handoff

Use this document as context when beginning a new agent session for the project.

## Project goal

Build a native Linux C++ bridge that makes a Skydroid C12 EO/thermal gimbal camera appear to ArduPilot as standard MAVLink camera and gimbal components.

```text
Pixhawk 6C
  <-> MAVLink over TELEM UART
Raspberry Pi 4 Model B, 8 GB
  <-> Ethernet / UDP / RTSP
Skydroid C12
```

The Raspberry Pi will translate:

- MAVLink Camera Protocol v2 <-> C12 proprietary UDP camera commands
- MAVLink Gimbal Protocol v2 <-> C12 proprietary UDP gimbal commands
- C12 visible and thermal RTSP streams will be handled later on an independent
  media path

**Video does not travel through MAVLink.** MAVLink carries payload commands,
status, and stream metadata only. The actual C12 video remains RTSP over
Ethernet, is decoded independently by GStreamer/FFmpeg on the Raspberry Pi, and
is sent over HDMI to the 5.5 GHz video transmitter. The authoritative proposed
routing is illustrated in [`proposed_path.png`](proposed_path.png), generated
from [`proposed_pathing.mmd`](proposed_pathing.mmd).

The Pi is the host/translator. The C12 is represented as logical MAVLink components:

```text
MAV_COMP_ID_CAMERA + MAV_TYPE_CAMERA
MAV_COMP_ID_GIMBAL + MAV_TYPE_GIMBAL
```

All three components use the aircraft/Pixhawk system ID. The
`MAV_COMP_ID_ONBOARD_COMPUTER` component represents the Linux bridge itself.
Start development with its heartbeat, then add the logical camera and gimbal
components alongside it.

## Current repository and environment

Repository location in Debian WSL:

```text
~/coding/Skydroid_C12_MAV_bridge
```

Toolchain already installed and working:

- Debian WSL2
- GCC/G++ 14.2
- CMake 3.31
- Ninja 1.12
- Git
- VS Code Remote WSL
- Official full MAVLink repository under `extern/mavlink`, including its pinned
  `pymavlink` generator submodule

Current live development is on Debian 13 under WSL2. A Pixhawk 6C connected to
Windows through USB is bridged by MAVProxy to the WSL process over UDP port
14551. This development route has verified bidirectional heartbeats: the bridge
discovers aircraft system ID 1 and advertises component
`MAV_COMP_ID_ONBOARD_COMPUTER` on that system. The bridge also sends measured
Linux `ONBOARD_COMPUTER_STATUS` values for uptime, per-core CPU load, RAM, and
root-filesystem usage. Raspberry Pi and TELEM-UART behavior remain unverified.

The UDP executable accepts `[TARGET_IP] [TARGET_PORT]`, defaulting to
`127.0.0.1 14551`. Its non-blocking UDP manager first binds the target port
locally and falls back to a logged Linux-assigned ephemeral port only when the
target port is already in use. The manager transmits a system ID 1, component
ID 191 onboard-computer heartbeat at 1 Hz independently of received traffic.
Application code uses the manager's generic frame sender to publish measured
Linux `ONBOARD_COMPUTER_STATUS` at 1 Hz on the same schedule. Complete incoming
MAVLink messages are delivered to an application callback; the current callback
reports heartbeats whose `autopilot` field is not `MAV_AUTOPILOT_INVALID`.

The project currently builds successfully with:

```bash
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
./build/debug/c12-bridge
```

The current `main.cpp` has a MAVLink smoke test that:

1. Creates a MAVLink `HEARTBEAT`.
2. Serializes it to bytes.
3. Parses those bytes back.
4. Verifies the decoded message ID is `HEARTBEAT`.

It currently prints:

```text
Encoded MAVLink heartbeat: 21 bytes
Decoded message ID: 0
MAVLink C bindings are working correctly.
```

## Important user preference

The user is new to C++ and wants to learn rather than treat the agent as a code generator.

Before or alongside any code change:

1. Explain what problem the code solves.
2. Explain why this design was selected.
3. Explain unfamiliar C++ syntax and key lines.
4. Make small, testable changes.
5. Tell the user how to build and verify the change.
6. Do not silently introduce abstractions, frameworks, or dependencies.

Avoid large code dumps. Do not assume the user owns the C12 yet; they do not.

## Current development direction

The first hardware milestone is a bidirectional MAVLink connection between the
Raspberry Pi and flight controller over a TELEM UART. The Pi advertises itself
as `MAV_COMP_ID_ONBOARD_COMPUTER` and sends measured telemetry whether or not a
flight controller is connected. A valid autopilot heartbeat updates the system
ID but does not start or stop transmission.

Do not implement real C12 communication until this link works reliably. C12
control and video may wait until the user obtains the hardware.

Later milestones:

1. Linux serial transport with pseudo-terminal tests.
2. Flight-controller heartbeat reception and aircraft system-ID discovery.
3. Pi heartbeat and `ONBOARD_COMPUTER_STATUS` transmission.
4. Offline C12 packet encoder/checksum module and tests.
5. Fake local UDP C12 server, followed by real C12 bench control.
6. MAVLink Camera Protocol v2 and Gimbal Protocol v2 implementation.
7. RTSP, recording, and later OpenCV/geolocation.

Item 7 is a separate media pipeline. It must not be implemented as MAVLink
payload transport; MAVLink integration may expose only stream discovery,
control, and status metadata.

## Safety and repository rules

- Inspect `git status` before changing anything.
- Preserve unrelated user changes.
- Do not commit or push unless the user explicitly requests it.
- Do not commit secrets, packet captures, C12 credentials, Wi-Fi credentials, or decompiled proprietary source.
- Use named MAVLink constants such as `MAV_COMP_ID_CAMERA`; do not hard-code component-ID numbers.
- Use the official MAVLink C headers for packet serialization/parsing; do not hand-build MAVLink frames.
