# Codex and Contributor Guidance

## Scope

This file provides implementation guidance for work performed from the `docs/` subtree and documents the expectations that should also be followed when designing or modifying the repository's production software.

Before making architectural or protocol changes, read:

- `docs/platform-design-goals.md`
- `docs/video-path-options.md`
- `docs/proposed_pathing.mmd`
- `README.md`
- `ROADMAP.md`

When documentation and code disagree, do not silently choose one. Identify the inconsistency and update the design documentation or implementation as part of the same change.

## Project intent

This repository implements a native Linux bridge that exposes the Skydroid C12 as MAVLink camera and gimbal components for ArduPilot while keeping video on a separate IP media path.

The Raspberry Pi is a headless onboard systems computer. It is not primarily a desktop, display generator, or HDMI appliance.

The software should remain usable as the platform evolves from a Pixhawk 6C to a Cube Orange+ and from serial/USB connectivity toward an Ethernet-first DroneNet architecture.

## Required architectural boundaries

Keep these concerns separate, even if early versions run them in one process:

1. Flight-controller transport
2. MAVLink parsing and routing
3. Camera Protocol v2 behavior
4. Gimbal behavior
5. C12 vendor-protocol translation
6. Video ingestion, relay, and optional processing
7. Diagnostics, logging, and metrics
8. Configuration and service supervision

Do not allow socket, UART, USB, or radio-specific details to leak into core camera and gimbal logic.

Core modules should consume decoded commands and produce domain-level results. Transport adapters should handle framing, reconnects, device paths, and network endpoints.

## Language and platform rules

- Production services should be written in C or C++.
- Prefer modern, conservative C++ features when C++ is used.
- Do not add Python as a production runtime dependency without an explicit design decision.
- Target Raspberry Pi OS Lite 64-bit on ARM64.
- Preserve build and test support under Debian WSL2.
- Use CMake and Ninja unless the repository explicitly changes build systems.
- Prefer POSIX/Linux APIs and libraries available from normal Debian or Raspberry Pi OS repositories.
- Avoid desktop-environment, X11, Wayland, or interactive-shell dependencies.
- Services must run unattended under systemd.

## MAVLink rules

- Use the official MAVLink XML definitions and generated headers as the source of truth.
- Never hand-author MAVLink packet layouts, CRC values, field offsets, or message IDs.
- Use MAVLink 2 unless compatibility testing requires otherwise.
- Preserve system ID and component ID ownership rules.
- Track MAVLink sequence numbers separately for each `(system_id, component_id)` source.
- Do not treat a sequence gap as proof of RF packet loss without considering routing, filtering, component changes, startup, and duplicate paths.
- Expose parser, framing, CRC, and sequence diagnostics where the generated MAVLink library provides them.
- A command handler must return a standards-appropriate acknowledgement when the protocol requires one.
- Unsupported commands should fail explicitly rather than appearing to succeed.
- Do not invent Camera Protocol v2 capabilities that the C12 or bridge cannot actually perform.

## Video rules

- MAVLink is not a video transport.
- Camera Protocol v2 advertises stream metadata and reachable endpoints; it does not carry visible or thermal frames.
- Keep the media plane independent from the MAVLink command plane.
- Prefer direct RTSP/RTP relay or repacketization when codec compatibility permits.
- Do not decode and re-encode video unless pixels must be modified or the receiving path requires a different codec or packetization.
- When transcoding is required, make bitrate, codec, resolution, frame rate, keyframe interval, and hardware-acceleration settings explicit and configurable.
- Stream metadata must describe the endpoint that the ground station can actually reach.
- Do not hard-code unverified secondary WFB-ng ports, C12 URLs, codecs, or HM30 addressing.
- Treat the SIYI HM30 as the current preferred fieldable IP downlink, not as a permanent dependency in core video logic.
- Keep WFB-ng support possible behind the same video-output abstraction.

## C12 adapter rules

- Isolate all Skydroid-specific commands, packet formats, addresses, and quirks in a dedicated adapter module.
- Do not expose vendor packet structures directly to MAVLink handlers.
- Validate lengths, checksums, ranges, and response types before using incoming data.
- Represent timeouts, negative responses, unavailable functions, and stale telemetry explicitly.
- Document any behavior inferred from testing rather than confirmed by vendor documentation.
- Keep visible and thermal stream handling distinct where the hardware exposes separate streams.

## Networking rules

- Prefer explicit configuration over implicit discovery.
- Bind local-only control sockets to loopback where practical.
- Document all externally reachable ports.
- Do not expose unauthenticated administrative services to the public internet.
- Use stable interface names, static addresses, or clearly documented DHCP reservations for aircraft components.
- Avoid assuming that the companion computer has only one network interface.
- Handle route changes and interface loss without requiring a reboot.
- Keep LTE/Tailscale, 915 MHz telemetry, and the primary video/data link logically independent.
- Prevent accidental routing loops and duplicate MAVLink forwarding when multiple telemetry paths are active.

## Serial and device rules

- Prefer `/dev/serial/by-id/...` paths over enumeration-dependent names.
- Make baud rate, flow control, reconnect policy, and read timeout configurable.
- Open serial devices in a well-defined raw mode.
- Handle partial reads and writes.
- Do not assume one `read()` call contains one MAVLink packet.
- Log disconnect and reconnect events with enough context to identify the device.
- Do not let multiple processes open the same flight-controller serial device unless a deliberate proxy architecture is used.

## Reliability rules

Every external interface must define:

- startup behavior
- initialization timeout
- reconnect behavior
- retry and backoff policy
- stale-data threshold
- degraded-mode behavior
- shutdown behavior

Services should recover from C12, RTSP, Ethernet, LTE, telemetry-radio, and flight-controller disconnects without rebooting the aircraft.

Avoid unbounded queues. When producers exceed consumers, use a documented drop or backpressure policy.

Avoid blocking the main control path on video processing, DNS, logging, or remote network operations.

## Diagnostics requirements

New transport or protocol code should provide counters or logs for relevant events, including:

- bytes received and transmitted
- messages or packets received and transmitted
- malformed frames
- parser failures
- CRC failures where detectable
- sequence gaps
- duplicate packets
- reconnect attempts
- timeouts
- queue depth or dropped work
- RTSP state changes
- media bitrate and packet loss where measurable
- CPU, memory, temperature, and throttling state

Logs should identify the subsystem and endpoint. Prefer structured fields or consistent prefixes over ambiguous prose.

Do not log high-rate packet contents by default. Make raw capture and verbose protocol traces opt-in.

## Configuration rules

- Keep addresses, ports, device paths, IDs, intervals, and stream settings out of business logic.
- Provide conservative defaults only where they are safe and documented.
- Validate configuration at startup and fail clearly on conflicts.
- Do not silently fall back to an unrelated device or port.
- Make configuration examples match the actual supported schema.
- Avoid breaking existing configuration without migration notes.

## Testing expectations

Changes should be testable without the complete aircraft.

Prefer interfaces that support:

- loopback UDP endpoints
- pseudo-terminals
- recorded MAVLink streams
- synthetic MAVLink peers
- mocked C12 responses
- synthetic H.264 or H.265 RTP sources
- packet loss and reordering simulation
- WSL2 host tests
- Raspberry Pi hardware-in-the-loop tests

Add focused tests for parsers, state machines, command acknowledgements, timeout behavior, and reconnect logic.

Do not make unit tests depend on live internet access, RF hardware, or the physical C12.

## Coding style expectations

- Favor small modules with explicit ownership.
- Use clear names based on protocol concepts rather than vague terms such as `manager2` or `handler_misc`.
- Check all system-call and library return values.
- Preserve `errno` or equivalent error context when reporting failures.
- Use monotonic time for intervals and timeout calculations.
- Avoid hidden global mutable state.
- Make thread ownership and synchronization explicit.
- Prefer RAII for C++ resource ownership.
- Avoid exceptions crossing C or system-service boundaries unless the repository establishes a consistent policy.
- Keep high-rate paths allocation-conscious, but do not sacrifice correctness for premature optimization.
- Comment protocol decisions, invariants, and non-obvious failure handling rather than restating code.

## Change discipline

Before adding a dependency:

1. Confirm that it is available and maintained on ARM64 Raspberry Pi OS.
2. Confirm that it works under Debian WSL2 where host testing is expected.
3. Explain why the standard library, POSIX APIs, or an existing dependency are insufficient.
4. Consider package size, update risk, and systemd deployment impact.

Before changing an interface:

1. Identify all callers and consumers.
2. Preserve compatibility where practical.
3. Update examples and documentation.
4. Add or update tests.
5. Record any operational migration steps.

## Documentation expectations

Architecture documents must distinguish among:

- selected design
- proposed design
- unverified assumption
- vendor claim
- measured result

Use Mermaid for editable logical diagrams where practical. Include text descriptions so the design remains understandable when diagrams do not render.

Hardware wiring documentation should include:

- source and destination connector
- signal names
- voltage levels
- power source
- expected current
- grounding
- cable or pinout notes
- whether a connection is bidirectional

Do not present an estimated range, latency, bitrate, or power figure as measured fact.

## Pull request expectations

A pull request should state:

- the problem being solved
- the selected design
- alternatives considered
- protocol or hardware assumptions
- tests performed
- tests not yet possible
- operational or configuration changes
- known risks

Keep unrelated refactoring out of focused protocol or hardware changes.

## Priority order for implementation decisions

When requirements conflict, use this order:

1. Flight safety and preservation of independent telemetry
2. Correct protocol behavior
3. Recoverability and observability
4. Clear module boundaries
5. Testability
6. Field maintainability
7. Performance based on measurement
8. Convenience and feature breadth

The preferred solution is not necessarily the one with the fewest files or shortest code. It is the one that keeps hardware-specific behavior at the edges, makes failure modes visible, and allows the platform to evolve without rewriting the core bridge.
