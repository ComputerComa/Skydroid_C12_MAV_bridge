# Skydroid C12 MAVLink Bridge — Agent Instructions

## Project purpose

Build a native Linux C++ bridge that represents a Skydroid C12 EO/thermal
gimbal camera to ArduPilot as standard MAVLink camera and gimbal components.

The Raspberry Pi is the translator between:

- Pixhawk MAVLink over a TELEM UART
- Skydroid C12 proprietary camera and gimbal commands over Ethernet/UDP
- C12 RTSP video streams at a later stage

Represent the C12 using logical `MAV_COMP_ID_CAMERA`/`MAV_TYPE_CAMERA` and
`MAV_COMP_ID_GIMBAL`/`MAV_TYPE_GIMBAL` components. Both use the aircraft's
system ID. An onboard-computer component is optional and outside the initial
scope.

## How to work with the user

The user is learning C++. Treat implementation as guided development, not code
generation.

Before or alongside a change:

1. Explain the problem being solved and why the design fits.
2. Explain unfamiliar C++ syntax and important lines.
3. Make small, testable changes rather than broad rewrites.
4. Give exact build and verification steps.
5. Do not silently add abstractions, frameworks, or dependencies.

Avoid large unexplained code dumps. Do not assume C12 hardware is available.

## Current technical direction

Do not implement real C12 hardware communication until explicitly requested.
Prefer this progression:

1. Refactor the existing MAVLink smoke test into a small module under
   `include/c12bridge/mavlink/` and `src/mavlink/`.
2. Add automated tests with CTest.
3. Build an offline C12 packet encoder/checksum module from documented reference
   packets and test vectors.
4. Add a fake local UDP C12 server.
5. Add Raspberry Pi serial transport to the Pixhawk.
6. Perform real C12 bench control when hardware is available.
7. Implement MAVLink Camera Protocol v2 and Gimbal Protocol v2 behavior.
8. Add RTSP, recording, OpenCV, and geolocation later.

## Build and verify

The expected development toolchain is CMake, Ninja, GCC/G++, and the official
MAVLink C headers in `extern/mavlink`.

```bash
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
./build/debug/c12-bridge
```

If tests have not been added yet, report that clearly instead of treating an
empty CTest run as meaningful verification.

## Repository and safety rules

- Inspect `git status --short` before editing.
- Preserve unrelated user changes; never discard or rewrite them.
- Do not commit, push, or open a pull request unless explicitly requested.
- Never commit secrets, credentials, Wi-Fi details, packet captures, or
  decompiled proprietary source.
- Use official MAVLink C headers for serialization and parsing. Do not hand-build
  MAVLink frames.
- Use named MAVLink constants such as `MAV_COMP_ID_CAMERA`; do not hard-code
  component ID numbers.
- Keep protocol discoveries attributable to public documentation, observed
  behavior, or user-provided legal artifacts.
- Update durable project documentation when a verified protocol fact or major
  architectural decision changes.

## Session startup

At the beginning of project work:

1. Read this file and `docs/AGENT_HANDOFF.md` if it exists.
2. Inspect `git status --short` and the relevant source files.
3. Distinguish verified behavior from hypotheses in protocol notes.
4. State assumptions that materially affect the implementation.

Repository-relative paths and commands are intentional so this file remains
portable across machines and clones.
