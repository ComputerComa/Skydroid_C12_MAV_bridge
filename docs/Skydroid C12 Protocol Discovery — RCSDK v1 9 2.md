# Skydroid C12 Protocol Discovery — RCSDK v1.9.2

> **Status:** Major protocol discovery from decompiling Skydroid RCSDK v1.9.2 on July 15, 2026.

>

> The SDK contains readable, C12-specific implementations for gimbal movement, camera control, thermal settings, telemetry parsing, UDP transport, and checksum generation. This materially reduces the amount of blind reverse engineering required for the Teensy 4.1 MAVLink-to-C12 bridge.

# Executive summary

- The SDK explicitly implements `com.skydroid.rcsdk.common.payload.C12`.
- Default C12 address confirmed in the SDK: `192.168.144.108`.
- C12 payload control can be instantiated over UDP with a configurable local bind port, remote IP, and remote port.
- Port `12580` is strongly associated with the payload protocol in the SDK.
- Commands are short ASCII strings beginning with `#TPU`.
- Commands append a two-character uppercase hexadecimal checksum.
- Commands awaiting replies are terminated with `\r\n`.
- Default response timeout is 2500 ms.
- The public C12 wrapper exposes gimbal rates, absolute angles, recording, photography, zoom, thermal palettes, video settings, attitude telemetry, calibration, and other thermal controls.
- Most relevant behavior appears in recoverable Java/Kotlin rather than the bundled native AR8030 libraries.

# Sources and provenance

## Decompiled SDK

- Input: `rcsdk-v1.9.2.aar`
- Local JADX export: `C:\Users\ComputerComa\Downloads\RCSDK-deco`
- Key classes:
  - `com.skydroid.rcsdk.common.payload.C12`
  - `com.skydroid.rcsdk.internal.payload.SkydroidGimbalControlCore`
  - `com.skydroid.rcsdk.internal.payload.TopCameraCore`
  - `com.skydroid.rcsdk.internal.payload.TopParser`
  - `com.skydroid.rcsdk.common.pipeline.UDPPipeline`
  - `com.skydroid.rcsdk.PayloadManager`

## Independent protocol reference

- [Reverse-engineered C12 Python repository](https://github.com/Mithilesh5957/demo-master-nidar-)
- [Commit-pinned C12Driver.py](https://github.com/Mithilesh5957/demo-master-nidar-/blob/bf87ea6c3d698c5317f527f2f6e9dec5e0eec695/pi_scripts/C12Driver.py)
- A local copy of the repository has also been preserved.

# Packet format

## Checksum

Both `SkydroidGimbalControlCore` and `TopCameraCore` implement the same checksum:

1. Encode the command body as UTF-8/ASCII.
2. Add every byte.
3. Keep the low eight bits.
4. Format that byte as two uppercase hexadecimal characters.
5. Append it to the command body.

```cpp
String finalizeCommand(const String& body, bool appendCrlf = false)
{
    uint8_t checksum = 0;

    for (size_t i = 0; i < body.length(); ++i)
        checksum += static_cast<uint8_t>(body[i]);

    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", checksum);

    return body + hex + (appendCrlf ? "\r\n" : "");
}
```

## Transport behavior

- Protocol: UDP
- Default device IP: `192.168.144.108`
- SDK payload constructor accepts local bind port, remote IP, and remote port.
- Port `12580` is repeatedly associated with the payload protocol.
- Earlier independent work identified port `5000` for gimbal traffic and `12580` for camera traffic. The exact division must be verified against the real C12.
- Replies appear to be processed through the same UDP pipeline and matched by three-character response flags such as `REC`, `IMG`, or `GAC`.

# Confirmed camera command bodies

| Function                | Command body   |
| ----------------------- | -------------- |
| Take picture            | `#TPUD2wCAP01` |
| Start recording         | `#TPUD2wREC01` |
| Stop recording          | `#TPUD2wREC00` |
| Read recording state    | `#TPUD2rREC00` |
| Read camera version     | `#TPUD2rVER00` |
| Read camera model       | `#TPUD2rMOD00` |
| Read SD-card capacity   | `#TPUD2rSDC01` |
| Factory reset           | `#TPUD2wRTF01` |
| Zoom in one step        | `#TPUD2wDZM0A` |
| Zoom out one step       | `#TPUD2wDZM0B` |
| Read zoom               | `#TPUD2rDZM00` |
| Select telephoto lens   | `#TPUD2wDZM0C` |
| Select alternate lens   | `#TPUD2wDZM0D` |
| Read thermal palette    | `#TPUD2rIMG00` |
| Read thermal scene mode | `#TPUD2rTSM00` |
| Read camera IP          | `#TPUD2rIPV00` |
| Read gateway            | `#TPUD2rGTW00` |
| Reboot camera           | `#TPUD2wRST00` |

Append the checksum to each body. Commands expecting replies are sent with `\r\n` after the checksum.

# Thermal palette commands

Palette selection uses:

```
#TPUD2wIMGxx
```

| Palette   | Value |
| --------- | ----- |
| White hot | `01`  |
| Sepia     | `03`  |
| Ironbow   | `04`  |
| Rainbow   | `05`  |
| Night     | `06`  |
| Aurora    | `07`  |
| Red hot   | `08`  |
| Jungle    | `09`  |
| Medical   | `0A`  |
| Black hot | `0B`  |
| Glory hot | `0C`  |

Example body for Ironbow:

```
#TPUD2wIMG04
```

# Thermal scene modes

Scene selection uses `#TPUD2wTSMxx`.

| Scene                   | Value |
| ----------------------- | ----- |
| Default                 | `00`  |
| Patrol                  | `01`  |
| Manual                  | `02`  |
| Low temperature         | `03`  |
| Linear                  | `04`  |
| Low contrast            | `05`  |
| High contrast           | `06`  |
| Highlight               | `07`  |
| Rescue                  | `0C`  |
| Urban                   | `0D`  |
| Temperature measurement | `0E`  |
| Outdoors                | `0F`  |

# Other confirmed thermal controls

| Function                 | Write prefix | Read command   |
| ------------------------ | ------------ | -------------- |
| Thermal shutter          | `#TPUD2wTAS` | `#TPUD2rTAS00` |
| Detail enhancement       | `#TPUD2wTDI` | `#TPUD2rTDI00` |
| Brightness               | `#TPUD2wTIB` | `#TPUD2rTIB00` |
| Contrast                 | `#TPUD2wTIC` | `#TPUD2rTIC00` |
| Spatial noise reduction  | `#TPUD2wTAR` | `#TPUD2rTAR00` |
| Temporal noise reduction | `#TPUD2wTTR` | `#TPUD2rTTR00` |
| Gamma                    | `#TPUD2wTGM` | `#TPUD2rTGM00` |

Numeric write values are converted to hexadecimal by the SDK. Valid ranges should be extracted and bench-verified before exposing them to operators.

# Confirmed gimbal command structures

| Function                     | Command body structure   |
| ---------------------------- | ------------------------ |
| Yaw rate                     | `#TPUG2wGSYxx`           |
| Pitch rate                   | `#TPUG2wGSPxx`           |
| Combined yaw/pitch rate      | `#TPUG4wGSMxxyy`         |
| Absolute yaw                 | `#TPUG6wGAYaaaa10`       |
| Absolute pitch               | `#TPUG6wGAPaaaa10`       |
| Absolute roll                | `#TPUG6wGARaaaa10`       |
| Absolute yaw/pitch           | `#TPUGCwGAMaaaa10bbbb10` |
| Configure attitude push rate | `#TPUG2wGAAxx`           |

## Rate encoding

- Rate fields are signed 8-bit values.
- SDK scale is 0.5 degrees/second per count.
- The SDK clamps the encoded value to -127 through +127.
- Combined movement contains yaw then pitch.

## Angle encoding

- Convert degrees to hundredths: `encoded = degrees × 100`.
- Store as a signed 16-bit value.
- Encode as four hexadecimal characters.
- The SDK clamps these absolute commands to ±90 degrees.
- The meaning of the literal `10` following each angle remains to be confirmed; it may represent speed, transition behavior, or another control field.

# Gimbal modes and actions

The SDK exposes commands for:

- Center and preset A-key actions
- Follow/lock-related gimbal control modes
- Assembly orientation
- Calibration
- Fine tuning
- Attitude reporting
- Model and version queries

Observed PTZ action bodies include:

```
#TPUG2wPTZ01
#TPUG2wPTZ02
#TPUG2wPTZ03
#TPUG2wPTZ04
#TPUG2wPTZ05
#TPUG2wPTZ06
#TPUG2wPTZ07
#TPUG2wPTZ08
#TPUG2wPTZ0A
#TPUG2wPTZ0B
#TPUG2wPTZ0C
#TPUG2wPTZ0D
#TPUG2wPTZ0E through #TPUG2wPTZ14
```

Their exact enum-to-physical-action mapping should be documented from the associated enum switch tables and then confirmed on the bench.

# Attitude telemetry

- The SDK enables push telemetry with `#TPUG2wGAAxx`.
- It searches incoming packets for the `GAC` response field.
- The payload contains three signed 16-bit values.
- Each value is divided by 100 to produce angles.
- The SDK constructs a `GimbalAttitud` object from those three values.

The exact axis ordering should be verified by moving one axis at a time while logging telemetry.

# Additional camera capabilities exposed by the SDK

The C12 wrapper also exposes:

- Camera IP and gateway configuration
- Camera reboot and factory reset
- Camera and gimbal version/model queries
- Video resolution
- Frame rate, GOP, bitrate, horizontal flip, and vertical flip
- Image tone, brightness, saturation, contrast, and sharpness
- SD-card capacity
- OSD configuration
- Time synchronization
- Thermal calibration
- Ranging query
- Recording-state query

# Important decompilation warning

The decompiled `gotoPitch()` and `gotoRoll()` methods appear to call the checksum routine twice:

```java
String command = genSendControlCmd(body);
writeData(genSendControlCmd(command));
```

Yaw and combined yaw/pitch only apply it once. This may be:

- an SDK bug;
- a JADX reconstruction artifact; or
- an unusual protocol exception.

Do not implement double checksums until a real C12 packet capture or controlled bench test confirms it. Compare behavior against the independent Python driver.

## Recommended firmware layers

1. `C12Protocol`
   - Checksum generation
   - Signed integer encoding
   - Command construction
   - Response framing and parsing
2. `C12UdpTransport`
   - Static network configuration
   - UDP send/receive
   - Timeouts
   - Connection health
3. `C12Gimbal`
   - Rate commands
   - Absolute angles
   - Modes, center, calibration
   - Attitude telemetry
4. `C12Camera`
   - Photo and recording
   - Zoom and lens selection
   - Thermal palette and scene settings
5. `SerialCommandShell`
   - Early bench testing without MAVLink
6. `MavlinkGimbalDevice`
   - ArduPilot Gimbal Device v2 translation
   - Heartbeats, information, status, commands, and acknowledgements
7. `SafetySupervisor`
   - Command timeout
   - Explicit zero-rate stop
   - Startup inhibition
   - Ethernet/UART watchdogs

# First bench-test command shell

Implement simple serial commands first:

```
help
status
center
down
stop
yaw-rate <deg_s>
pitch-rate <deg_s>
angles <yaw_deg> <pitch_deg>
photo
record start
record stop
palette white
palette ironbow
palette blackhot
scene rescue
attitude on <rate>
attitude off
```

This proves the C12 transport and protocol independently of ArduPilot and MAVLink.

# Verification plan

- [ ] Power the C12 through its protected/fused payload branch.
- [ ] Connect the C12 and Teensy through SwitchBlox.
- [ ] Configure the Teensy on the `192.168.144.0/24` subnet.
- [ ] Verify Ethernet link and ARP visibility.
- [ ] Test UDP port `12580`.
- [ ] Test the independently reported gimbal UDP port `5000`.
- [ ] Send a harmless version/model query before movement commands.
- [ ] Verify the checksum implementation.
- [ ] Send center and explicit zero-rate commands.
- [ ] Test conservative pitch/yaw rate commands.
- [ ] Validate absolute-angle encoding and axis signs.
- [ ] Determine whether pitch/roll require one or two checksum applications.
- [ ] Enable attitude telemetry and determine axis ordering.
- [ ] Test photo and recording controls.
- [ ] Test visible/thermal lens switching.
- [ ] Test multiple thermal palettes.
- [ ] Verify thermal-setting ranges.
- [ ] Capture all successful transactions with Wireshark.
- [ ] Compare SDK-generated traffic, Python-driver traffic, and Teensy traffic.
- [ ] Write a clean, verified C12 protocol specification before MAVLink integration.

# Safety rules

- Clamp all gimbal angles and rates.
- Begin movement tests at very low rates.
- Keep the gimbal mechanically unobstructed.
- Send an explicit zero-rate command after every rate test.
- Stop motion if the controlling serial/MAVLink stream disappears for 250–500 ms.
- Do not enable automatic tracking until manual override and loss-of-command behavior are proven.
- Treat decompiled behavior as evidence, not unquestionable documentation.
- Preserve the original AAR, decompiled export, repository copy, packet captures, and a record of SDK version 1.9.2.

# Current conclusion

The C12 is no longer merely a proprietary camera with one unofficial Python reference. Skydroid's own SDK contains a readable, purpose-built C12 control implementation with enough detail to create an independent Teensy client.

This substantially lowers the technical risk of:

- Manual ArduPilot gimbal control
- MAVLink Gimbal Device v2 translation
- Thermal palette and scene control
- Photo and recording functions
- Gimbal attitude feedback
- Future ROI/POI tracking
- Later OpenCV hotspot tracking and geolocation

The immediate next milestone is a minimal Teensy UDP client that sends a non-motion query, validates the reply, and then performs conservative manual gimbal control.
