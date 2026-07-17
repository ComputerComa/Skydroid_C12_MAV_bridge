# Platform, Mission, and Design Goals

## Purpose

This document records the physical platform, intended operational use, long-term direction, and design constraints for the C12 MAVLink Bridge project. It exists so contributors and coding agents can make implementation decisions that fit the aircraft rather than optimizing only for the current prototype.

The repository should be treated as part of a broader airborne systems platform, not merely as a single protocol translation utility.

## Project vision

The project will expose the Skydroid C12 electro-optical and thermal payload as standards-based MAVLink camera and gimbal components while preserving a separate, IP-native media path for video.

The Raspberry Pi companion computer is expected to become the integration point for:

- MAVLink camera and gimbal services
- C12 command and telemetry translation
- video discovery, relay, and optional processing
- network routing and link supervision
- diagnostics and health reporting
- future payload and perception services

The desired result is an aircraft architecture that is modular, inspectable, testable, and able to evolve without replacing the entire software stack whenever one hardware component changes.

## Aircraft platform

### Airframe

The target aircraft is a large multirotor/hexacopter platform with enough payload and electrical capacity for a flight controller, companion computer, Ethernet infrastructure, multiple radios, and an EO/IR gimbal.

The system should therefore prioritize field reliability, maintainability, thermal management, clean power distribution, and deterministic failure behavior over minimum component count.

### Flight controller

Current flight controller:

- Holybro Pixhawk 6C
- ArduPilot firmware
- MAVLink 2
- initial companion-computer connection over USB or TELEM UART

Planned future migration:

- Cube Orange+
- possible Ethernet-based flight-controller integration through CubeNode ETH or another DroneNet-compatible interface

Software must not assume that the flight controller will always be connected through one specific serial device. Flight-controller transport belongs at the edge of the system and should be replaceable without changing camera, video, or payload business logic.

### Companion computer

Current companion-computer direction:

- Raspberry Pi 4 or Raspberry Pi 5 class hardware
- Raspberry Pi OS Lite 64-bit
- Linux system services managed by systemd
- native C or C++ production software
- host development and testing from Debian under WSL2

The companion computer is a headless onboard network and payload appliance. It should not be designed primarily as a graphical desktop or HDMI display generator.

### Payload

Primary payload:

- Skydroid C12 gimbal camera
- visible-light stream
- thermal stream
- vendor command and telemetry interface
- Ethernet RTSP media transport

The bridge should present standards-compliant MAVLink Camera Protocol v2 and gimbal behavior wherever the C12 supports the underlying function.

Vendor-specific protocol details should remain behind a dedicated adapter boundary.

### Airborne network

The preferred long-term architecture is Ethernet-first.

Expected components include:

- BotBlox DroneNet or comparable managed airborne Ethernet switch
- Raspberry Pi companion computer
- Skydroid C12
- future Ethernet-capable flight-controller interface
- primary IP video/data radio
- LTE modem for independent remote access and backup telemetry

The network design should support static addressing, documented subnets, explicit routing, service discovery only where needed, and packet capture at useful boundaries.

### Ground station

Primary ground-control software:

- Mission Planner on Windows

Secondary development and diagnostic environments may include:

- WSL2 on the same Windows computer
- standalone GStreamer or FFmpeg pipelines
- Wireshark and tcpdump
- QGroundControl for interoperability testing

Mission Planner integration is the operational target, but the aircraft-side software should remain standards-based rather than embedding Mission Planner-specific assumptions into core modules.

## Communications strategy

### Command and metadata plane

MAVLink is used for:

- camera and gimbal commands
- component discovery
- heartbeats and status
- stream metadata
- health reporting
- aircraft and payload telemetry

MAVLink is not a video transport.

### Media plane

Video remains a separate IP media path using RTSP, RTP, UDP, or another explicitly selected media transport.

Camera Protocol v2 should advertise the stream that the ground station can actually reach. It must not imply that video bytes are carried in MAVLink.

### Primary video downlink direction

The preferred design direction is a native IP link, with the SIYI HM30 currently favored for the first fieldable implementation because it behaves as a purpose-built network bridge and reduces monitor-mode Wi-Fi and patched-driver maintenance.

WFB-ng remains a technically strong alternative and test platform, especially where open implementation, link tuning, and low latency are more important than operational simplicity.

The software must keep the video transport replaceable. C12 ingestion, optional processing, stream supervision, and Camera Protocol v2 metadata should not depend directly on HM30- or WFB-ng-specific code.

### Independent backup paths

Expected independent paths include:

- 915 MHz telemetry radio for basic command and telemetry resilience
- LTE plus Tailscale for remote administration and backup MAVLink access

Routine low-latency video should not depend on cellular coverage.

No single routing failure should silently remove all telemetry paths.

## Primary software goals

### Standards compliance

Use official MAVLink definitions and generated headers as the protocol source of truth.

Implement Camera Protocol v2 and gimbal behavior according to current MAVLink and ArduPilot expectations. Do not invent message fields, component IDs, stream flags, or command semantics.

### Modularity

Separate the following concerns:

- flight-controller transport
- MAVLink routing and parsing
- camera component behavior
- gimbal component behavior
- C12 vendor-protocol translation
- media ingestion and relay
- diagnostics and metrics
- configuration and process supervision

A hardware or transport adapter may fail without corrupting unrelated service state.

### Transport independence

Core camera and gimbal logic should operate on decoded commands and internal domain objects rather than serial-port or socket details.

Serial, USB, UDP, Ethernet, and future DroneNet interfaces should be adapters around stable internal APIs.

### Reliability before feature count

The project should establish a direct and measurable path for commands, telemetry, and one video stream before adding overlays, picture-in-picture, object detection, or dual-stream processing.

Every feature should define:

- startup behavior
- timeout behavior
- retry policy
- stale-data behavior
- degraded-mode behavior
- shutdown behavior

### Observability

The aircraft must make failures diagnosable.

At minimum, the system should be able to report or log:

- MAVLink parser errors
- CRC failures where exposed by the parser
- sequence gaps tracked separately per system and component
- received and transmitted packet counts
- socket and serial read/write failures
- reconnect attempts
- C12 command timeouts and negative acknowledgements
- RTSP connection state
- stream bitrate and packet loss where measurable
- frame or keyframe timing
- CPU load
- memory use
- Raspberry Pi temperature and throttling state
- network-interface state and throughput

Logs should identify the failing boundary rather than only reporting a generic camera failure.

### Testability

Development must be possible without the complete aircraft.

Interfaces should support:

- simulated MAVLink endpoints
- recorded MAVLink playback
- synthetic RTP video
- mocked C12 protocol responses
- loopback UDP tests
- WSL2 host testing
- Raspberry Pi integration tests

External interfaces should be mockable without replacing core logic.

### Safe configuration

Runtime configuration should be explicit and reviewable.

Avoid scattering addresses, ports, device paths, stream IDs, and component IDs through source files. Configuration should support clear defaults while failing loudly on invalid or conflicting values.

Stable Linux device names such as `/dev/serial/by-id/...` are preferred over enumeration-dependent names such as `/dev/ttyUSB0`.

## Design constraints

### Headless operation

The aircraft software must boot and operate without a monitor, keyboard, desktop session, or interactive shell.

### Resource limits

The companion computer has finite CPU, memory, USB bandwidth, power, and thermal margin.

Avoid decoding and re-encoding video unless pixel modification is required. Direct depayloading and repacketization should be preferred when codec and packetization compatibility allow it.

Hardware acceleration must be verified on the actual Raspberry Pi model and operating-system image before becoming an architectural dependency.

### Power and EMI

Network radios, USB adapters, the Pi, and the C12 may have high transient current requirements.

Software documentation should not assume that USB power alone is adequate for every peripheral. Hardware integration documents should include power source, voltage, expected current, grounding, connector retention, cooling, and antenna separation.

### Intermittent links

RF, cellular, RTSP, and serial links will disconnect.

A disconnect must not require rebooting the aircraft. Services should reconnect with bounded backoff and expose their degraded state.

### Security

Do not expose unauthenticated management services directly to the public internet.

Use Tailscale or another authenticated private network for remote administration. Bind local-only services to loopback where possible and document every externally reachable port.

### Regulatory and operational limits

RF power, frequencies, antennas, and operating range must be selected and operated within applicable regulations and license conditions.

Software and documentation should distinguish measured performance from vendor-advertised performance.

## Recommended service boundaries

The exact process layout may evolve, but the following logical boundaries should remain clear:

```text
flight-controller adapter
        |
        v
MAVLink router/parser
        |
        +--------------------+
        |                    |
        v                    v
camera/gimbal service   diagnostics service
        |
        v
C12 protocol adapter

C12 RTSP streams
        |
        v
video manager
        |
        v
selected IP downlink
        |
        v
Mission Planner / GStreamer
```

A single executable may initially host several modules, but the source layout and APIs should preserve these boundaries so they can later become separate systemd services if operational experience justifies it.

## Implementation priorities

1. Reliable bidirectional MAVLink transport with raw capture and parser diagnostics.
2. Correct onboard-computer heartbeat and status reporting.
3. Camera component discovery and command acknowledgement.
4. C12 command and telemetry adapter.
5. One direct video stream from C12 to a standalone ground receiver.
6. The same stream displayed in Mission Planner.
7. Camera Protocol v2 stream metadata matching the real ground endpoint.
8. Gimbal integration and complete command coverage.
9. Second visible/thermal stream where bandwidth permits.
10. Optional video processing only after the unmodified path is stable.

## Long-term goals

The architecture should leave room for:

- multiple payloads
- additional EO/IR cameras
- image capture and geotagging
- target tracking and computer vision
- AI-assisted detection
- multiple simultaneous ground consumers
- remote health dashboards
- recorded telemetry and media correlation
- migration from Pixhawk 6C to Cube Orange+
- DroneNet-native flight-controller and payload connectivity
- replacement of any specific video radio without rewriting payload logic

These are future capabilities, not requirements for the first reliable release.

## Decision principles

When tradeoffs are unclear, use these principles in order:

1. Preserve safe flight and independent telemetry paths.
2. Prefer reliable, observable behavior over hidden automation.
3. Keep video, MAVLink, and vendor protocols as separate planes.
4. Put hardware-specific behavior at the edges.
5. Prefer standards and documented interfaces.
6. Make failures recoverable without rebooting the aircraft.
7. Make the system testable without the complete airframe.
8. Optimize only after measuring the real hardware.

The central architectural rule is:

> If a hardware component or transport changes, the core camera and gimbal software should remain largely unchanged. Hardware-specific behavior belongs at the edges of the system, while core modules communicate through stable, testable interfaces.
