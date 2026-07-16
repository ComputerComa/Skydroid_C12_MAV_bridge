# Project Roadmap

This roadmap tracks the major goals for the Skydroid C12 MAVLink Bridge. It is
directional: milestone scope may change as the C12 protocol is verified and the
bridge is tested with real hardware.

For current work and detailed discussion, use
[GitHub issues](../../issues) and [GitHub milestones](../../milestones).

## Status legend

- [x] Complete
- [ ] Planned
- 🚧 In progress
- 🛑 Blocked by hardware or protocol verification

## Major goals

1. Build a reliable, testable C++ foundation for MAVLink and C12 messaging.
2. Translate between standard MAVLink camera/gimbal protocols and C12 UDP
   commands.
3. Run the bridge safely and reliably on a Raspberry Pi connected to ArduPilot.
4. Add video and search-and-rescue features after core control is proven.

## Milestone 0: Project foundation

**Target:** `v0.1.0`  
**Outcome:** A reproducible development build with automated tests for the
initial MAVLink module.

- [x] Configure CMake, C++20, compiler warnings, and Ninja builds.
- [x] Add the official MAVLink C headers.
- [x] Encode and parse a MAVLink heartbeat in an offline smoke test.
- [ ] Move MAVLink behavior into `include/c12bridge/mavlink/` and
  `src/mavlink/`.
- [ ] Add unit tests and register them with CTest.
- [ ] Document local build, test, and contribution workflows.

## Milestone 1: Offline C12 protocol support

**Target:** `v0.2.0`  
**Outcome:** C12 packets can be encoded and validated without camera hardware.

- [ ] Record packet facts separately from unverified protocol hypotheses.
- [ ] Implement packet encoding from documented reference packets.
- [ ] Implement the verified checksum algorithm.
- [ ] Add test vectors for known camera and gimbal commands.
- [ ] Reject invalid sizes, fields, and checksums with useful errors.
- [ ] Document each supported command and the source of its protocol evidence.

## Milestone 2: Simulated C12 integration

**Target:** `v0.3.0`  
**Outcome:** The bridge can exchange commands and responses with a local fake
C12 endpoint.

- [ ] Add a UDP transport module with configurable addresses and ports.
- [ ] Build a fake local UDP C12 server for repeatable development.
- [ ] Test command transmission, response parsing, timeouts, and retries.
- [ ] Add integration tests that require no physical hardware.
- [ ] Add structured, readable diagnostic logging.

## Milestone 3: Pixhawk transport and MAVLink components

**Target:** `v0.4.0`  
**Outcome:** The Raspberry Pi communicates with a Pixhawk and advertises the C12
as standard MAVLink camera and gimbal components.

- [ ] Add configurable Linux serial transport for a Pixhawk TELEM port.
- [ ] Use the aircraft system ID for both logical components.
- [ ] Advertise `MAV_COMP_ID_CAMERA` with `MAV_TYPE_CAMERA`.
- [ ] Advertise `MAV_COMP_ID_GIMBAL` with `MAV_TYPE_GIMBAL`.
- [ ] Send component heartbeats and handle MAVLink routing correctly.
- [ ] Add clean startup, shutdown, reconnect, and error behavior.
- [ ] Document Raspberry Pi serial setup and bench wiring.

## Milestone 4: Camera Protocol v2

**Target:** `v0.5.0`  
**Outcome:** ArduPilot and compatible ground stations can discover and control
the supported C12 camera functions through MAVLink.

- [ ] Publish camera information and capabilities.
- [ ] Implement command handling and `COMMAND_ACK` responses.
- [ ] Translate supported capture, recording, zoom, and mode commands.
- [ ] Report camera settings, capture status, and storage status where the C12
  exposes verified data.
- [ ] Test supported behavior with ArduPilot and a ground station.
- [ ] Clearly report unsupported camera capabilities.

## Milestone 5: Gimbal Protocol v2

**Target:** `v0.6.0`  
**Outcome:** ArduPilot can discover, command, and monitor the C12 as a MAVLink
Gimbal Device.

- [ ] Publish `GIMBAL_DEVICE_INFORMATION`.
- [ ] Handle `GIMBAL_DEVICE_SET_ATTITUDE`.
- [ ] Publish `GIMBAL_DEVICE_ATTITUDE_STATUS`.
- [ ] Translate supported pitch/yaw commands to verified C12 packets.
- [ ] Enforce known motion limits and safe timeout behavior.
- [ ] Test manual and mission-driven control with ArduPilot as Gimbal Manager.

## Milestone 6: Hardware validation and first stable release

**Target:** `v1.0.0`  
**Outcome:** Core camera and gimbal control is documented and repeatable on the
target Raspberry Pi, Pixhawk, and C12 hardware.

- [ ] Bench-test C12 commands before connecting flight hardware.
- [ ] Validate end-to-end camera and gimbal control on a Raspberry Pi 4.
- [ ] Test disconnects, malformed traffic, restarts, and degraded links.
- [ ] Add installation, configuration, service, and troubleshooting guides.
- [ ] Define a supported command/capability matrix.
- [ ] Complete a release checklist and publish the first stable release.

## Future milestones

These goals are intentionally outside the initial control-bridge scope and do
not have release targets yet.

- [ ] Expose and monitor visible and thermal RTSP streams.
- [ ] Add recording and media management.
- [ ] Add an optional MAVLink camera-definition file.
- [ ] Evaluate MAVLink message signing for operational deployments.
- [ ] Correlate video, vehicle position, time, and gimbal attitude.
- [ ] Explore OpenCV detection, thermal hotspot geolocation, and
  search-and-rescue workflows.
- [ ] Evaluate ADS-B display integration separately from the bridge core.

## Scope boundaries

- Real C12 control work begins only after commands are supported by public
  documentation, observed behavior, or user-provided legal artifacts.
- The generated official MAVLink C headers remain the source of truth for
  MAVLink serialization and parsing.
- RTSP, OpenCV, geolocation, and ADS-B features must not delay the core camera
  and gimbal bridge.
- Dates are intentionally omitted until protocol access and hardware
  availability make estimates meaningful.

## Contributing to the roadmap

Open an issue before proposing a major change in scope. Link implementation
issues to the matching GitHub milestone and update this file when a milestone's
outcome or release target changes. Completing a checkbox should mean the work is
implemented, tested, and documented at the level appropriate for that phase.
