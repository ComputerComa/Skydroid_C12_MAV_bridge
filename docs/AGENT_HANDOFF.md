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
- C12 RTSP streams will be handled later

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
- Official MAVLink `c_library_v2` headers under `extern/mavlink`

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
Raspberry Pi and flight controller over a TELEM UART. The Pi should receive the
vehicle heartbeat, use the aircraft system ID, advertise itself as
`MAV_COMP_ID_ONBOARD_COMPUTER`, and send measured onboard-computer telemetry.

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

## Safety and repository rules

- Inspect `git status` before changing anything.
- Preserve unrelated user changes.
- Do not commit or push unless the user explicitly requests it.
- Do not commit secrets, packet captures, C12 credentials, Wi-Fi credentials, or decompiled proprietary source.
- Use named MAVLink constants such as `MAV_COMP_ID_CAMERA`; do not hard-code component-ID numbers.
- Use the official MAVLink C headers for packet serialization/parsing; do not hand-build MAVLink frames.
